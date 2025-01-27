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


/// image_span : a non-owning reference to a 4D image-like array (having
/// channel, x, y, z) with known dimensions and optionally non-default strides
/// (expressed in bytes) through the data.  An image_span<T> is mutable (the
/// values in the image may be modified), whereas an image_span<const T> is
/// not mutable.
///
/// Another way to describe an image_span is as a 4-dimensional span with
/// byte strides.
template<typename T> class image_span {
public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef int64_t stride_t;
    static constexpr stride_t AutoStride = std::numeric_limits<stride_t>::min();

    /// Default ctr -- points to nothing
    image_span() = default;

    /// Copy constructor
    image_span(const image_span& copy) = default;

    /// Construct from T*, dimensions, and (possibly default) strides (in
    /// bytes).
    image_span(T* data, int nchannels, int width, int height, int depth = 1,
               stride_t chanstride = AutoStride, stride_t xstride = AutoStride,
               stride_t ystride = AutoStride, stride_t zstride = AutoStride,
               uint32_t chansize = sizeof(T))
        : m_data(data)
        , m_nchannels(nchannels)
        , m_width(width)
        , m_height(height)
        , m_depth(depth)
        , m_chanstride(chanstride != AutoStride ? chanstride : chansize)
        , m_xstride(xstride != AutoStride ? xstride
                                          : m_nchannels * m_chanstride)
        , m_ystride(ystride != AutoStride ? ystride : m_width * m_xstride)
        , m_zstride(zstride != AutoStride ? zstride : m_height * m_ystride)
        , m_chansize(chansize)
    {
        // Validations:
        // - an image_span<byte> can have any chansize, but any other T must
        //   have the chansize equal to the data type size.
        OIIO_DASSERT(nchannels > 0 && width > 0 && height > 0 && depth > 0);
        OIIO_DASSERT((std::is_same<std::remove_const_t<T>, std::byte>::value)
                     || m_chansize == sizeof(T));
    }

    /// Construct from span<T> and dimensions, assume contiguous strides.
    image_span(span<T> data, int nchannels, int width, int height,
               int depth = 1)
        : image_span(data.data(), nchannels, width, height, depth)
    {
        // Validations:
        // - The full layout must fit within the original span size.
        OIIO_DASSERT(nvalues() <= data.size());
    }

    /// assignments -- not a deep copy, just make this image_span point to the
    /// same data as the operand.
    image_span& operator=(const image_span& copy) = default;

    /// image_span(x,y,z) returns a strided_ptr<T,1> for the pixel (x,y,z).
    /// The z can be omitted for 2D images.  Note than the resulting
    /// strided_ptr can then have individual channels accessed with
    /// operator[]. This particular strided pointer has stride multiplier 1,
    /// because this class uses bytes as strides, not sizeof(T).
    strided_ptr<T /*, 1*/> operator()(int x, int y, int z = 0)
    {
        return strided_ptr<T /*, 1*/>(getptr(0, x, y, z) /*, m_chanstride*/);
    }

    int nchannels() const { return m_nchannels; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int depth() const { return m_depth; }

    stride_t chanstride() const { return m_chanstride; }
    stride_t xstride() const { return m_xstride; }
    stride_t ystride() const { return m_ystride; }
    stride_t zstride() const { return m_zstride; }

    T* data() const { return m_data; }

    /// Convert an image_span<T> to an image_span<const std::byte>
    /// representing the same sized and strided memory pattern represented
    /// un-typed memory.
    image_span<const std::byte> as_bytes_image_span() const noexcept
    {
        return image_span<const std::byte>(static_cast<const std::byte*>(m_data),
                                           m_nchannels, m_width, m_height,
                                           m_depth, m_chanstride, m_xstride,
                                           m_ystride, m_zstride, m_chansize);
    }

    /// Convert an image_span<T> to an image_span<std::byte> representing the
    /// same sized and strided memory pattern represented un-typed memory.
    image_span<std::byte> as_writable_bytes_image_span() const noexcept
    {
        return image_span<std::byte>(static_cast<std::byte*>(m_data),
                                     m_nchannels, m_width, m_height, m_depth,
                                     m_chanstride, m_xstride, m_ystride,
                                     m_zstride, m_chansize);
    }

    /// Does this image_span represent contiguous data -- i.e. each channel,
    /// pixel, and scanline directly abuts its neighbour, with no gaps?
    bool is_contiguous() const noexcept
    {
        return /* pixel is contiguous channels */
            m_chanstride == m_chansize
            /* scanline is contiguous pixels */
            && m_xstride == m_chanstride * m_nchannels
            /* image plane is contiguous scanlines */
            && m_ystride == m_xstride * m_width
            /* volume is contiguous planes */
            && m_zstride == m_ystride * m_height;
    }

    /// Return the total number of values (c*w*h*d).
    size_t nvalues() const
    {
        return size_t(m_nchannels) * size_t(m_width) * size_t(m_height)
               * size_t(m_depth);
    }

    /// Return the total number of bytes of (c*w*h*d) values of the given type.
    size_t nbytes() const { return nvalues() * sizeof(m_chansize); }

    inline T& get(int c, int x, int y, int z = 0) const
    {
        // Bounds check in debug mode
        OIIO_DASSERT(unsigned(c) < unsigned(m_nchannels)
                     && unsigned(x) < unsigned(m_width)
                     && unsigned(y) < unsigned(m_height)
                     && unsigned(z) < unsigned(m_depth));
        return *getptr(c, x, y, z);
    }

    inline T* getptr(int c, int x, int y, int z = 0) const
    {
        // Bounds check in debug mode
        OIIO_DASSERT(unsigned(c) < unsigned(m_nchannels)
                     && unsigned(x) < unsigned(m_width)
                     && unsigned(y) < unsigned(m_height)
                     && unsigned(z) < unsigned(m_depth));
        return (T*)((char*)m_data + c * m_chanstride + x * m_xstride
                    + y * m_ystride + z * m_zstride);
    }

private:
    T* m_data { nullptr };
    uint32_t m_nchannels { 0 }, m_width { 0 }, m_height { 0 }, m_depth { 0 };
    stride_t m_chanstride { 0 }, m_xstride { 0 }, m_ystride { 0 },
        m_zstride { 0 };
    uint32_t m_chansize { sizeof(T) };
    // bool m_contiguous { true };

};


OIIO_NAMESPACE_END
