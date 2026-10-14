// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

/// \file
/// Implementation of ImageBufAlgo algorithms that do math on
/// single pixels at a time.

#include <cmath>
#include <iostream>
#include <limits>

#include <OpenImageIO/half.h>

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/deepdata.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imagebufalgo_util.h>
#include <OpenImageIO/simd.h>

#include "imageio_pvt.h"


OIIO_NAMESPACE_3_1_BEGIN


static bool
decode_over_channels(const ImageBuf& R, int& nchannels, int& alpha, int& z,
                     int& colors)
{
    if (!R.initialized()) {
        alpha  = -1;
        z      = -1;
        colors = 0;
        return false;
    }
    const ImageSpec& spec(R.spec());
    alpha          = spec.alpha_channel;
    bool has_alpha = (alpha >= 0);
    z              = spec.z_channel;
    bool has_z     = (z >= 0);
    nchannels      = spec.nchannels;
    colors         = nchannels - has_alpha - has_z;
    if (!has_alpha && colors == 4) {
        // No marked alpha channel, but suspiciously 4 channel -- assume
        // it's RGBA.
        colors -= 1;
        // Assume alpha is the highest channel that's not z
        alpha = nchannels - 1;
        if (alpha == z)
            --alpha;
    }
    return true;
}



// Fully type-specialized version of over.
template<class Rtype, class Atype, class Btype>
static bool
over_impl(ImageBuf& R, const ImageBuf& A, const ImageBuf& B, bool zcomp,
          bool z_zeroisinf, ROI roi, int nthreads)
{
    // It's already guaranteed that R, A, and B have matching channel
    // ordering, and have an alpha channel.  So just decode one.
    int nchannels = 0, alpha_channel = 0, z_channel = 0, ncolor_channels = 0;
    decode_over_channels(R, nchannels, alpha_channel, z_channel,
                         ncolor_channels);
    bool has_z = (z_channel >= 0);

    ImageBufAlgo::parallel_image(roi, nthreads, [=, &R, &A, &B](ROI roi) {
        ImageBuf::ConstIterator<Atype> a(A, roi);
        ImageBuf::ConstIterator<Btype> b(B, roi);
        ImageBuf::Iterator<Rtype> r(R, roi);
        for (; !r.done(); ++r, ++a, ++b) {
            float az = 0.0f, bz = 0.0f;
            bool a_is_closer = true;  // will remain true if !zcomp
            if (zcomp && has_z) {
                az = a[z_channel];
                bz = b[z_channel];
                if (z_zeroisinf) {
                    if (az == 0.0f)
                        az = std::numeric_limits<float>::max();
                    if (bz == 0.0f)
                        bz = std::numeric_limits<float>::max();
                }
                a_is_closer = (az <= bz);
            }
            if (a_is_closer) {
                // A over B
                float alpha           = clamp(a[alpha_channel], 0.0f, 1.0f);
                float one_minus_alpha = 1.0f - alpha;
                for (int c = roi.chbegin; c < roi.chend; c++)
                    r[c] = a[c] + one_minus_alpha * b[c];
                if (has_z)
                    r[z_channel] = (alpha != 0.0) ? a[z_channel] : b[z_channel];
            } else {
                // B over A -- because we're doing a Z composite
                float alpha           = clamp(b[alpha_channel], 0.0f, 1.0f);
                float one_minus_alpha = 1.0f - alpha;
                for (int c = roi.chbegin; c < roi.chend; c++)
                    r[c] = b[c] + one_minus_alpha * a[c];
                r[z_channel] = (alpha != 0.0) ? b[z_channel] : a[z_channel];
            }
        }
    });

    return true;
}



