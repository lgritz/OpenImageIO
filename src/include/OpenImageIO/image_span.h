// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO


#pragma once

#include <stdexcept>
#include <vector>

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/span.h>
#include <OpenImageIO/strided_ptr.h>


OIIO_NAMESPACE_BEGIN

#ifndef OIIO_STRIDE_T_DEFINED
#    define OIIO_STRIDE_T_DEFINED
/// Type we use to express how many pixels (or bytes) constitute an image,
/// tile, or scanline.
using imagesize_t = uint64_t;

/// Type we use for stride lengths between pixels, scanlines, or image
/// planes.
using stride_t = int64_t;

/// Special value to indicate a stride length that should be
/// auto-computed.
inline constexpr stride_t AutoStride = std::numeric_limits<stride_t>::min();
#endif



/// image_span : a non-owning reference to n-D image-like array having between
/// 2 and 4 dimensions representing channel, x, y, z with each dimension
/// having known size and optionally non-default strides (expressed in bytes)
/// through the data.  An image_span<T> is mutable (the values in the image
/// may be modified), whereas an image_span<const T> is not mutable.
///
template<typename T, size_t Dims = 4>
class image_span {
    static_assert(Dims >= 2 && Dims <= 4, "Dimension must be between 2 and 4");

public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    using stride_t = int64_t;
    using stride_type = int64_t;
    using size_type = uint32_t;
    static constexpr stride_t AutoStride = std::numeric_limits<stride_t>::min();

    /// Default ctr -- points to nothing
    image_span() = default;

    /// Copy constructor
    image_span(const image_span& copy) = default;

    /// Construct from T*, dimensions, and (possibly default) strides (in
    /// bytes).
    // template<typename T = value_type, OIIO_ENABLE_IF(Dims == 4)>
    image_span(T* data, uint32_t nchannels, uint32_t width, uint32_t height,
               uint32_t depth = 1, stride_t chanstride = AutoStride,
               stride_t xstride = AutoStride, stride_t ystride = AutoStride,
               stride_t zstride = AutoStride, uint32_t chansize = sizeof(T))
        : m_data(data)
        , m_sizes({ nchannels, width, height, depth })
        , m_chansize(chansize)
    {
        chanstride = chanstride != AutoStride ? chanstride : chansize;
        xstride    = xstride != AutoStride ? xstride : nchannels * chanstride;
        if constexpr (Dims >= 3)
            ystride = ystride != AutoStride ? ystride : width * xstride;
        if constexpr (Dims >= 4)
            zstride = zstride != AutoStride ? zstride : height * ystride;

        m_strides[0] = chanstride;
        m_strides[1] = xstride;
        if constexpr (Dims >= 3)
            m_strides[2] = ystride;
        if constexpr (Dims >= 4)
            m_strides[3] = zstride;

        // Validations:
        // - an image_span<byte> can have any chansize, but any other T must
        //   have the chansize equal to the data type size.
        OIIO_DASSERT(nchannels > 0 && width > 0 && height > 0 && depth > 0);
        OIIO_DASSERT((std::is_same<std::remove_const_t<T>, std::byte>::value)
                     || chansize == sizeof(T));
    }

    /// Construct from span<T> and dimensions, assume contiguous strides.
    image_span(span<T> data, uint32_t nchannels, uint32_t width,
               uint32_t height, uint32_t depth = 1)
        : image_span(data.data(), nchannels, width, height, depth)
    {
        // Validations:
        // - The full layout must fit within the original span size.
        OIIO_DASSERT(nvalues() <= data.size());
    }

    /// assignments -- not a deep copy, just make this image_span point to the
    /// same data as the operand.
    image_span& operator=(const image_span& copy) = default;

#if 1
    /// image_span(x,y,z) returns a strided_ptr<T,1> for the pixel (x,y,z).
    /// The z can be omitted for 2D images.  Note than the resulting
    /// strided_ptr can then have individual channels accessed with
    /// operator[]. This particular strided pointer has stride multiplier 1,
    /// because this class uses bytes as strides, not sizeof(T).
    strided_ptr<T /*, 1*/> operator()(uint32_t x, uint32_t y, uint32_t z = 0)
    {
        return strided_ptr<T /*, 1*/>(getptr(0, x, y, z) /*, m_chanstride*/);
    }
    // strided_ptr<T /*, 1*/> operator()(int x, int y, int z = 0)
    // {
    //     return (*this)(uint32_t(x), uint32_t(y), uint32_t(x));
    // }
#endif