// Special case -- 4 channel RGBA float, in-memory buffer, no wrapping.
// Use loops and SIMD.
static bool
over_impl_rgbafloat(ImageBuf& R, const ImageBuf& A, const ImageBuf& B, ROI roi,
                    int nthreads)
{
    using namespace simd;
    OIIO_DASSERT(A.localpixels() && B.localpixels()
                 && A.spec().format == TypeFloat && A.nchannels() == 4
                 && B.spec().format == TypeFloat && B.nchannels() == 4
                 && A.spec().alpha_channel == 3 && A.spec().z_channel < 0
                 && B.spec().alpha_channel == 3 && B.spec().z_channel < 0);
    // const int nchannels = 4, alpha_channel = 3;
    ImageBufAlgo::parallel_image(roi, nthreads, [=, &R, &A, &B](ROI roi) {
        vfloat4 zero = vfloat4::Zero();
        vfloat4 one  = vfloat4::One();
        int w        = roi.width();
        for (int z = roi.zbegin; z < roi.zend; ++z) {
            for (int y = roi.ybegin; y < roi.yend; ++y) {
                float* r       = (float*)R.pixeladdr(roi.xbegin, y, z);
                const float* a = (const float*)A.pixeladdr(roi.xbegin, y, z);
                const float* b = (const float*)B.pixeladdr(roi.xbegin, y, z);
                for (int x = 0; x < w; ++x, r += 4, a += 4, b += 4) {
                    vfloat4 a_simd(a);
                    vfloat4 b_simd(b);
                    vfloat4 alpha           = broadcast_element<3>(a_simd);
                    vfloat4 one_minus_alpha = one - clamp(alpha, zero, one);
                    vfloat4 result          = a_simd + one_minus_alpha * b_simd;
                    result.store(r);
                }
            }
        }
    });
    return true;
}



bool
ImageBufAlgo::over(ImageBuf& dst, const ImageBuf& A, const ImageBuf& B, ROI roi,
                   int nthreads)
{
    OIIO::pvt::LoggedTimer logtime("IBA::over");
    if (!IBAprep(roi, &dst, &A, &B, NULL,
                 IBAprep_REQUIRE_ALPHA | IBAprep_REQUIRE_SAME_NCHANNELS))
        return false;

    if (A.localpixels() && B.localpixels() && A.spec().format == TypeFloat
        && A.nchannels() == 4 && B.spec().format == TypeFloat
        && B.nchannels() == 4 && A.spec().alpha_channel == 3
        && A.spec().z_channel < 0 && B.spec().alpha_channel == 3
        && B.spec().z_channel < 0 && A.roi().contains(roi)
        && B.roi().contains(roi) && roi.chbegin == 0 && roi.chend == 4) {
        // Easy case -- both buffers are float, 4 channels, alpha is
        // channel[3], no special z channel, and pixel data windows
        // completely cover the roi. This reduces to a simpler case we can
        // handle without iterators and taking advantage of SIMD.
        return over_impl_rgbafloat(dst, A, B, roi, nthreads);
    }

    bool ok;
    OIIO_DISPATCH_COMMON_TYPES3(ok, "over", over_impl, dst.spec().format,
                                A.spec().format, B.spec().format, dst, A, B,
                                false, false, roi, nthreads);
    return ok && !dst.has_error();
}



ImageBuf
ImageBufAlgo::over(const ImageBuf& A, const ImageBuf& B, ROI roi, int nthreads)
{
    ImageBuf result;
    bool ok = over(result, A, B, roi, nthreads);
    if (!ok && !result.has_error())
        result.errorfmt("ImageBufAlgo::over() error");
    return result;
}



bool
ImageBufAlgo::zover(ImageBuf& dst, const ImageBuf& A, const ImageBuf& B,
                    bool z_zeroisinf, ROI roi, int nthreads)
{
    OIIO::pvt::LoggedTimer logtime("IBA::zover");
    if (!IBAprep(roi, &dst, &A, &B, NULL,
                 IBAprep_REQUIRE_ALPHA | IBAprep_REQUIRE_Z
                     | IBAprep_REQUIRE_SAME_NCHANNELS))
        return false;
    bool ok;
    OIIO_DISPATCH_COMMON_TYPES3(ok, "zover", over_impl, dst.spec().format,
                                A.spec().format, B.spec().format, dst, A, B,
                                true, z_zeroisinf, roi, nthreads);
    return ok && !dst.has_error();
}



ImageBuf
ImageBufAlgo::zover(const ImageBuf& A, const ImageBuf& B, bool z_zeroisinf,
                    ROI roi, int nthreads)
{
    ImageBuf result;
    bool ok = zover(result, A, B, z_zeroisinf, roi, nthreads);
    if (!ok && !result.has_error())
        result.errorfmt("ImageBufAlgo::zover() error");
    return result;
}


OIIO_NAMESPACE_3_1_END