    static constexpr size_t rank() noexcept { return Dims; }

    constexpr size_type nchannels() const { return m_sizes[0]; }
    constexpr stride_type chanstride() const { return m_strides[0]; }

    constexpr size_type width() const { return m_sizes[1]; }
    constexpr stride_type xstride() const { return m_strides[1]; }

    constexpr size_type height() const
    {
        if constexpr (Dims >= 2)
            return m_sizes[2];
        else
            return 1;
    }
    constexpr stride_type ystride() const
    {
        if constexpr (Dims >= 2)
            return m_strides[2];
        else
            return 0;
    }

    constexpr size_type depth() const
    {
        if constexpr (Dims >= 3)
            return m_sizes[3];
        else
            return 1;
    }
    constexpr stride_type zstride() const
    {
        if constexpr (Dims >= 3)
            return m_strides[3];
        else
            return 0;
    }

    // int nchannels() const { return m_nchannels; }
    // int width() const { return m_width; }
    // int height() const { return m_height; }
    // int depth() const { return m_depth; }

    // stride_t chanstride() const { return m_chanstride; }
    // stride_t xstride() const { return m_xstride; }
    // stride_t ystride() const { return m_ystride; }
    // stride_t zstride() const { return m_zstride; }

    constexpr size_type chansize() const { return m_chansize; }

    T* data() const { return m_data; }

    /// Convert an image_span<T> to an image_span<const std::byte>
    /// representing the same sized and strided memory pattern represented
    /// un-typed memory.
    image_span<const std::byte> as_bytes_image_span() const noexcept
    {
        return image_span<const std::byte>(static_cast<const std::byte*>(m_data),
                                           nchannels(), width(), height(),
                                           depth(), chanstride(), xstride(),
                                           ystride(), zstride(), m_chansize);
    }

    /// Convert an image_span<T> to an image_span<std::byte> representing the
    /// same sized and strided memory pattern represented un-typed memory.
    image_span<std::byte> as_writable_bytes_image_span() const noexcept
    {
        return image_span<std::byte>(static_cast<std::byte*>(m_data),
                                     nchannels(), width(), height(), depth(),
                                     chanstride(), xstride(), ystride(),
                                     zstride(), m_chansize);
    }

    /// Does this image_span represent contiguous data -- i.e. each channel,
    /// pixel, and scanline directly abuts its neighbour, with no gaps?
    bool is_contiguous() const noexcept
    {
        return /* pixel is contiguous channels */
            chanstride() == m_chansize
            /* scanline is contiguous pixels */
            && xstride() == chanstride() * nchannels()
            /* image plane is contiguous scanlines */
            && ystride() == xstride() * width()
            /* volume is contiguous planes */
            && zstride() == ystride() * height();
    }

    /// Return the total number of values (c*w*h*d).
    size_t nvalues() const
    {
        return size_t(nchannels()) * size_t(width()) * size_t(height())
               * size_t(depth());
    }

    /// Return the total number of bytes of (c*w*h*d) values of the given type.
    size_t nbytes() const { return nvalues() * sizeof(chansize()); }

    inline T& get(int c, int x, int y, int z = 0) const
    {
        // Bounds check in debug mode
        OIIO_DASSERT(unsigned(c) < unsigned(nchannels())
                     && unsigned(x) < unsigned(width())
                     && unsigned(y) < unsigned(height())
                     && unsigned(z) < unsigned(depth()));
        return *getptr(c, x, y, z);
    }

    inline T* getptr(int c, int x, int y, int z = 0) const
    {
        // Bounds check in debug mode
        OIIO_DASSERT(unsigned(c) < unsigned(nchannels())
                     && unsigned(x) < unsigned(width())
                     && unsigned(y) < unsigned(height())
                     && unsigned(z) < unsigned(depth()));
        return (T*)((char*)data() + c * chanstride() + x * xstride()
                    + y * ystride() + z * zstride());
    }

private:
    T* m_data { nullptr };
    std::array<stride_type, Dims> m_strides;
    std::array<size_type, Dims> m_sizes;
    uint32_t m_chansize { sizeof(T) };
};


OIIO_NAMESPACE_END
