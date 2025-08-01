// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO


/////////////////////////////////////////////////////////////////////////////
/// \file
///
/// Provides a simple API that abstracts the reading and writing of
/// images.  Subclasses, which may be found in DSO/DLL's, implement
/// particular formats.
///
/////////////////////////////////////////////////////////////////////////////

// clang-format off

#pragma once
#define OPENIMAGEIO_IMAGEIO_H

#if defined(_MSC_VER)
// Ignore warnings about DLL exported classes with member variables that are template classes.
// This happens with the std::vector<T> and std::string members of the classes below.
#    pragma warning(disable : 4251)
#endif

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/export.h>
#include <OpenImageIO/span.h>
#include <OpenImageIO/image_span.h>
#include <OpenImageIO/paramlist.h>
#include <OpenImageIO/platform.h>
#include <OpenImageIO/strutil.h>
#include <OpenImageIO/thread.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/memory.h>

OIIO_NAMESPACE_BEGIN

class DeepData;
class ImageBuf;
class Timer;

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

// Signal that this version of ImageBuf has constructors from spans
#define OIIO_IMAGEINPUT_IMAGE_SPAN_SUPPORT 1
#define OIIO_IMAGEOUTPUT_IMAGE_SPAN_SUPPORT 1



/// Pointer to a function called periodically by read_image and
/// write_image.  This can be used to implement progress feedback, etc.
/// It takes an opaque data pointer (passed to read_image/write_image)
/// and a float giving the portion of work done so far.  It returns a
/// bool, which if 'true' will STOP the read or write.
typedef bool (*ProgressCallback)(void *opaque_data, float portion_done);



// Forward declaration of IOProxy
namespace Filesystem {
    class IOProxy;
}


/// ROI is a small helper struct describing a rectangular region of interest
/// in an image. The region is [xbegin,xend) x [begin,yend) x [zbegin,zend),
/// with the "end" designators signifying one past the last pixel in each
/// dimension, a la STL style.
///
struct ROI {
    ///@{
    /// @name ROI data members
    /// The data members are:
    ///
    ///     int xbegin, xend, ybegin, yend, zbegin, zend;
    ///     int chbegin, chend;
    ///
    /// These describe the spatial extent
    ///     [xbegin,xend) x [ybegin,yend) x [zbegin,zend)
    /// And the channel extent:
    ///     [chbegin,chend)]
    int xbegin, xend;
    int ybegin, yend;
    int zbegin, zend;
    int chbegin, chend;
    ///@}

    /// Default constructor is an undefined region. Note that this is also
    /// interpreted as All().
    constexpr ROI () noexcept : xbegin(std::numeric_limits<int>::min()), xend(0),
             ybegin(0), yend(0), zbegin(0), zend(0), chbegin(0), chend(0)
    { }

    /// Constructor with an explicitly defined region.
    ///
    constexpr ROI (int xbegin, int xend, int ybegin, int yend,
         int zbegin=0, int zend=1, int chbegin=0, int chend=10000) noexcept
        : xbegin(xbegin), xend(xend), ybegin(ybegin), yend(yend),
          zbegin(zbegin), zend(zend), chbegin(chbegin), chend(chend)
    { }

    /// Is a region defined?
    constexpr bool defined () const noexcept { return (xbegin != std::numeric_limits<int>::min()); }

    ///@{
    /// @name Spatial size functions.
    /// The width, height, and depth of the region.
    constexpr int width () const noexcept { return xend - xbegin; }  ///< Height
    constexpr int height () const noexcept { return yend - ybegin; } ///< Width
    constexpr int depth () const noexcept { return zend - zbegin; }  ///< Depth
    ///@}

    /// Number of channels in the region.  Beware -- this defaults to a
    /// huge number, and to be meaningful you must consider
    /// std::min (imagebuf.nchannels(), roi.nchannels()).
    constexpr int nchannels () const noexcept { return chend - chbegin; }

    /// Total number of pixels in the region.
    constexpr imagesize_t npixels () const noexcept {
        return defined()
            ? imagesize_t(width()) * imagesize_t(height()) * imagesize_t(depth())
            : 0;
    }

    /// All() is an alias for the default constructor, which indicates that
    /// it means "all" of the image, or no region restriction.  For example,
    ///     float myfunc (ImageBuf &buf, ROI roi = ROI::All());
    /// Note that this is equivalent to:
    ///     float myfunc (ImageBuf &buf, ROI roi = {});
    static constexpr ROI All () noexcept { return ROI(); }

    /// Test equality of two ROIs
    friend constexpr bool operator== (const ROI &a, const ROI &b) noexcept {
        return (a.xbegin == b.xbegin && a.xend == b.xend &&
                a.ybegin == b.ybegin && a.yend == b.yend &&
                a.zbegin == b.zbegin && a.zend == b.zend &&
                a.chbegin == b.chbegin && a.chend == b.chend);
    }
    /// Test inequality of two ROIs
    friend constexpr bool operator!= (const ROI &a, const ROI &b) noexcept {
        return (a.xbegin != b.xbegin || a.xend != b.xend ||
                a.ybegin != b.ybegin || a.yend != b.yend ||
                a.zbegin != b.zbegin || a.zend != b.zend ||
                a.chbegin != b.chbegin || a.chend != b.chend);
    }

    /// Test if the coordinate is within the ROI.
    constexpr bool contains (int x, int y, int z=0, int ch=0) const noexcept {
        return x >= xbegin && x < xend && y >= ybegin && y < yend
            && z >= zbegin && z < zend && ch >= chbegin && ch < chend;
    }

    /// Test if another ROI is entirely within our ROI.
    constexpr bool contains (const ROI& other) const noexcept {
        return (other.xbegin >= xbegin && other.xend <= xend &&
                other.ybegin >= ybegin && other.yend <= yend &&
                other.zbegin >= zbegin && other.zend <= zend &&
                other.chbegin >= chbegin && other.chend <= chend);
    }

    /// Stream output of the range
    friend std::ostream & operator<< (std::ostream &out, const ROI &roi) {
        out << roi.xbegin << ' ' << roi.xend << ' ' << roi.ybegin << ' '
            << roi.yend << ' ' << roi.zbegin << ' ' << roi.zend << ' '
            << roi.chbegin << ' ' << roi.chend;
        return out;
    }
};



/// Union of two regions, the smallest region containing both.
inline constexpr ROI roi_union (const ROI &A, const ROI &B) noexcept {
    return (A.defined() && B.defined())
        ? ROI (std::min (A.xbegin,  B.xbegin),  std::max (A.xend,  B.xend),
               std::min (A.ybegin,  B.ybegin),  std::max (A.yend,  B.yend),
               std::min (A.zbegin,  B.zbegin),  std::max (A.zend,  B.zend),
               std::min (A.chbegin, B.chbegin), std::max (A.chend, B.chend))
        : (A.defined() ? A : B);
}

/// Intersection of two regions.
inline constexpr ROI roi_intersection (const ROI &A, const ROI &B) noexcept {
    return (A.defined() && B.defined())
        ? ROI (std::max (A.xbegin,  B.xbegin),  std::min (A.xend,  B.xend),
               std::max (A.ybegin,  B.ybegin),  std::min (A.yend,  B.yend),
               std::max (A.zbegin,  B.zbegin),  std::min (A.zend,  B.zend),
               std::max (A.chbegin, B.chbegin), std::min (A.chend, B.chend))
        : (A.defined() ? A : B);
}




/// ImageSpec describes the data format of an image -- dimensions, layout,
/// number and meanings of image channels.
///
/// The `width, height, depth` are the size of the data of this image, i.e.,
/// the number of pixels in each dimension.  A ``depth`` greater than 1
/// indicates a 3D "volumetric" image. The `x, y, z` fields indicate the
/// *origin* of the pixel data of the image. These default to (0,0,0), but
/// setting them differently may indicate that this image is offset from the
/// usual origin.
/// Therefore the pixel data are defined over pixel coordinates
///    [`x` ... `x+width-1`] horizontally,
///    [`y` ... `y+height-1`] vertically,
///    and [`z` ... `z+depth-1`] in depth.
///
/// The analogous `full_width`, `full_height`, `full_depth` and `full_x`,
/// `full_y`, `full_z` fields define a "full" or "display" image window over
/// the region [`full_x` ... `full_x+full_width-1`] horizontally, [`full_y`
/// ... `full_y+full_height-1`] vertically, and [`full_z`...
/// `full_z+full_depth-1`] in depth.
///
/// Having the full display window different from the pixel data window can
/// be helpful in cases where you want to indicate that your image is a
/// *crop window* of a larger image (if the pixel data window is a subset of
/// the full display window), or that the pixels include *overscan* (if the
/// pixel data is a superset of the full display window), or may simply
/// indicate how different non-overlapping images piece together.
///
/// For tiled images, `tile_width`, `tile_height`, and `tile_depth` specify
/// that the image is stored in a file organized into rectangular *tiles*
/// of these dimensions. The default of 0 value for these fields indicates
/// that the image is stored in scanline order, rather than as tiles.
///

class OIIO_API ImageSpec {
public:
    ///@{
    /// @name ImageSpec data members
    ///
    /// The `ImageSpec` contains data fields for the values that are
    /// required to describe nearly any image, and an extensible list of
    /// arbitrary attributes that can hold metadata that may be user-defined
    /// or specific to individual file formats.
    ///
    /// Here are the hard-coded data fields:

    int x;                    ///< origin (upper left corner) of pixel data
    int y;                    ///< origin (upper left corner) of pixel data
    int z;                    ///< origin (upper left corner) of pixel data
    int width;                ///< width of the pixel data window
    int height;               ///< height of the pixel data window
    int depth;                ///< depth of pixel data, >1 indicates a "volume"
    int full_x;               ///< origin of the full (display) window
    int full_y;               ///< origin of the full (display) window
    int full_z;               ///< origin of the full (display) window
    int full_width;           ///< width of the full (display) window
    int full_height;          ///< height of the full (display) window
    int full_depth;           ///< depth of the full (display) window
    int tile_width;           ///< tile width (0 for a non-tiled image)
    int tile_height;          ///< tile height (0 for a non-tiled image)
    int tile_depth;           ///< tile depth (0 for a non-tiled image,
                              ///<             1 for a non-volume image)
    int nchannels;            ///< number of image channels, e.g., 4 for RGBA

    TypeDesc format;          ///< Data format of the channels.
        ///< Describes the native format of the pixel data values
        /// themselves, as a `TypeDesc`.  Typical values would be
        /// `TypeDesc::UINT8` for 8-bit unsigned values, `TypeDesc::FLOAT`
        /// for 32-bit floating-point values, etc.
    std::vector<TypeDesc> channelformats;
        ///< Optional per-channel data formats. If all channels of the image
        /// have the same data format, that will be described by `format`
        /// and `channelformats` will be empty (zero length). If there are
        /// different data formats for each channel, they will be described
        /// in the `channelformats` vector, and the `format` field will
        /// indicate a single default data format for applications that
        /// don't wish to support per-channel formats (usually this will be
        /// the format of the channel that has the most precision).

    std::vector<std::string> channelnames;
        ///< The names of each channel, in order. Typically this will be "R",
        ///< "G", "B", "A" (alpha), "Z" (depth), or other arbitrary names.
    int alpha_channel;
        ///< The index of the channel that represents *alpha* (pixel
        ///< coverage and/or transparency).  It defaults to -1 if no alpha
        ///< channel is present, or if it is not known which channel
        ///< represents alpha.
    int z_channel;
        ///< The index of the channel that represents *z* or *depth* (from
        ///< the camera).  It defaults to -1 if no depth channel is present,
        ///< or if it is not know which channel represents depth.
    bool deep;                ///< True if the image contains deep data.
        ///< If `true`, this indicates that the image describes contains
        ///< "deep" data consisting of multiple samples per pixel.  If
        ///< `false`, it's an ordinary image with one data value (per
        ///< channel) per pixel.
    ParamValueList extra_attribs;
        ///< A list of arbitrarily-named and arbitrarily-typed additional
        /// attributes of the image, for any metadata not described by the
        /// hard-coded fields described above.  This list may be manipulated
        /// with the `attribute()` and `find_attribute()` methods.

    ///@}

    /// Constructor: given just the data format, set all other fields to
    /// something reasonable.
    ImageSpec (TypeDesc format = TypeDesc::UNKNOWN) noexcept;

    /// Constructor: given just the data format (as any type name recognized
    /// by the `TypeDesc` constructor), set all other fields to something
    /// reasonable.
    ImageSpec (string_view format) noexcept : ImageSpec(TypeDesc(format)) {}

    /// Constructs an `ImageSpec` with the given x and y resolution, number
    /// of channels, and pixel data format.
    ///
    /// All other fields are set to the obvious defaults -- the image is an
    /// ordinary 2D image (not a volume), the image is not offset or a crop
    /// of a bigger image, the image is scanline-oriented (not tiled),
    /// channel names are "R", "G", "B"' and "A" (up to and including 4
    /// channels, beyond that they are named "channel *n*"), the fourth
    /// channel (if it exists) is assumed to be alpha.
    ImageSpec (int xres, int yres, int nchans, TypeDesc fmt = TypeUInt8) noexcept;

    /// Construct an `ImageSpec` with the given x and y resolution, number
    /// of channels, and pixel data format name (as any type name recognized
    /// by the `TypeDesc` constructor).
    ImageSpec (int xres, int yres, int nchans, string_view fmt) noexcept
        : ImageSpec(xres, yres, nchans, TypeDesc(fmt)) {}

    /// Construct an `ImageSpec` whose dimensions (both data and "full") and
    /// number of channels are given by the `ROI`, pixel data type by `fmt`,
    /// and other fields are set to their default values.
    explicit ImageSpec (const ROI &roi, TypeDesc fmt = TypeUInt8) noexcept;

    /// Construct an `ImageSpec` from an ROI giving dimensions, and the name
    /// of a data type that will be recognized by the `TypeDesc` constructor.
    explicit ImageSpec (const ROI &roi, string_view fmt) noexcept
        : ImageSpec(roi, TypeDesc(fmt)) {}

    /// Set the data format, and clear any per-channel format information
    /// in `channelformats`.
    void set_format (TypeDesc fmt) noexcept;

    /// Set the data format, and clear any per-channel format information
    /// in `channelformats`. The `fmt` may be a string such as `"uint8"`,
    /// or any other type name recognized by the TypeDesc constructor.
    void set_format (string_view fmt) noexcept { set_format(TypeDesc(fmt)); }

    /// Sets the `channelnames` to reasonable defaults for the number of
    /// channels.  Specifically, channel names are set to "R", "G", "B,"
    /// and "A" (up to and including 4 channels, beyond that they are named
    /// "channel*n*".
    void default_channel_names () noexcept;

    /// Returns the number of bytes comprising each channel of each pixel
    /// (i.e., the size of a single value of the type described by the
    /// `format` field).
    size_t channel_bytes() const noexcept { return format.size(); }

    /// Return the number of bytes needed for the single specified
    /// channel.  If native is false (default), compute the size of one
    /// channel of `this->format`, but if native is true, compute the size
    /// of the channel in terms of the "native" data format of that
    /// channel as stored in the file.
    size_t channel_bytes (int chan, bool native=false) const noexcept;

    /// Return the number of bytes for each pixel (counting all channels).
    /// If `native` is false (default), assume all channels are in
    /// `this->format`, but if `native` is true, compute the size of a pixel
    /// in the "native" data format of the file (these may differ in
    /// the case of per-channel formats).
    size_t pixel_bytes (bool native=false) const noexcept;

    /// Return the number of bytes for just the subset of channels in each
    /// pixel described by [chbegin,chend). If native is false (default),
    /// assume all channels are in this->format, but if native is true,
    /// compute the size of a pixel in the "native" data format of the file
    /// (these may differ in the case of per-channel formats).
    size_t pixel_bytes (int chbegin, int chend, bool native=false) const noexcept;

    /// Returns the number of bytes comprising each scanline, i.e.,
    /// `pixel_bytes(native) * width` This will return
    /// `std::numeric_limits<imagesize_t>::max()` in the event of an
    /// overflow where it's not representable in an `imagesize_t`.
    imagesize_t scanline_bytes (bool native=false) const noexcept;

    /// Returns the number of bytes comprising each scanline, if all channels
    /// were of the given type. If `type` is `TypeUnknown`, then it returns
    /// the bytes a scanline using each channel's native type. This will
    /// return `std::numeric_limits<imagesize_t>::max()` in the event of an
    /// overflow where it's not representable in an `imagesize_t`.
    imagesize_t scanline_bytes(TypeDesc type) const noexcept;

    /// Return the number of pixels comprising a tile (or 0 if it is not a
    /// tiled image).  This will return
    /// `std::numeric_limits<imagesize_t>::max()` in the event of an
    /// overflow where it's not representable in an `imagesize_t`.
    imagesize_t tile_pixels () const noexcept;

    /// Returns the number of bytes comprising an image tile, i.e.,
    ///     `pixel_bytes(native) * tile_width * tile_height * tile_depth`
    /// If native is false (default), assume all channels are in
    /// `this->format`, but if `native` is true, compute the size of a pixel
    /// in the "native" data format of the file (these may differ in the
    /// case of per-channel formats).
    imagesize_t tile_bytes (bool native=false) const noexcept;

    /// Returns the number of bytes comprising each tile, if all channels
    /// were of the given type. If `type` is `TypeUnknown`, then it returns
    /// the bytes a scanline using each channel's native type. This will
    /// return `std::numeric_limits<imagesize_t>::max()` in the event of an
    /// overflow where it's not representable in an `imagesize_t`.
    imagesize_t tile_bytes(TypeDesc type) const noexcept;

    /// Return the number of pixels for an entire image.  This will
    /// return `std::numeric_limits<imagesize_t>::max()` in the event of
    /// an overflow where it's not representable in an `imagesize_t`.
    imagesize_t image_pixels () const noexcept;

    /// Returns the number of bytes comprising an entire image of these
    /// dimensions, i.e.,
    ///     `pixel_bytes(native) * width * height * depth`
    /// This will return `std::numeric_limits<image size_t>::max()` in the
    /// event of an overflow where it's not representable in an
    /// `imagesize_t`. If `native` is false (default), assume all channels
    /// are in `this->format`, but if `native` is true, compute the size of
    /// a pixel in the "native" data format of the file (these may differ in
    /// the case of per-channel formats).
    imagesize_t image_bytes (bool native=false) const noexcept;

    /// Returns the number of bytes comprising an entire image of these
    /// dimensions, if the values were all of type `datatype`. For the
    /// special case of `datatype == `TypeUnknown`, compute the size of
    /// the image in the "native" data types for all channels.
    imagesize_t image_bytes(TypeDesc datatype) const noexcept;

    /// Verify that on this platform, a `size_t` is big enough to hold the
    /// number of bytes (and pixels) in a scanline, a tile, and the
    /// whole image.  If this returns false, the image is much too big
    /// to allocate and read all at once, so client apps beware and check
    /// these routines for overflows!
    bool size_t_safe() const noexcept {
        const imagesize_t big = std::numeric_limits<size_t>::max();
        return image_bytes() < big && scanline_bytes() < big &&
            tile_bytes() < big;
    }

    /// Adjust the stride values, if set to AutoStride, to be the right
    /// sizes for contiguous data with the given format, channels,
    /// width, height.
    static void auto_stride (stride_t &xstride, stride_t &ystride,
                             stride_t &zstride, stride_t channelsize,
                             int nchannels, int width, int height) noexcept {
        if (xstride == AutoStride)
            xstride = nchannels * channelsize;
        if (ystride == AutoStride)
            ystride = xstride * width;
        if (zstride == AutoStride)
            zstride = ystride * height;
    }

    /// Adjust the stride values, if set to AutoStride, to be the right
    /// sizes for contiguous data with the given format, channels,
    /// width, height.
    static void auto_stride (stride_t &xstride, stride_t &ystride,
                             stride_t &zstride, TypeDesc format,
                             int nchannels, int width, int height) noexcept {
        auto_stride (xstride, ystride, zstride, format.size(),
                     nchannels, width, height);
    }

    /// Adjust xstride, if set to AutoStride, to be the right size for
    /// contiguous data with the given format and channels.
    static void auto_stride (stride_t &xstride, TypeDesc format, int nchannels) noexcept {
        if (xstride == AutoStride)
            xstride = nchannels * format.size();
    }

    /// Add a metadata attribute to `extra_attribs`, with the given name and
    /// data type. The `value` pointer specifies the address of the data to
    /// be copied.
    void attribute (string_view name, TypeDesc type, const void *value);

    /// Add an `unsigned int` attribute to `extra_attribs`.
    void attribute (string_view name, unsigned int value) {
        attribute (name, TypeDesc::UINT, &value);
    }

    /// Add an `int` attribute to `extra_attribs`.
    void attribute (string_view name, int value) {
        attribute (name, TypeDesc::INT, &value);
    }

    /// Add a `float` attribute to `extra_attribs`.
    void attribute (string_view name, float value) {
        attribute (name, TypeDesc::FLOAT, &value);
    }

    /// Add a string attribute to `extra_attribs`.
    void attribute (string_view name, string_view value);

    /// Add a string attribute (passed as a ustring) to `extra_attribs`.
    void attribute (string_view name, ustring value);

    /// Parse a string containing a textual representation of a value of
    /// the given `type`, and add that as an attribute to `extra_attribs`.
    /// Example:
    ///
    ///     spec.attribute ("temperature", TypeFloat, "-273.15");
    ///
    void attribute (string_view name, TypeDesc type, string_view value);

    /// Searches `extra_attribs` for any attributes matching `name` (as a
    /// regular expression), removing them entirely from `extra_attribs`. If
    /// `searchtype` is anything other than `TypeDesc::UNKNOWN`, matches
    /// will be restricted only to attributes with the given type. The name
    /// comparison will be case-sensitive if `casesensitive` is true,
    /// otherwise in a case-insensitive manner.
    void erase_attribute (string_view name,
                          TypeDesc searchtype=TypeDesc::UNKNOWN,
                          bool casesensitive=false);

    /// Searches `extra_attribs` for an attribute matching `name`, returning
    /// a pointer to the attribute record, or NULL if there was no match.
    /// If `searchtype` is anything other than `TypeDesc::UNKNOWN`, matches
    /// will be restricted only to attributes with the given type. The name
    /// comparison will be exact if `casesensitive` is true, otherwise in a
    /// case-insensitive manner if `caseinsensitive` is false.
    ParamValue * find_attribute (string_view name,
                                 TypeDesc searchtype=TypeDesc::UNKNOWN,
                                 bool casesensitive=false);
    const ParamValue *find_attribute (string_view name,
                                      TypeDesc searchtype=TypeDesc::UNKNOWN,
                                      bool casesensitive=false) const;

    /// Search for the named attribute and return the pointer to its
    /// `ParamValue` record, or NULL if not found.  This variety of
    /// `find_attribute(}` can retrieve items such as "width", which are
    /// data members of the `ImageSpec`, but not in `extra_attribs`. The
    /// `tmpparam` is a storage area owned by the caller, which is used as
    /// temporary buffer in cases where the information does not correspond
    /// to an actual `extra_attribs` (in this case, the return value will be
    /// `&tmpparam`). The extra names it understands are:
    ///
    /// - `"x"` `"y"` `"z"` `"width"` `"height"` `"depth"`
    ///   `"full_x"` `"full_y"` `"full_z"` `"full_width"` `"full_height"` `"full_depth"`
    ///
    ///   Returns the `ImageSpec` fields of those names (despite the
    ///   fact that they are technically not arbitrary named attributes
    ///   in `extra_attribs`). All are of type `int`.
    ///
    /// - `"datawindow"`
    ///
    ///   Without a type, or if requested explicitly as an `int[4]`,
    ///   returns the OpenEXR-like pixel data min and max coordinates,
    ///   as a 4-element integer array: `{ x, y, x+width-1, y+height-1
    ///   }`. If instead you specifically request as an `int[6]`, it
    ///   will return the volumetric data window, `{ x, y, z, x+width-1,
    ///   y+height-1, z+depth-1 }`.
    ///
    /// - `"displaywindow"`
    ///
    ///   Without a type, or if requested explicitly as an `int[4]`,
    ///   returns the OpenEXR-like pixel display min and max
    ///   coordinates, as a 4-element integer array: `{ full_x, full_y,
    ///   full_x+full_width-1, full_y+full_height-1 }`. If instead you
    ///   specifically request as an `int[6]`, it will return the
    ///   volumetric display window, `{ full_x, full_y, full_z,
    ///   full_x+full_width-1, full_y+full_height-1, full_z+full_depth-1 }`.
    ///
    /// EXAMPLES
    ///
    ///     ImageSpec spec;           // has the info
    ///     Imath::Box2i dw;          // we want the displaywindow here
    ///     ParamValue tmp;           // so we can retrieve pseudo-values
    ///     TypeDesc int4("int[4]");  // Equivalent: TypeDesc int4(TypeDesc::INT,4);
    ///     const ParamValue* p = spec.find_attribute ("displaywindow", int4);
    ///     if (p)
    ///         dw = Imath::Box2i(p->get<int>(0), p->get<int>(1),
    ///                           p->get<int>(2), p->get<int>(3));
    ///
    ///     p = spec.find_attribute("temperature", TypeFloat);
    ///     if (p)
    ///         float temperature = p->get<float>();
    ///
    const ParamValue * find_attribute (string_view name,
                         ParamValue &tmpparam,
                         TypeDesc searchtype=TypeDesc::UNKNOWN,
                         bool casesensitive=false) const;

    /// If the named attribute can be found in the `ImageSpec`, return its
    /// data type. If no such attribute exists, return `TypeUnknown`.
    ///
    /// This was added in version 2.1.
    TypeDesc getattributetype (string_view name,
                               bool casesensitive = false) const;

    /// If the `ImageSpec` contains the named attribute and its type matches
    /// `type`, copy the attribute value into the memory pointed to by `val`
    /// (it is up to the caller to ensure there is enough space) and return
    /// `true`. If no such attribute is found, or if it doesn't match the
    /// type, return `false` and do not modify `val`.
    ///
    /// EXAMPLES:
    ///
    ///     ImageSpec spec;
    ///     ...
    ///     // Retrieving an integer attribute:
    ///     int orientation = 0;
    ///     spec.getattribute ("orientation", TypeInt, &orientation);
    ///
    ///     // Retrieving a string attribute with a char*:
    ///     const char* compression = nullptr;
    ///     spec.getattribute ("compression", TypeString, &compression);
    ///
    ///     // Alternately, retrieving a string with a ustring:
    ///     ustring compression;
    ///     spec.getattribute ("compression", TypeString, &compression);
    ///
    /// Note that when passing a string, you need to pass a pointer to the
    /// `char*`, not a pointer to the first character.  Also, the `char*`
    /// will end up pointing to characters owned by the `ImageSpec`; the
    /// caller does not need to ever free the memory that contains the
    /// characters.
    ///
    /// This was added in version 2.1.
    bool getattribute (string_view name, TypeDesc type, void* value,
                       bool casesensitive = false) const;

    /// Retrieve the named metadata attribute and return its value as an
    /// `int`. Any integer type will convert to `int` by truncation or
    /// expansion, string data will parsed into an `int` if its contents
    /// consist of of the text representation of one integer. Floating point
    /// data will not succeed in converting to an `int`. If no such metadata
    /// exists, or are of a type that cannot be converted, the `defaultval`
    /// will be returned.
    int get_int_attribute (string_view name, int defaultval=0) const;

    /// Retrieve the named metadata attribute and return its value as a
    /// `float`. Any integer or floating point type will convert to `float`
    /// in the obvious way (like a C cast), and so will string metadata if
    /// its contents consist of of the text representation of one floating
    /// point value. If no such metadata exists, or are of a type that cannot
    /// be converted, the `defaultval` will be returned.
    float get_float_attribute (string_view name, float defaultval=0) const;

    /// Retrieve any metadata attribute, converted to a string.
    /// If no such metadata exists, the `defaultval` will be returned.
    string_view get_string_attribute (string_view name,
                           string_view defaultval = string_view()) const;

    /// For a given parameter `p`, format the value nicely as a string.  If
    /// `human` is true, use especially human-readable explanations (units,
    /// or decoding of values) for certain known metadata.
    static std::string metadata_val (const ParamValue &p, bool human=false);

    enum SerialFormat  { SerialText, SerialXML };
    enum SerialVerbose { SerialBrief, SerialDetailed, SerialDetailedHuman };

    /// Returns, as a string, a serialized version of the `ImageSpec`. The
    /// `format` may be either `ImageSpec::SerialText` or
    /// `ImageSpec::SerialXML`. The `verbose` argument may be one of:
    /// `ImageSpec::SerialBrief` (just resolution and other vital
    /// statistics, one line for `SerialText`, `ImageSpec::SerialDetailed`
    /// (contains all metadata in original form), or
    /// `ImageSpec::SerialDetailedHuman` (contains all metadata, in many
    /// cases with human-readable explanation).
    std::string serialize (SerialFormat format,
                           SerialVerbose verbose = SerialDetailed) const;

    /// Converts the contents of the `ImageSpec` as an XML string.
    std::string to_xml () const;

    /// Populates the fields of the `ImageSpec` based on the XML passed in.
    void from_xml (const char *xml);

    /// Hunt for the "Compression" and "CompressionQuality" settings in the
    /// spec and turn them into the compression name and quality. This
    /// handles compression name/qual combos of the form "name:quality".
    std::pair<string_view, int>
    decode_compression_metadata(string_view defaultcomp = "",
                                int defaultqual = -1) const;

    /// Helper function to verify that the given pixel range exactly covers a
    /// set of 2D tiles.  Also returns false if the spec indicates that the
    /// image isn't tiled at all.
    bool valid_tile_range (int xbegin, int xend, int ybegin, int yend) noexcept {
        return (tile_width &&
                ((xbegin-x) % tile_width)  == 0 &&
                ((ybegin-y) % tile_height) == 0 &&
                (((xend-x) % tile_width)  == 0 || (xend-x) == width) &&
                (((yend-y) % tile_height) == 0 || (yend-y) == height));
    }

    /// Helper function to verify that the given pixel range exactly covers a
    /// set of 3D tiles.  Also returns false if the spec indicates that the
    /// image isn't tiled at all.
    bool valid_tile_range (int xbegin, int xend, int ybegin, int yend,
                           int zbegin, int zend) noexcept {
        return (tile_width &&
                ((xbegin-x) % tile_width)  == 0 &&
                ((ybegin-y) % tile_height) == 0 &&
                ((zbegin-z) % tile_depth)  == 0 &&
                (((xend-x) % tile_width)  == 0 || (xend-x) == width) &&
                (((yend-y) % tile_height) == 0 || (yend-y) == height) &&
                (((zend-z) % tile_depth)  == 0 || (zend-z) == depth));
    }

    /// Return the channelformat of the given channel. This is safe even
    /// if channelformats is not filled out.
    TypeDesc channelformat (int chan) const {
        return chan >= 0 && chan < (int)channelformats.size()
            ? channelformats[chan] : format;
    }

    /// Return the channel name of the given channel. This is safe even if
    /// channelnames is not filled out.
    string_view channel_name (int chan) const {
        return chan >= 0 && chan < (int)channelnames.size()
            ? string_view(channelnames[chan]) : "";
    }

    /// Fill in an array of channel formats describing all channels in
    /// the image.  (Note that this differs slightly from the member
    /// data channelformats, which is empty if there are not separate
    /// per-channel formats.)
    void get_channelformats (std::vector<TypeDesc> &formats) const {
        formats = channelformats;
        if ((int)formats.size() < nchannels)
            formats.resize (nchannels, format);
    }

    /// Return the index of the channel with the given name, or -1 if no
    /// such channel is present in `channelnames`.
    int channelindex (string_view name) const;

    /// Return pixel data window for this ImageSpec expressed as a ROI.
    ROI roi () const noexcept {
        return ROI (x, x+width, y, y+height, z, z+depth, 0, nchannels);
    }

    /// Return full/display window for this ImageSpec expressed as a ROI.
    ROI roi_full () const noexcept {
        return ROI (full_x, full_x+full_width, full_y, full_y+full_height,
                    full_z, full_z+full_depth, 0, nchannels);
    }

    /// Set pixel data window parameters (x, y, z, width, height, depth)
    /// for this ImageSpec from an ROI.
    /// Does NOT change the channels of the spec, regardless of r.
    void set_roi (const ROI &r) noexcept {
        x = r.xbegin;
        y = r.ybegin;
        z = r.zbegin;
        width = r.width();
        height = r.height();
        depth = r.depth();
    }

    /// Set full/display window parameters (full_x, full_y, full_z,
    /// full_width, full_height, full_depth) for this ImageSpec from an ROI.
    /// Does NOT change the channels of the spec, regardless of r.
    void set_roi_full (const ROI &r) noexcept {
        full_x = r.xbegin;
        full_y = r.ybegin;
        full_z = r.zbegin;
        full_width = r.width();
        full_height = r.height();
        full_depth = r.depth();
    }

    /// Copy from `other` the image dimensions (x, y, z, width, height,
    /// depth, full*, nchannels, format) and data types. It does *not* copy
    /// arbitrary named metadata or channel names (thus, for an `ImageSpec`
    /// with lots of metadata, it is much less expensive than copying the
    /// whole thing with `operator=()`).
    void copy_dimensions (const ImageSpec &other) {
        x = other.x;
        y = other.y;
        z = other.z;
        width = other.width;
        height = other.height;
        depth = other.depth;
        full_x = other.full_x;
        full_y = other.full_y;
        full_z = other.full_z;
        full_width = other.full_width;
        full_height = other.full_height;
        full_depth = other.full_depth;
        tile_width = other.tile_width;
        tile_height = other.tile_height;
        tile_depth = other.tile_depth;
        nchannels = other.nchannels;
        format = other.format;
        channelformats = other.channelformats;
        alpha_channel = other.alpha_channel;
        z_channel = other.z_channel;
        deep = other.deep;
    }

    /// Set the metadata to presume that color space is `name` (or to assume
    /// nothing about the color space if `name` is empty). The core operation
    /// is to set the "oiio:ColorSpace" attribute, but it also removes or
    /// alters several other attributes that may hint color space in ways that
    /// might be contradictory or no longer true.
    ///
    /// @version 2.5
    void set_colorspace(string_view name);

    /// Returns `true` for a newly initialized (undefined) `ImageSpec`.
    /// (Designated by no channels and undefined data type -- true of the
    /// uninitialized state of an ImageSpec, and presumably not for any
    /// ImageSpec that is useful or purposefully made.)
    bool undefined () const noexcept {
        return nchannels == 0 && format == TypeUnknown;
    }

    /// Array indexing by string will create an AttrDelegate that enables a
    /// convenient shorthand for adding and retrieving values from the spec:
    ///
    /// 1. Assigning to the delegate adds a metadata attribute:
    ///
    ///        ImageSpec spec;
    ///        spec["foo"] = 42;                   // int
    ///        spec["pi"] = float(M_PI);           // float
    ///        spec["oiio:ColorSpace"] = "sRGB";   // string
    ///        spec["cameratoworld"] = Imath::Matrix44(...);  // matrix
    ///
    ///    Be very careful, the attribute's type will be implied by the C++
    ///    type of what you assign.
    ///
    /// 2. String data may be retrieved directly, and for other types, the
    ///    delegate supports a get<T>() that retrieves an item of type T:
    ///
    ///         std::string colorspace = spec["oiio:ColorSpace"];
    ///         int dither = spec["oiio:dither"].get<int>();
    ///
    /// This was added in version 2.1.
    AttrDelegate<ImageSpec> operator[](string_view name)
    {
        return { this, name };
    }
    AttrDelegate<const ImageSpec> operator[](string_view name) const
    {
        return { this, name };
    }
};




/// ImageInput abstracts the reading of an image file in a file
/// format-agnostic manner.
class OIIO_API ImageInput {
public:
    /// unique_ptr to an ImageInput
    using unique_ptr = std::unique_ptr<ImageInput>;

    /// @{
    /// @name Creating an ImageInput

    /// Create an ImageInput subclass instance that is able to read the
    /// given file and open it, returning a `unique_ptr` to the ImageInput
    /// if successful.  The `unique_ptr` is set up with an appropriate
    /// deleter so the ImageInput will be properly closed and deleted when
    /// the `unique_ptr` goes out of scope or is reset. If the open fails,
    /// return an empty `unique_ptr` and set an error that can be retrieved
    /// by `OIIO::geterror()`.
    ///
    /// The `config`, if not nullptr, points to an ImageSpec giving hints,
    /// requests, or special instructions.  ImageInput implementations are
    /// free to not respond to any such requests, so the default
    /// implementation is just to ignore `config`.
    ///
    /// `open()` will first try to make an ImageInput corresponding to
    /// the format implied by the file extension (for example, `"foo.tif"`
    /// will try the TIFF plugin), but if one is not found or if the
    /// inferred one does not open the file, every known ImageInput type
    /// will be tried until one is found that will open the file.
    ///
    /// @param filename
    ///         The name of the file to open, UTF-8 encoded.
    ///
    /// @param config
    ///         Optional pointer to an ImageSpec whose metadata contains
    ///         "configuration hints."
    ///
    /// @param ioproxy
    ///         Optional pointer to an IOProxy to use (not supported by all
    ///         formats, see `supports("ioproxy")`). The caller retains
    ///         ownership of the proxy.
    ///
    /// @returns
    ///         A `unique_ptr` that will close and free the ImageInput when
    ///         it exits scope or is reset. The pointer will be empty if the
    ///         required writer was not able to be created.
    static unique_ptr open (const std::string& filename,
                            const ImageSpec *config = nullptr,
                            Filesystem::IOProxy* ioproxy = nullptr);

    /// Create and open an ImageInput using a UTF-16 encoded wstring filename.
    static unique_ptr open (const std::wstring& filename,
                            const ImageSpec *config = nullptr,
                            Filesystem::IOProxy* ioproxy = nullptr) {
        return open(Strutil::utf16_to_utf8(filename), config, ioproxy);
    }

    /// Create and return an ImageInput implementation that is able to read
    /// the given file or format.  If `do_open` is true (and the `filename`
    /// is the name of a file, not just a format), fully open it if possible
    /// (using the optional `config` configuration spec, if supplied),
    /// otherwise just create the ImageInput but don't open it. The
    /// plugin_searchpath parameter is an override of the searchpath.
    /// colon-separated list of directories to search for ImageIO plugin
    /// DSO/DLL's (not a searchpath for the image itself!).
    ///
    /// If the `filename` parameter is the name of a file format (such as
    /// "openexr"), it will create an ImageInput that reads that particular
    /// format. If the name is a file extension (such as "exr" or ".exr"),
    /// it will guess the file format from the extension and return that
    /// type of ImageInput.
    ///
    /// If `filename` is a full file name (such as "hawaii.exr"), it will
    /// create an ImageInput that reads the format implied by the file
    /// extension (".tif") and try to open the file with that reader. If the
    /// file can be opened and appears to be of the correct type, then that
    /// ImageInput (after being closed) will be returned to the caller. But
    /// if it fails (say, because the file type does not match the
    /// extension), then every known kind of image reader will be tried in
    /// turn, until one can be found that succeeds in opening that file. The
    /// `create()` file will fail entirely only if no known image reader
    /// type succeeds.
    ///
    /// If the caller intends to immediately open the file, then it is often
    /// simpler to call static `ImageInput::open()`.
    ///
    /// @param filename
    ///         The name of an image file, or a file extension, or the name
    ///         of a file format. The filename is UTF-8 encoded.
    ///
    /// @param do_open
    ///         If `true`, not only create but also open the file.
    ///
    /// @param config
    ///         Optional pointer to an ImageSpec whose metadata contains
    ///         "configuration hints" for the ImageInput implementation.
    ///
    /// @param ioproxy
    ///         Optional pointer to an IOProxy to use (not supported by all
    ///         formats, see `supports("ioproxy")`). The caller retains
    ///         ownership of the proxy. If this is not supplied, it is still
    ///         possible to set the proxy with a call to `set_proxy()` prior
    ///         to `open()`.
    ///
    /// @param plugin_searchpath
    ///         An optional colon-separated list of directories to search
    ///         for OpenImageIO plugin DSO/DLL's.
    ///
    /// @returns
    ///         A `unique_ptr` that will close and free the ImageInput when
    ///         it exits scope or is reset. The pointer will be empty if the
    ///         required writer was not able to be created.
    static unique_ptr create (string_view filename, bool do_open=false,
                              const ImageSpec *config=nullptr,
                              Filesystem::IOProxy* ioproxy = nullptr,
                              string_view plugin_searchpath = "");

    /// Create an ImageInput using a UTF-16 encoded wstring filename.
    static unique_ptr create (const std::wstring& filename, bool do_open=false,
                              const ImageSpec *config=nullptr,
                              Filesystem::IOProxy* ioproxy = nullptr,
                              string_view plugin_searchpath = "") {
        return create(Strutil::utf16_to_utf8(filename), do_open, config,
                      ioproxy, plugin_searchpath);
    }

    /// @}

protected:
    ImageInput ();
public:
    virtual ~ImageInput ();

    /// Return the name of the format implemented by this class.
    virtual const char *format_name (void) const = 0;

    /// Given the name of a "feature", return whether this ImageInput
    /// supports output of images with the given properties. Most queries
    /// will simply return 0 for "doesn't support" and 1 for "supports it,"
    /// but it is acceptable to have queries return other nonzero integers
    /// to indicate varying degrees of support or limits (but should be
    /// clearly documented as such).
    ///
    /// Feature names that ImageInput implementations are expected to
    /// recognize include:
    ///
    /// - `"arbitrary_metadata"` : Does this format allow metadata with
    ///       arbitrary names and types?
    ///
    /// - `"exif"` :
    ///       Can this format store Exif camera data?
    ///
    /// - `"ioproxy"` :
    ///       Does this format reader support reading from an `IOProxy`?
    ///
    /// - `"iptc"` :
    ///       Can this format store IPTC data?
    ///
    /// - `"procedural"` :
    ///       Can this format create images without reading from a disk
    ///       file?
    ///
    /// - `"thumbnail"` :
    ///       Does this format reader support retrieving a reduced
    ///       resolution copy of the image via the `thumbnail()` method?
    ///
    ///  - `"multiimage"` :
    ///       Does this format support multiple subimages within a file?
    ///       (Note: this doesn't necessarily mean that the particular
    ///       file this ImageInput is reading has multiple subimages.)
    ///
    ///  - `"mipmap"` :
    ///       Does this format support multiple resolutions for an
    ///       image/subimage? (Note: this doesn't necessarily mean that the
    ///       particular file this ImageInput is reading is MIP-mapped.)
    ///       This query was added in OpenImageIO 3.1.
    ///
    ///  - `"noimage"` :
    ///        Does this format allow 0x0 sized images, i.e. an image file
    ///        with metadata only and no pixels?
    ///
    /// This list of queries may be extended in future releases. Since this
    /// can be done simply by recognizing new query strings, and does not
    /// require any new API entry points, addition of support for new
    /// queries does not break ``link compatibility'' with
    /// previously-compiled plugins.
    virtual int supports (string_view feature OIIO_MAYBE_UNUSED) const {
        return false;
    }

    /// Return true if the `filename` names a file of the type for this
    /// ImageInput.  The implementation will try to determine this as
    /// efficiently as possible, in most cases much less expensively than
    /// doing a full `open()`.  Note that there can be false positives: a
    /// file can appear to be of the right type (i.e., `valid_file()`
    /// returning `true`) but still fail a subsequent call to `open()`, such
    /// as if the contents of the file are truncated, nonsensical, or
    /// otherwise corrupted. The filename is UTF-8 encoded.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure.
    virtual bool valid_file (const std::string& filename) const;

    /// Check valid file using a UTF-16 encoded wstring filename.
    bool valid_file (const std::wstring& filename) const {
        return valid_file(Strutil::utf16_to_utf8(filename));
    }

    /// Return true if the `ioproxy` represents a file of the type for this
    /// ImageInput.  The implementation will try to determine this as
    /// efficiently as possible, in most cases much less expensively than
    /// doing a full `open()`.  Note that there can be false positives: a
    /// file can appear to be of the right type (i.e., `valid_file()`
    /// returning `true`) but still fail a subsequent call to `open()`, such
    /// as if the contents of the file are truncated, nonsensical, or
    /// otherwise corrupted.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure.
    virtual bool valid_file (Filesystem::IOProxy* ioproxy) const;

    /// Opens the file with given name and seek to the first subimage in the
    /// file.  Various file attributes are put in `newspec` and a copy
    /// is also saved internally to the `ImageInput` (retrievable via
    /// `spec()`.  From examining `newspec` or `spec()`, you can
    /// discern the resolution, if it's tiled, number of channels, native
    /// data format, and other metadata about the image.
    ///
    /// @param name
    ///         Filename to open, UTF-8 encoded.
    ///
    /// @param newspec
    ///         Reference to an ImageSpec in which to deposit a full
    ///         description of the contents of the first subimage of the
    ///         file.
    ///
    /// @returns
    ///         `true` if the file was found and opened successfully.
    virtual bool open (const std::string& name, ImageSpec &newspec) = 0;

    /// Open the ImageInput using a UTF-16 encoded wstring filename.
    bool open (const std::wstring& name, ImageSpec &newspec) {
        return open(Strutil::utf16_to_utf8(name), newspec);
    }

    /// Open file with given name, similar to `open(name,newspec)`. The
    /// `config` is an ImageSpec giving requests or special instructions.
    /// ImageInput implementations are free to not respond to any such
    /// requests, so the default implementation is just to ignore config and
    /// call regular `open(name,newspec)`.
    ///
    /// @param name
    ///         Filename to open, UTF-8 encoded.
    ///
    /// @param newspec
    ///         Reference to an ImageSpec in which to deposit a full
    ///         description of the contents of the first subimage of the
    ///         file.
    ///
    /// @param config
    ///         An ImageSpec whose metadata contains "configuration hints"
    ///         for the ImageInput implementation.
    ///
    /// @returns
    ///         `true` if the file was found and opened successfully.
    virtual bool open (const std::string& name, ImageSpec &newspec,
                       const ImageSpec& config OIIO_MAYBE_UNUSED) {
        return open(name,newspec);
    }
    /// Open the ImageInput using a UTF-16 encoded wstring filename.
    bool open (const std::wstring& name, ImageSpec &newspec,
               const ImageSpec& config OIIO_MAYBE_UNUSED) {
        return open(name,newspec);
    }

    /// Return a reference to the image specification of the current
    /// subimage/MIPlevel.  Note that the contents of the spec are invalid
    /// before `open()` or after `close()`, and may change with a call to
    /// `seek_subimage()`. It is thus not thread-safe, since the spec may
    /// change if another thread calls `seek_subimage`, or any of the
    /// `read_*()` functions that take explicit subimage/miplevel.
    ///
    /// This method should be considered deprecated, and we advise
    /// always using the thread-safe `spec(subimage, miplevel)` method
    /// instead.
    virtual const ImageSpec &spec (void) const { return m_spec; }

    /// Return a full copy of the ImageSpec of the designated subimage and
    /// MIPlevel. This method is thread-safe, but it is potentially
    /// expensive, due to the work that needs to be done to fully copy an
    /// ImageSpec if there is lots of named metadata to allocate and copy.
    /// See also the less expensive `spec_dimensions()`. Errors (such as
    /// having requested a nonexistent subimage) are indicated by returning
    /// an ImageSpec with `format==TypeUnknown`, but does not call
    /// errorfmt() to set an error message merely for being an out-of-range
    /// subimage or miplevel.
    virtual ImageSpec spec (int subimage, int miplevel=0);

    /// Return a copy of the ImageSpec of the designated subimage and
    /// miplevel, but only the dimension and type fields. Just as with a
    /// call to `ImageSpec::copy_dimensions()`, neither the channel names
    /// nor any of the arbitrary named metadata will be copied, thus this is
    /// a relatively inexpensive operation if you don't need that
    /// information. It is guaranteed to be thread-safe. Errors (such as
    /// having requested a nonexistent subimage) are indicated by returning
    /// an ImageSpec with `format==TypeUnknown`, but does not call
    /// errorfmt() to set an error message merely for being an out-of-range
    /// subimage or miplevel.
    virtual ImageSpec spec_dimensions (int subimage, int miplevel=0);

    /// Retrieve a reduced-resolution ("thumbnail") version of the given
    /// subimage. It is guaranteed to be thread-safe.
    ///
    /// @param thumb
    ///         A reference to an `ImageBuf` which will be overwritten with
    ///         the thumbnail image.
    /// @param subimage
    ///         The index of the subimage in the file whose thumbnail is to
    ///         be retrieved.
    /// @returns
    ///         `true` upon success, `false` if no thumbnail was available,
    ///         or if this file format (or reader) does not support
    ///         thumbnails.
    ///
    /// @note This method was added to OpenImageIO 2.3.
    virtual bool get_thumbnail(ImageBuf& thumb, int subimage) {
        return false;
    }

    /// Close an open ImageInput. The call to close() is not strictly
    /// necessary if the ImageInput is destroyed immediately afterwards,
    /// since it is required for the destructor to close if the file is
    /// still open.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure.
    virtual bool close () = 0;

    /// Returns the index of the subimage that is currently being read.
    /// The first subimage (or the only subimage, if there is just one)
    /// is number 0.
    virtual int current_subimage (void) const { return 0; }

    /// Returns the index of the MIPmap image that is currently being read.
    /// The highest-res MIP level (or the only level, if there is just
    /// one) is number 0.
    virtual int current_miplevel (void) const { return 0; }

    /// Seek to the given subimage and MIP-map level within the open image
    /// file.  The first subimage of the file has index 0, the highest-
    /// resolution MIP level has index 0.  The new subimage's vital
    /// statistics may be retrieved by `this->spec()`.  The reader is
    /// expected to give the appearance of random access to subimages and
    /// MIP levels -- in other words, if it can't randomly seek to the given
    /// subimage/level, it should transparently close, reopen, and
    /// sequentially read through prior subimages and levels.
    ///
    /// Inability to seek to an out-of-range subimage or miplevel is indicated
    /// by returning false, but it does not call errorfmt() to set an error
    /// message unless it's the result of a damaged file.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure. A failure may
    ///         indicate that no such subimage or MIP level exists in the
    ///         file.
    virtual bool seek_subimage (int subimage, int miplevel) {
        // Default implementation assumes no support for subimages or
        // mipmaps, so there is no work to do.
        return subimage == current_subimage() && miplevel == current_miplevel();
    }

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2,0,0) && !defined(OIIO_DOXYGEN)
    // Old version for backwards-compatibility: pass reference to newspec.
    // Some day this will be deprecated.
    OIIO_DEPRECATED("Prefer the version that doesn't take an ImageSpec argument (2.0)")
    bool seek_subimage (int subimage, int miplevel, ImageSpec &newspec) {
        bool ok = seek_subimage (subimage, miplevel);
        if (ok)
            newspec = spec();
        return ok;
    }
#endif

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2,1,0) && !defined(OIIO_DOXYGEN)
    // Seek to the given subimage -- backwards-compatible call that
    // doesn't worry about MIP-map levels at all.
    OIIO_DEPRECATED("Prefer the version that takes a mipmap argument (2.1)")
    bool seek_subimage (int subimage, ImageSpec &newspec) {
        bool ok = seek_subimage (subimage, 0 /* miplevel */);
        if (ok)
            newspec = spec(subimage);
        return ok;
    }
#endif

    // clang-format on

    /// @name Reading pixels ("safe" methods with bounded spans)
    ///
    /// Common features of all the `read` methods:
    ///
    /// * There is a base case that takes a `image_span<byte>` describing
    ///   untyped memory layout and a `TypeDesc` describing the data type
    ///   that the values should be converted to (or TypeUnknown to keep
    ///   the data in its "native" file types with no conversion).
    ///
    /// * The type-aware versions that accept an `image_span<T>` (for
    ///   potentially non-contiguous data) or `span<T>` (for contiguous data)
    ///   and understand to convert the data into the given `T` type.
    ///
    /// * The image_span (in either case) includes the memory bounds and
    ///   stride lengths (in bytes) between channels, pixels, scanlines, and
    ///   volumetric slices.
    ///
    /// * Any *range* parameters (such as `ybegin` and `yend`) describe a
    ///   "half open interval", meaning that `begin` is the first item and
    ///   `end` is *one past the last item*. That means that the number of
    ///   items is `end - begin`.
    ///
    /// * For ordinary 2D (non-volumetric) images, any `z` or `zbegin`
    ///   coordinates should be 0 and any `zend` should be 1, indicating
    ///   that only a single image "plane" exists.
    ///
    /// * Some read methods take a channel range [chbegin,chend) to allow
    ///   reading of a contiguous subset of channels (chbegin=0,
    ///   chend=spec.nchannels reads all channels).
    ///
    /// * ImageInput readers are expected to give the appearance of random
    ///   access -- in other words, if it can't randomly seek to the given
    ///   scanline or tile, it should transparently close, reopen, and
    ///   sequentially read through prior scanlines.
    ///
    /// * All read functions return `true` for success, `false` for failure
    ///   (after which a call to `geterror()` may retrieve a specific error
    ///   message).
    ///

    /// Read the entire image of `spec.width x spec.height x spec.depth`
    /// pixels into a buffer with the given strides and in the desired
    /// data format.
    ///
    /// Depending on the spec, this will read either all tiles or all
    /// scanlines. Assume that data points to a layout in row-major order.
    ///
    /// This version of read_image, because it passes explicit subimage and
    /// miplevel, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any
    /// of the read_* methods that take an explicit subimage/miplevel (but
    /// not against any other ImageInput methods).
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_image that takes raw pointers.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  chbegin/chend
    ///                     The channel range to read. If chend is -1, it
    ///                     will automatically be set to spec.nchannels.
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that you want the native data format
    ///                     with no type conversion.
    /// @param  data        An `image_span<byte>` describing the memory
    ///                     extent of the data buffer and including the sizes
    ///                     and byte strides for each dimension (channel, x,
    ///                     y, and z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    virtual bool read_image(int subimage, int miplevel, int chbegin, int chend,
                            TypeDesc format, const image_span<std::byte>& data);

    /// A version of `read_image()` taking an `image_span<T>`, where the type
    /// of the underlying data is `T`.  This is a convenience wrapper around
    /// the `read_image()` that takes an `image_span<std::byte>`.
    template<typename T>
    bool read_image(int subimage, int miplevel, int chbegin, int chend,
                    const image_span<T>& data)
    {
        static_assert(!std::is_const_v<T>,
                      "read_image() does not accept image_span<const T>");
        return read_image(subimage, miplevel, chbegin, chend,
                          TypeDescFromC<T>::value(),
                          as_image_span_writable_bytes(data));
    }

    /// A version of `read_image()` taking a `span<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `read_image()` that takes an `image_span<T>`.
    template<typename T>
    bool read_image(int subimage, int miplevel, int chbegin, int chend,
                    span<T> data)
    {
        static_assert(!std::is_const_v<T>,
                      "read_image() does not accept span<const T>");
        ImageSpec spec = spec_dimensions(subimage, miplevel);
        if (chend < 0 || chend > spec.nchannels)
            chend = spec.nchannels;
        auto ispan = image_span<T>(data.data(), chend - chbegin, spec.width,
                                   spec.height, spec.depth);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return read_image(subimage, miplevel, chbegin, chend, ispan);
    }

    /// Read multiple scanlines that include pixels (*,y) for all ybegin <= y
    /// < yend in the specified subimage and mip level, into `data`, using the
    /// strides given and converting to the requested data `format`
    /// (TypeUnknown indicates no conversion, just copy native data types).
    /// Only channels [chbegin,chend) will be read/copied (chbegin=0,
    /// chend=nchannels reads all channels, and chend=-1 also means the same
    /// thing).
    ///
    /// This method, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any of
    /// the read_* methods that take an explicit subimage/miplevel (but not
    /// against any other non-thread-safe ImageInput methods).
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  ybegin/yend The y range of the scanlines to read.
    /// @param  chbegin/chend
    ///                     The channel range to read. If chend is -1, it
    ///                     will automatically be set to spec.nchannels.
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that you want the native data format
    ///                     with no type conversion.
    /// @param  data        An `image_span<byte>` describing the memory
    ///                     extent of the data buffer and including the sizes
    ///                     and byte strides for each dimension (channel, x,
    ///                     y, and z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    ///
    virtual bool read_scanlines(int subimage, int miplevel, int ybegin,
                                int yend, int chbegin, int chend,
                                TypeDesc format,
                                const image_span<std::byte>& data);

    /// A version of `read_scanlines()` taking an `image_span<T>`, where the
    /// type of the underlying data is `T`.  This is a convenience wrapper
    /// around the `read_scanlines()` that takes an `image_span<std::byte>`.
    template<typename T>
    bool read_scanlines(int subimage, int miplevel, int ybegin, int yend,
                        int chbegin, int chend, const image_span<T>& data)
    {
        static_assert(!std::is_const_v<T>,
                      "read_scanlines() does not accept span<const T>");
        // reduce to type + image_span<byte>
        return read_scanlines(subimage, miplevel, ybegin, yend, chbegin, chend,
                              TypeDescFromC<T>::value(),
                              as_image_span_writable_bytes(data));
    }

    /// A version of `read_scanlines()` taking a `span<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `read_scanlines()` that takes an `image_span<T>`.
    template<typename T>
    bool read_scanlines(int subimage, int miplevel, int ybegin, int yend,
                        int chbegin, int chend, span<T> data)
    {
        static_assert(!std::is_const_v<T>,
                      "read_scanlines() does not accept span<const T>");
        // reduce to type + image_span<byte>
        ImageSpec spec = spec_dimensions(subimage, miplevel);
        if (chend < 0 || chend > spec.nchannels)
            chend = spec.nchannels;
        auto ispan = image_span<T>(data.data(), chend - chbegin, spec.width,
                                   spec.height, 1);
        return read_scanlines(subimage, miplevel, ybegin, yend, chbegin, chend,
                              ispan);
    }

    /// Read the block of multiple tiles that include all pixels in
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size.
    ///
    /// This version of read_tiles, because it passes explicit subimage and
    /// miplevel, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any
    /// of the read_* methods that take an explicit subimage/miplevel (but
    /// not against any other ImageInput methods).
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles being read.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that you want the native data format
    ///                     with no type conversion.
    /// @param  data        An `image_span<byte>` describing the memory
    ///                     extent of the data buffer and including the sizes
    ///                     and byte strides for each dimension (channel, x,
    ///                     y, and z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_tiles that takes raw pointers.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool read_tiles(int subimage, int miplevel, int xbegin, int xend,
                            int ybegin, int yend, int zbegin, int zend,
                            int chbegin, int chend, TypeDesc format,
                            const image_span<std::byte>& data);

    /// A version of `read_tiles()` taking an `image_span<T>`, where the type
    /// of the underlying data is `T`.  This is a convenience wrapper around
    /// the `read_tiles()` that takes an `image_span<const std::byte>`.
    template<typename T>
    bool read_tiles(int subimage, int miplevel, int xbegin, int xend,
                    int ybegin, int yend, int zbegin, int zend, int chbegin,
                    int chend, const image_span<T>& data)
    {
        return read_tiles(subimage, miplevel, xbegin, xend, ybegin, yend,
                          zbegin, zend, chbegin, chend,
                          TypeDescFromC<T>::value(),
                          as_image_span_writable_bytes(data));
    }

    /// A version of `read_tiles()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `read_tiles()` that takes an `image_span<const T>`.
    template<typename T>
    bool read_tiles(int subimage, int miplevel, int xbegin, int xend,
                    int ybegin, int yend, int zbegin, int zend, int chbegin,
                    int chend, span<T> data)
    {
        static_assert(!std::is_const_v<T>,
                      "read_tiles() does not accept span<const T>");
        // reduce to type + image_span<byte>
        ImageSpec spec = spec_dimensions(subimage, miplevel);
        if (chend < 0 || chend > spec.nchannels)
            chend = spec.nchannels;
        auto ispan = image_span<T>(data.data(), chend - chbegin, xend - xbegin,
                                   yend - ybegin, zend - zbegin);
        OIIO_DASSERT(data.size_bytes() >= ispan.size_bytes()
                     && ispan.is_contiguous());
        return read_tiles(subimage, miplevel, xbegin, xend, ybegin, yend,
                          zbegin, zend, chbegin, chend, ispan);
    }

    // clang-format off

    /// Read deep scanlines containing pixels (*,y,z), for all y in the
    /// range [ybegin,yend) into `deepdata`. This will fail if it is not a
    /// deep file.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  deepdata    A `DeepData` object into which the data for
    ///                     these scanlines will be placed.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_native_deep_scanlines (int subimage, int miplevel,
                                             int ybegin, int yend, int z,
                                             int chbegin, int chend,
                                             DeepData &deepdata);

    /// Read into `deepdata` the block of native deep data tiles that
    /// include all pixels and channels specified by pixel range.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles being read.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  deepdata    A `DeepData` object into which the data for
    ///                     these tiles will be placed.
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool read_native_deep_tiles (int subimage, int miplevel,
                                         int xbegin, int xend,
                                         int ybegin, int yend,
                                         int zbegin, int zend,
                                         int chbegin, int chend,
                                         DeepData &deepdata);

    /// Read the entire deep data image of spec.width x spec.height x
    /// spec.depth pixels, all channels, into `deepdata`.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  deepdata    A `DeepData` object into which the data for
    ///                     the image will be placed.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_native_deep_image (int subimage, int miplevel,
                                         DeepData &deepdata);

    /// @}

    /// @{
    /// @name Reading pixels (unsafe methods with pointers and strides)
    ///
    /// These methods are the "unsafe" versions of the `read` methods, which
    /// take raw pointers and strides.  They are provided for backwards
    /// compatibility with older code, but the preferred interface is to use
    /// the `read` methods that take `image_span<T>` or `span<T>` arguments,
    /// which are bounds-safe and type-aware.
    ///
    /// These pointer-based versions are considered "soft-deprecated" in
    /// OpenImageIO 3.1, will be marked/warned as deprecated in 3.2, and will
    /// be removed in 4.0.
    ///
    /// Common features of all the `read` methods:
    ///
    /// * The `format` parameter describes the data type of the `data[]`
    ///   buffer. The read methods automatically convert the data from the
    ///   data type it is stored in the file into the `format` of the `data`
    ///   buffer.  If `format` is `TypeUnknown` it will just copy pixels of
    ///   file's native data layout (including, possibly, per-channel data
    ///   formats as specified by the ImageSpec's `channelformats` field).
    ///
    /// * The `stride` values describe the layout of the `data` buffer:
    ///   `xstride` is the distance in bytes between successive pixels
    ///   within each scanline. `ystride` is the distance in bytes between
    ///   successive scanlines. For volumetric images `zstride` is the
    ///   distance in bytes between successive "volumetric planes".  Strides
    ///   set to the special value `AutoStride` imply contiguous data, i.e.,
    ///
    ///       xstride = format.size() * nchannels
    ///       ystride = xstride * width
    ///       zstride = ystride * height
    ///
    /// * Any *range* parameters (such as `ybegin` and `yend`) describe a
    ///   "half open interval", meaning that `begin` is the first item and
    ///   `end` is *one past the last item*. That means that the number of
    ///   items is `end - begin`.
    ///
    /// * For ordinary 2D (non-volumetric) images, any `z` or `zbegin`
    ///   coordinates should be 0 and any `zend` should be 1, indicating
    ///   that only a single image "plane" exists.
    ///
    /// * Some read methods take a channel range [chbegin,chend) to allow
    ///   reading of a contiguous subset of channels (chbegin=0,
    ///   chend=spec.nchannels reads all channels).
    ///
    /// * ImageInput readers are expected to give the appearance of random
    ///   access -- in other words, if it can't randomly seek to the given
    ///   scanline or tile, it should transparently close, reopen, and
    ///   sequentially read through prior scanlines.
    ///
    /// * All read functions return `true` for success, `false` for failure
    ///   (after which a call to `geterror()` may retrieve a specific error
    ///   message).
    ///

    /// Read the scanline that includes pixels (*,y,z) from the "current"
    /// subimage and MIP level.  The `xstride` value gives the distance
    /// between successive pixels (in bytes).  Strides set to `AutoStride`
    /// imply "contiguous" data.
    ///
    /// @note This variety of `read_scanline` is not re-entrant nor
    /// thread-safe. If you require concurrent reads to the same open
    /// ImageInput, you should use `read_scanlines` that has the `subimage`
    /// and `miplevel` passed explicitly.
    ///
    /// @param  y/z         The y & z coordinates of the scanline. For 2D
    ///                     images, z should be 0.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data buffer.
    /// @param  xstride     The distance in bytes between successive
    ///                     pixels in `data` (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_scanline (int y, int z, TypeDesc format, void *data,
                                stride_t xstride=AutoStride);

    /// Simple read_scanline reads into contiguous float pixels.
    bool read_scanline (int y, int z, float *data) {
        return read_scanline (y, z, TypeFloat, data);
    }

    /// Read multiple scanlines that include pixels (*,y,z) for all ybegin
    /// <= y < yend in the specified subimage and mip level, into `data`,
    /// using the strides given and converting to the requested data
    /// `format` (TypeUnknown indicates no conversion, just copy native data
    /// types). Only channels [chbegin,chend) will be read/copied
    /// (chbegin=0, chend=spec.nchannels reads all channels, yielding
    /// equivalent behavior to the simpler variant of `read_scanlines`).
    ///
    /// This version of read_scanlines, because it passes explicit
    /// subimage/miplevel, does not require a separate call to
    /// seek_subimage, and is guaranteed to be thread-safe against other
    /// concurrent calls to any of the read_* methods that take an explicit
    /// subimage/miplevel (but not against any other ImageInput methods).
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride
    ///                     The distance in bytes between successive pixels
    ///                     and scanlines (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note This call was changed for OpenImageIO 2.0 to include the
    ///     explicit subimage and miplevel parameters.
    virtual bool read_scanlines (int subimage, int miplevel,
                                 int ybegin, int yend, int z,
                                 int chbegin, int chend,
                                 TypeDesc format, void *data,
                                 stride_t xstride=AutoStride,
                                 stride_t ystride=AutoStride);

    /// Read the tile whose upper-left origin is (x,y,z) into `data[]`,
    /// converting if necessary from the native data format of the file into
    /// the `format` specified. The stride values give the data spacing of
    /// adjacent pixels, scanlines, and volumetric slices (measured in
    /// bytes). Strides set to AutoStride imply 'contiguous' data in the
    /// shape of a full tile, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * spec.tile_width
    ///     zstride = ystride * spec.tile_height
    ///
    /// @note This variety of `read_tile` is not re-entrant nor thread-safe.
    /// If you require concurrent reads to the same open ImageInput, you
    /// should use `read_tiles()` that has the `subimage` and `miplevel`
    /// passed explicitly.
    ///
    /// @param  x/y/z       The upper left coordinate of the tile being passed.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride` to
    ///                     indicate a "contiguous" single tile).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note This call will fail if the image is not tiled, or if (x,y,z)
    /// is not the upper left corner coordinates of a tile.
    virtual bool read_tile (int x, int y, int z, TypeDesc format,
                            void *data, stride_t xstride=AutoStride,
                            stride_t ystride=AutoStride,
                            stride_t zstride=AutoStride);

    /// Simple read_tile reads into contiguous float pixels.
    bool read_tile (int x, int y, int z, float *data) {
        return read_tile (x, y, z, TypeDesc::FLOAT, data,
                          AutoStride, AutoStride, AutoStride);
    }

    /// Read the block of multiple tiles that include all pixels in
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// This is analogous to calling `read_tile(x,y,z,...)` for each tile
    /// in turn (but for some file formats, reading multiple tiles may allow
    /// it to read more efficiently or in parallel).
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size. The
    /// stride values give the data spacing of adjacent pixels, scanlines,
    /// and volumetric slices (measured in bytes). Strides set to AutoStride
    /// imply contiguous data in the shape of the [begin,end) region, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * (xend-xbegin)
    ///     zstride = ystride * (yend-ybegin)
    ///
    /// This version of read_tiles, because it passes explicit subimage and
    /// miplevel, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any
    /// of the read_* methods that take an explicit subimage/miplevel (but
    /// not against any other ImageInput methods).
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles being read.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool read_tiles (int subimage, int miplevel, int xbegin, int xend,
                             int ybegin, int yend, int zbegin, int zend,
                             int chbegin, int chend, TypeDesc format, void *data,
                             stride_t xstride=AutoStride, stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride);

    /// Read the entire image of `spec.width x spec.height x spec.depth`
    /// pixels into a buffer with the given strides and in the desired
    /// data format.
    ///
    /// Depending on the spec, this will read either all tiles or all
    /// scanlines. Assume that data points to a layout in row-major order.
    ///
    /// This version of read_image, because it passes explicit subimage and
    /// miplevel, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any
    /// of the read_* methods that take an explicit subimage/miplevel (but
    /// not against any other ImageInput methods).
    ///
    /// Because this may be an expensive operation, a progress callback
    /// may be passed.  Periodically, it will be called as follows:
    ///
    ///     progress_callback (progress_callback_data, float done);
    ///
    /// where `done` gives the portion of the image (between 0.0 and 1.0)
    /// that has been written thus far.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  chbegin/chend
    ///                     The channel range to read. If chend is -1, it
    ///                     will automatically be set to spec.nchannels.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @param  progress_callback/progress_callback_data
    ///                     Optional progress callback.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_image (int subimage, int miplevel,
                             int chbegin, int chend,
                             TypeDesc format, void *data,
                             stride_t xstride=AutoStride,
                             stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride,
                             ProgressCallback progress_callback=NULL,
                             void *progress_callback_data=NULL);

    /// @}

    /// @{
    /// @name Reading native pixels -- implementation overloads
    ///
    /// @note read_native_* methods are usually not directly called by user
    ///     code (except for read_native_deep_* varieties). These are the
    ///     methods that are overloaded by the ImageInput subclasses that
    ///     implement the individual file format readers.
    ///
    /// The read_native_* methods always read the "native" data types
    /// (including per-channel data types) and assume that `data` points to
    /// contiguous memory (no non-default strides). In contrast, the
    /// read_scanline/scanlines/tile/tiles handle data type translation and
    /// arbitrary strides.
    ///
    /// The read_native_* methods take an explicit subimage and miplevel, and
    /// thus do not require a prior call to seek_subimage (and therefore no
    /// saved state). They are all required to be thread-safe when called
    /// concurrently with any other read_native_* call or with the varieties
    /// of read_tiles() that also takes an explicit subimage and miplevel
    /// parameter.
    ///
    /// As far as format-reading ImageInput subclasses are concerned, the
    /// only truly required overloads are read_native_scanline (always) and
    /// read_native_tile (only for formats that support tiles). The other
    /// varieties are special cases, for example if the particular format is
    /// able to efficiently read multiple scanlines or tiles at once, and if
    /// the subclass does not provide overloads, the base class
    /// implementation will be used instead, which is implemented by reducing
    /// the operation to multiple calls to read_scanline or read_tile.
    ///
    /// Note for ImageInput implementors:
    ///
    /// The preferred ImageInput implementation strategy (and, eventually,
    /// required) is to fully implement the span-based, all-channel verison of
    /// read_native_scanlines and/or read_native_tiles and/or
    /// read_native_volumetric_tiles (whichever of these are supported by the
    /// image file format), and to make a trivial implementation of the old
    /// pointer-based varieties that simply call the span ones (with an
    /// inferred length).
    ///
    /// The channel-subset versions may also, optionally, be implemented. If
    /// omitted, they will fall back to a default implementation that calls
    /// the all-channel versions to read into a temporary buffer, then copies
    /// the requested channels to the user's buffer. It is only worth custom
    /// implementations of the channel-subset versions for formats where there
    /// is a significant performance advantage due to the underlying library
    /// providing that as a feature (as OpenEXR does, for example, but most
    /// format reading APIs do not).
    ///
    /// For the time being, old ImageInput format readers that do not yet
    /// implement the span-based readers, but provide full featured
    /// implementations of the pointer-based varieties, will continue to work.
    /// This is because the default base-class implementation of the
    /// span-based read_native methods will, if not overloaded, simply call
    /// the pointer-based versions.
    ///
    /// To reiterate: A format-reader is expected to provide EITHER (a)
    /// span-based calls with full implementations and pointer-based calls
    /// that simply call the span-based ones, OR (b) pointer-based full
    /// implementations only and rely on the base class span-based calls to
    /// call them by default.

    /// Read a range of scanlines (all channels) of native data into
    /// contiguous memory defined by the span `data`.
    virtual bool read_native_scanlines(int subimage, int miplevel, int ybegin,
                                       int yend, span<std::byte> data);

    /// Read a range of scanlines (with potentially a subset of channels) of
    /// native data into contiguous memory defined by the span `data`.
    virtual bool read_native_scanlines(int subimage, int miplevel, int ybegin,
                                       int yend, int chbegin, int chend,
                                       span<std::byte> data);

    /// Read a range of tiles (all channels) of native data into contiguous
    /// memory defined by the span `data`.
    virtual bool read_native_tiles(int subimage, int miplevel,
                                   int xbegin, int xend, int ybegin, int yend,
                                   span<std::byte> data);

    /// Read a range of tiles (with potentially a subset of channels) of
    /// native data into contiguous memory defined by the span `data`.
    virtual bool read_native_tiles(int subimage, int miplevel,
                                   int xbegin, int xend, int ybegin, int yend,
                                   int chbegin, int chend,
                                   span<std::byte> data);

    /// Read a range of 3D volumetric tiles (all channels) of native data into
    /// contiguous memory. This is only functional for volumetric formats.
    virtual bool read_native_volumetric_tiles(int subimage, int miplevel,
                                   int xbegin, int xend, int ybegin, int yend,
                                   int zbegin, int zend, span<std::byte> data);

    /// Read a range of 3D volumetric tiles (with potentially a subset of
    /// channels) of native data into contiguous memory. This is only
    /// functional for volumetric formats.
    virtual bool read_native_volumetric_tiles(int subimage, int miplevel,
                                int xbegin, int xend, int ybegin, int yend,
                                int zbegin, int zend, int chbegin, int chend,
                                span<std::byte> data);

    /// OLD pointer-based version: this will eventually be deprecated.
    ///
    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual bool read_native_scanline (int subimage, int miplevel,
                                       int y, int z, void *data) = 0;

    /// OLD pointer-based version: this will eventually be deprecated.
    ///
    /// Read a range of scanlines (all channels) of native data into
    /// contiguous memory.
    virtual bool read_native_scanlines (int subimage, int miplevel,
                                        int ybegin, int yend, int z,
                                        void *data);
    /// OLD pointer-based version: this will eventually be deprecated.
    ///
    /// Read a range of scanlines (with optionally a subset of channels) of
    /// native data into contiguous memory.
    virtual bool read_native_scanlines (int subimage, int miplevel,
                                        int ybegin, int yend, int z,
                                        int chbegin, int chend, void *data);

    /// OLD pointer-based version: this will eventually be deprecated.
    ///
    /// Read a single tile (all channels) of native data into contiguous
    /// memory. The base class read_native_tile fails. A format reader that
    /// supports tiles MUST overload this virtual method that reads a single
    /// tile (all channels).
    virtual bool read_native_tile (int subimage, int miplevel,
                                   int x, int y, int z, void *data);

    /// OLD pointer-based version: this will eventually be deprecated.
    ///
    /// Read multiple tiles (all channels) of native data into contiguous
    /// memory. A format reader that supports reading multiple tiles at once
    /// (in a way that's more efficient than reading the tiles one at a
    /// time) is advised (but not required) to overload this virtual method.
    /// If an ImageInput subclass does not overload this, the default
    /// implementation here is simply to loop over the tiles, calling the
    /// single-tile read_native_tile() for each one.
    virtual bool read_native_tiles (int subimage, int miplevel,
                                    int xbegin, int xend, int ybegin, int yend,
                                    int zbegin, int zend, void *data);

    /// OLD pointer-based version: this will eventually be deprecated.
    ///
    /// Read multiple tiles (potentially a subset of channels) of native
    /// data into contiguous memory. A format reader that supports reading
    /// multiple tiles at once, and can handle a channel subset while doing
    /// so, is advised (but not required) to overload this virtual method.
    /// If an ImageInput subclass does not overload this, the default
    /// implementation here is simply to loop over the tiles, calling the
    /// single-tile read_native_tile() for each one (and copying carefully
    /// to handle the channel subset issues).
    virtual bool read_native_tiles (int subimage, int miplevel,
                                    int xbegin, int xend, int ybegin, int yend,
                                    int zbegin, int zend,
                                    int chbegin, int chend, void *data);

    /// @}


    // General message passing between client and image input server. This
    // is currently undefined and is reserved for future use.
    virtual int send_to_input (const char *format, ...);
    int send_to_client (const char *format, ...);

    /// Set an IOProxy for this reader. This must be called prior to
    /// `open()`, and only for readers that support them
    /// (`supports("ioproxy")`). The caller retains ownership of the proxy.
    ///
    /// @returns `true` for success, `false` for failure.
    virtual bool set_ioproxy (Filesystem::IOProxy* ioproxy);

    /// Is there a pending error message waiting to be retrieved, that
    /// resulted from an ImageInput API call made by the this thread?
    ///
    /// Note that any `error()` calls issued are thread-specific, and the
    /// `geterror()/has_error()` are expected to be called by the same
    /// thread that called whichever API function encountered an error.
    bool has_error() const;

    /// Return the text of all pending error messages issued against this
    /// ImageInput by the calling thread, and clear the pending error
    /// message unless `clear` is false. If no error message is pending, it
    /// will return an empty string.
    ///
    /// Note that any `error()` calls issued are thread-specific, and the
    /// `geterror()/has_error()` are expected to be called by the same
    /// thread that called whichever API function encountered an error.
    std::string geterror(bool clear = true) const;

    /// Error reporting for the plugin implementation: call this with
    /// std::format-like arguments. It is not necessary to have the error
    /// message contain a trailing newline.
    template<typename... Args>
    void errorfmt(const char* fmt, const Args&... args) const {
        append_error(Strutil::fmt::format (fmt, args...));
    }

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2, 6, 3) && \
    !defined(OIIO_INTERNAL) && !defined(OIIO_DOXYGEN)
    /// Error reporting for the plugin implementation: call this with
    /// Strutil::format-like arguments. It is not necessary to have the
    /// error message contain a trailing newline.
    /// Use with caution! Some day this will change to be fmt-like rather
    /// than printf-like.
    template<typename... Args>
    OIIO_DEPRECATED("use errorfmt instead, with std::format-like arguments (3.0)")
    void error(const char* fmt, const Args&... args) const {
        append_error(Strutil::format (fmt, args...));
    }

    /// Error reporting for the plugin implementation: call this with
    /// printf-like arguments. It is not necessary to have the error message
    /// contain a trailing newline.
    template<typename... Args>
    OIIO_DEPRECATED("use errorfmt instead, with std::format-like arguments (3.0)")
    void errorf(const char* fmt, const Args&... args) const {
        append_error(Strutil::sprintf (fmt, args...));
    }

    // Error reporting for the plugin implementation: call this with
    // std::format-like arguments. It is not necessary to have the
    // error message contain a trailing newline.
    template<typename... Args>
    OIIO_DEPRECATED("use `errorfmt` instead (2.3)")
    void fmterror(const char* fmt, const Args&... args) const {
        append_error(Strutil::fmt::format (fmt, args...));
    }
#endif

    /// Set the threading policy for this ImageInput, controlling the
    /// maximum amount of parallelizing thread "fan-out" that might occur
    /// during large read operations. The default of 0 means that the global
    /// `attribute("threads")` value should be used (which itself defaults
    /// to using as many threads as cores; see Section `Global Attributes`_).
    ///
    /// The main reason to change this value is to set it to 1 to indicate
    /// that the calling thread should do all the work rather than spawning
    /// new threads. That is probably the desired behavior in situations
    /// where the calling application has already spawned multiple worker
    /// threads.
    void threads(int n);

    /// Retrieve the current thread-spawning policy.
    /// @see  `threads(int)`
    int threads() const;

    /// There is a (hidden) internal recursive mutex to each ImageInput
    /// that can be used by the II to enforce thread safety. This is exposed
    /// via the obvious lock()/unlock()/try_lock() semantics.
    void lock() const;
    void unlock() const;
    bool try_lock() const;

    /// The presence of lock() and unlock() establish an ImageInput itself
    /// as having the BasicLockable concept and therefore can be used by
    /// std::lock_guard.
    typedef std::lock_guard<const ImageInput&> lock_guard;

    // Custom new and delete to ensure that allocations & frees happen in
    // the OpenImageIO library, not in the app or plugins (because Windows).
    void* operator new (size_t size);
    void operator delete (void *ptr);

    /// Call signature of a function that creates and returns an
    /// `ImageInput*`.
    typedef ImageInput* (*Creator)();

    /// Memory tracking method.
    /// Return the total heap memory allocated by `ImageInput`.
    /// Overridable version of heapsize defined in memory.h.
    virtual size_t heapsize() const;

    /// Memory tracking method.
    /// Return the total memory footprint of `ImageInput`.
    /// Overridable version of footprint defined in memory.h.
    virtual size_t footprint() const;

protected:
    ImageSpec m_spec;  // format spec of the current open subimage/MIPlevel
                       // BEWARE using m_spec directly -- not thread-safe

    /// @{
    /// @name IOProxy aids for ImageInput implementations.
    ///
    /// This set of utility functions are not meant to be called by user code.
    /// They are protected methods of ImageInput, and are used internally by
    /// the ImageInput implementation to help it properly implement support of
    /// IOProxy.
    ///

    /// Get the IOProxy being used underneath.
    Filesystem::IOProxy* ioproxy();
    const Filesystem::IOProxy* ioproxy() const;

    /// Is this file currently opened (active proxy)?
    bool ioproxy_opened() const;

    /// Clear the proxy ptr, and close/destroy any "local" proxy.
    void ioproxy_clear();

    /// Retrieve any ioproxy request from the configuration hint spec, and
    /// make `m_io` point to it. But if no IOProxy is found in the config,
    /// don't overwrite one we already have.
    void ioproxy_retrieve_from_config(const ImageSpec& config);

    /// Presuming that `ioproxy_retrieve_from_config` has already been called,
    /// if `m_io` is still not set (i.e., wasn't found in the config), open a
    /// IOFile local proxy with the given read/write `mode`. Return true if a
    /// proxy is set up. If it can't be done (i.e., no proxy passed, file
    /// couldn't be opened), issue an error and return false.
    bool ioproxy_use_or_open(string_view name);

    /// Helper: read from the proxy akin to fread(). Return true on success,
    /// false upon failure and issue a helpful error message. NOTE: this is
    /// not the same return value as std::fread, which returns the number of
    /// items read.
    bool ioread(void* buf, size_t itemsize, size_t nitems = 1);

    /// Helper: seek the proxy, akin to fseek. Return true on success, false
    /// upon failure and issue an error message. (NOTE: this is not the same
    /// return value as std::fseek, which returns 0 on success.)
    bool ioseek(int64_t pos, int origin = SEEK_SET);

    /// Helper: retrieve the current position of the proxy, akin to ftell.
    int64_t iotell() const;

    /// @}

    /// @{
    /// @name Helper methods for ImageInput implementations.
    ///
    /// This set of utility functions are not meant to be called by user code.
    /// They are protected methods of ImageInput, and are used internally by
    /// the ImageInput format-reading implementations.
    ///

    /// Helper: convenience boilerplate for several checks and operations that
    /// every implementation of ImageInput::open() will need to do. Failure is
    /// presumed to indicate a file that is corrupt (or perhaps maliciously
    /// crafted) and no further reading should be attempted.
    ///
    /// @param spec
    ///     The ImageSpec that we are validating.
    ///
    /// @param range
    ///     An ROI that describes the allowable resolution and channel count:
    ///     the width, height, depth, and channel count of the ROI are the
    ///     maximum allowed for the file type. For example, the default value
    ///     `{0, 65535, 0, 65535, 0, 1, 0, 4}` means that pixel data width
    ///     and height must be non-negative and representable by uint16
    ///     values, up to 4 channels are allowed, and volumes are not
    ///     permitted (z coordinate may only be 0). File formats that can
    ///     handle larger resolutions, or volumes, or >4 channels must
    ///     override these limits!
    ///
    /// @param flags
    ///     A bitfield flag (bits defined by `enum OpenChecks`) that can
    ///     indicate additional checks to perform, or checks that should be
    ///     skipped.
    ///
    /// @returns
    ///     Return `true` if the spec is valid and passes all checks,
    ///     otherwise return `false` and make appropriate calls to
    ///     this->errorfmt() to record the errors.
    ///
    /// Checks performed include:
    ///
    /// * Whether the resolution and channel count are within the range
    ///   implied by `range`.
    /// * Whether the channel count is within the `"limit:channels"` OIIO
    ///   attribute.
    /// * The total uncompressed pixel data size is expected to be within the
    ///   `"limit:imagesize_MB"` OIIO attribute.
    ///
    bool check_open (const ImageSpec &spec,
                     ROI range = {0, 65535, 0, 65535, 0, 1, 0, 4},
                     uint64_t flags = 0);

    /// Bit field definitions for the `flags` argument to `check_open()`.
    enum class OpenChecks : uint64_t {
        Defaults = 0,
        // Reserved for future use
    };

    /// Helper method: Validate that a `[chbegin, chend)` X `[xbegin, xend)` X
    /// `[ybegin, yend)` X `[zbegin, zend)` slab of native channel data types
    /// (according to the ImageSpec) can fit into the amount of contiguous
    /// memory held by the span. If it can, return true. If it can't, call
    /// `this->errorfmt()` with an appropriate error message and return false.
    /// Unspecified channel range indicates all channels.
    bool valid_raw_span_size(cspan<std::byte> buf, const ImageSpec& spec,
                             int xbegin, int xend, int ybegin, int yend,
                             int zbegin = 0, int zend = 1,
                             int chbegin = 0, int chend = -1);
    /// @}

private:
    // PIMPL idiom -- this lets us hide details of the internals of the
    // ImageInput parent class so that changing them does not break the
    // ABI.
    class Impl;
    static void impl_deleter(Impl*);
    std::unique_ptr<Impl, decltype(&impl_deleter)> m_impl;

    void append_error(string_view message) const; // add to error message

    /// declare friend heapsize and footprint definitions
    template <typename T> friend size_t pvt::heapsize(const T&);
};




/// ImageOutput abstracts the writing of an image file in a file
/// format-agnostic manner.
///
/// Users don't directly declare these. Instead, you call the `create()`
/// static method, which will return a `unique_ptr` holding a subclass of
/// ImageOutput that implements writing the particular format.
///
class OIIO_API ImageOutput {
public:
    /// unique_ptr to an ImageOutput.
    using unique_ptr = std::unique_ptr<ImageOutput>;

    /// @{
    /// @name Creating an ImageOutput

    /// Create an `ImageOutput` that can be used to write an image file.
    /// The type of image file (and hence, the particular subclass of
    /// `ImageOutput` returned, and the plugin that contains its methods) is
    /// inferred from the name, if it appears to be a full filename, or it
    /// may also name the format.
    ///
    /// @param filename
    ///         The name of the file format (e.g., "openexr"), a file
    ///         extension (e.g., "exr"), or a filename from which the the
    ///         file format can be inferred from its extension (e.g.,
    ///         "hawaii.exr"). The filename is UTF-8 encoded.
    ///
    /// @param plugin_searchpath
    ///         An optional colon-separated list of directories to search
    ///         for OpenImageIO plugin DSO/DLL's.
    ///
    /// @param ioproxy
    ///         Optional pointer to an IOProxy to use (not supported by all
    ///         formats, see `supports("ioproxy")`). The caller retains
    ///         ownership of the proxy.
    ///
    /// @returns
    ///         A `unique_ptr` that will close and free the ImageOutput when
    ///         it exits scope or is reset. The pointer will be empty if the
    ///         required writer was not able to be created.
    static unique_ptr create (string_view filename,
                              Filesystem::IOProxy* ioproxy = nullptr,
                              string_view plugin_searchpath = "");

    /// Create an ImageOutput using a UTF-16 encoded wstring filename and
    /// plugin searchpath.
    static unique_ptr create (const std::wstring& filename,
                              Filesystem::IOProxy* ioproxy = nullptr,
                              const std::wstring& plugin_searchpath = {}) {
        return create(Strutil::utf16_to_utf8(filename), ioproxy,
                      Strutil::utf16_to_utf8(plugin_searchpath));
    }

    /// @}

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2,2,0) \
    && !defined(OIIO_DOXYGEN) && !defined(OIIO_INTERNAL)
    OIIO_DEPRECATED("Obsolete version (2.2)")
    static unique_ptr create (const std::string &filename,
                              const std::string &plugin_searchpath) {
        return create(filename, nullptr, plugin_searchpath);
    }

    OIIO_DEPRECATED("destroy is no longer needed")
    static void destroy (ImageOutput *x) { delete x; }
#endif

protected:
    ImageOutput ();
public:
    virtual ~ImageOutput ();

    /// Return the name of the format implemented by this class.
    virtual const char *format_name (void) const = 0;

    // Override these functions in your derived output class
    // to inform the client which formats are supported

    /// @{
    /// @name Opening and closing files for output

    /// Given the name of a "feature", return whether this ImageOutput
    /// supports output of images with the given properties. Most queries
    /// will simply return 0 for "doesn't support" and 1 for "supports it,"
    /// but it is acceptable to have queries return other nonzero integers
    /// to indicate varying degrees of support or limits (but should be
    /// clearly documented as such).
    ///
    /// Feature names that ImageOutput implementations are expected to
    /// recognize include:
    ///
    ///  - `"tiles"` :
    ///         Is this format writer able to write tiled images?
    ///
    ///  - `"rectangles"` :
    ///         Does this writer accept arbitrary rectangular pixel regions
    ///         (via `write_rectangle()`)?  Returning 0 indicates that
    ///         pixels must be transmitted via `write_scanline()` (if
    ///         scanline-oriented) or `write_tile()` (if tile-oriented, and
    ///         only if `supports("tiles")` returns true).
    ///
    ///  - `"random_access"` :
    ///         May tiles or scanlines be written in any order (0 indicates
    ///         that they *must* be in successive order)?
    ///
    ///  - `"multiimage"` :
    ///         Does this format support multiple subimages within a file?
    ///
    ///  - `"appendsubimage"` :
    ///         Does this format support multiple subimages that can be
    ///         successively appended at will via
    ///         `open(name,spec,AppendSubimage)`? A value of 0 means that
    ///         the format requires pre-declaring the number and
    ///         specifications of the subimages when the file is first
    ///         opened, with `open(name,subimages,specs)`.
    ///
    ///  - `"mipmap"` :
    ///         Does this format support multiple resolutions for an
    ///         image/subimage?
    ///
    ///  - `"volumes"` :
    ///         Does this format support "3D" pixel arrays (a.k.a. volume
    ///         images)?
    ///
    ///  - `"alpha"` :
    ///         Can this format support an alpha channel?
    ///
    ///  - `"nchannels"` :
    ///         Can this format support arbitrary number of channels (beyond RGBA)?
    ///
    ///  - `"rewrite"` :
    ///         May the same scanline or tile be sent more than once?
    ///         Generally, this is true for plugins that implement
    ///         interactive display, rather than a saved image file.
    ///
    ///  - `"empty"` :
    ///         Does this plugin support passing a NULL data pointer to the
    ///         various `write` routines to indicate that the entire data
    ///         block is composed of pixels with value zero?  Plugins that
    ///         support this achieve a speedup when passing blank scanlines
    ///         or tiles (since no actual data needs to be transmitted or
    ///         converted).
    ///
    ///  - `"channelformats"` :
    ///         Does this format writer support per-channel data formats,
    ///         respecting the ImageSpec's `channelformats` field? If not,
    ///         it only accepts a single data format for all channels and
    ///         will ignore the `channelformats` field of the spec.
    ///
    ///  - `"displaywindow"` :
    ///         Does the format support display ("full") windows distinct
    ///         from the pixel data window?
    ///
    ///  - `"origin"` :
    ///         Does the image format support specifying a pixel window
    ///         origin (i.e., nonzero ImageSpec `x`, `y`, `z`)?
    ///
    ///  - `"negativeorigin"` :
    ///         Does the image format allow data and display window origins
    ///         (i.e., ImageSpec `x`, `y`, `z`, `full_x`, `full_y`, `full_z`)
    ///         to have negative values?
    ///
    ///  - `"deepdata"` :
    ///        Does the image format allow "deep" data consisting of
    ///        multiple values per pixel (and potentially a differing number
    ///        of values from pixel to pixel)?
    ///
    ///  - `"arbitrary_metadata"` :
    ///         Does the image file format allow metadata with arbitrary
    ///         names (and either arbitrary, or a reasonable set of, data
    ///         types)? (Versus the file format supporting only a fixed list
    ///         of specific metadata names/values.)
    ///
    ///  - `"exif"`
    ///         Does the image file format support Exif camera data (either
    ///         specifically, or via arbitrary named metadata)?
    ///
    ///  - `"iptc"`
    ///         Does the image file format support IPTC data (either
    ///         specifically, or via arbitrary named metadata)?
    ///
    ///  - `"ioproxy"`
    ///         Does the image file format support writing to an `IOProxy`?
    ///
    ///  - `"procedural"` :
    ///        Is this a purely procedural output that doesn't write an
    ///        actual file?
    ///
    ///  - `"thumbnail"` :
    ///        Does this format writer support adding a  reduced resolution
    ///        copy of the image via the `thumbnail()` method?
    ///
    ///  - `"thumbnail_after_write"` :
    ///        Does this format writer support calling `thumbnail()` after
    ///        the scanlines or tiles have been specified? (Supporting
    ///        `"thumbnail"` but not `"thumbnail_after_write"` means that any
    ///        thumbnail must be supplied immediately after `open()`, prior
    ///        to any of the `write_*()` calls.)
    ///
    ///  - `"noimage"` :
    ///        Does this format allow 0x0 sized images, i.e. an image file
    ///        with metadata only and no pixels?
    ///
    /// This list of queries may be extended in future releases. Since this
    /// can be done simply by recognizing new query strings, and does not
    /// require any new API entry points, addition of support for new
    /// queries does not break ``link compatibility'' with
    /// previously-compiled plugins.
    virtual int supports (string_view feature OIIO_MAYBE_UNUSED) const {
        return false;
    }

    /// Modes passed to the `open()` call.
    enum OpenMode { Create, AppendSubimage, AppendMIPLevel };

    /// Open the file with given name, with resolution and other format
    /// data as given in newspec. It is legal to call open multiple times
    /// on the same file without a call to `close()`, if it supports
    /// multiimage and mode is AppendSubimage, or if it supports
    /// MIP-maps and mode is AppendMIPLevel -- this is interpreted as
    /// appending a subimage, or a MIP level to the current subimage,
    /// respectively.
    ///
    /// @param  filename    The name of the image file to open, UTF-8 encoded.
    /// @param  newspec     The ImageSpec describing the resolution, data
    ///                     types, etc.
    /// @param  mode        Specifies whether the purpose of the `open` is
    ///                     to create/truncate the file (default: `Create`),
    ///                     append another subimage (`AppendSubimage`), or
    ///                     append another MIP level (`AppendMIPLevel`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool open (const std::string &filename, const ImageSpec &newspec,
                       OpenMode mode=Create) = 0;

    /// Open an ImageOutput using a UTF-16 encoded wstring filename.
    bool open (const std::wstring &filename, const ImageSpec &newspec,
               OpenMode mode=Create) {
        return open(Strutil::utf16_to_utf8(filename), newspec, mode);
    }

    /// Open a multi-subimage file with given name and specifications for
    /// each of the subimages.  Upon success, the first subimage will be
    /// open and ready for transmission of pixels.  Subsequent subimages
    /// will be denoted with the usual call of
    /// `open(name,spec,AppendSubimage)` (and MIP levels by
    /// `open(name,spec,AppendMIPLevel)`).
    ///
    /// The purpose of this call is to accommodate format-writing
    /// libraries that must know the number and specifications of the
    /// subimages upon first opening the file; such formats can be
    /// detected by::
    ///     supports("multiimage") && !supports("appendsubimage")
    /// The individual specs passed to the appending open() calls for
    /// subsequent subimages *must* match the ones originally passed.
    ///
    /// @param  filename    The name of the image file to open, UTF-8 encoded.
    /// @param  subimages   The number of subimages (and therefore the
    ///                     length of the `specs[]` array.
    /// @param  specs[]
    ///                      Pointer to an array of `ImageSpec` objects
    ///                      describing each of the expected subimages.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool open (const std::string &filename,
                       int subimages OIIO_MAYBE_UNUSED,
                       const ImageSpec *specs) {
        // Default implementation: just a regular open, assume that
        // appending will work.
        return open (filename, specs[0]);
    }

    bool open (const std::wstring &filename, int subimages OIIO_MAYBE_UNUSED,
               const ImageSpec *specs) {
        // Default implementation: just a regular open, assume that
        // appending will work.
        return open (Strutil::utf16_to_utf8(filename), specs[0]);
    }

    /// Return a reference to the image format specification of the current
    /// subimage.  Note that the contents of the spec are invalid before
    /// `open()` or after `close()`.
    const ImageSpec &spec (void) const { return m_spec; }

    /// Closes the currently open file associated with this ImageOutput and
    /// frees any memory or resources associated with it.
    virtual bool close () = 0;
    /// @}

    // clang-format on

    /// @{
    /// @name Writing pixels ("safe" methods with bounded spans)
    ///
    /// Common features of all the `write` methods:
    ///
    /// * There is a base case that takes a `image_span<byte>` describing
    ///   untyped memory layout and a `TypeDesc` describing the data type
    ///   that the values should be converted from (or TypeUnknown to
    ///   indicate that the data is already in its "native" file types with
    ///   no conversion needed).
    ///
    /// * The type-aware versions that accept an `image_span<T>` (for
    ///   potentially non-contiguous data) or `span<T>` (for contiguous data)
    ///   and understand to convert the data from the given `T` type.
    ///
    /// * The image_span (in either case) includes the memory bounds and
    ///   stride lengths (in bytes) between channels, pixels, scanlines, and
    ///   volumetric slices.
    ///
    /// * Any *range* parameters (such as `ybegin` and `yend`) describe a
    ///   "half open interval", meaning that `begin` is the first item and
    ///   `end` is *one past the last item*. That means that the number of
    ///   items is `end - begin`.
    ///
    /// * For ordinary 2D (non-volumetric) images, any `z` or `zbegin`
    ///   coordinates should be 0 and any `zend` should be 1, indicating
    ///   that only a single image "plane" exists.
    ///
    /// * Scanlines or tiles must be written in successive increasing
    ///   coordinate order, unless the particular output file driver allows
    ///   random access (indicated by `supports("random_access")`).
    ///
    /// * All write functions return `true` for success, `false` for failure
    ///   (after which a call to `geterror()` may retrieve a specific error
    ///   message).
    ///

    /// Given an explicit data type and an `image_span` of untyped bytes,
    /// write the entire image of `spec.width x spec.height x spec.depth`
    /// pixels from the buffer.
    ///
    /// Depending on the spec, this will write either all tiles or all
    /// scanlines. Assume that data points to a layout in row-major order.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_image that takes raw pointers.
    ///
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that the data is already in the native
    ///                     format and needs no type conversion.
    /// @param  data        An `image_span<const byte>` describing the memory
    ///                     extent of the data buffer and including the sizes
    ///                     and byte strides for each dimension (channel, x,
    ///                     y, and z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    virtual bool write_image(TypeDesc format,
                             const image_span<const std::byte>& data);

    /// A version of `write_image()` taking an `image_span<T>`, where the type
    /// of the underlying data is `T`.  This is a convenience wrapper around
    /// the `write_image()` that takes an `image_span<const std::byte>`.
    template<typename T> bool write_image(const image_span<T>& data)
    {
        return write_image(TypeDescFromC<T>::value(),
                           as_image_span_bytes(data));
    }

    /// A version of `write_image()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `write_image()` that takes an `image_span<const T>`.
    template<typename T> bool write_image(span<T> data)
    {
        auto ispan = image_span<const T>(data.data(), m_spec.nchannels,
                                         m_spec.width, m_spec.height,
                                         m_spec.depth);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return write_image(ispan);
    }

    /// Write one scanline that include pixels (*,y) from `data`, which is an
    /// `image_span` of un-typed bytes incorporating its bounded dimensions
    /// and strides. The data type is given explicity by the `format`
    /// argument, and will be automatically converted to the type being stored
    /// in the file.
    ///
    /// The image_span must have a width equal to a full scanline width,
    /// and its height and depth must be 1.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_scanline that takes raw pointers.
    ///
    /// @param  y           The y coordinate of the scanline.
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that the data is already in the native
    ///                     format and needs no type conversion.
    /// @param  data        An `image_span<byte>` describing the buffer and
    ///                     including its sizes and byte strides for each
    ///                     dimension (channel, x, y, z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    virtual bool write_scanline(int y, TypeDesc format,
                                const image_span<const std::byte>& data);

    /// A version of `write_scanline()` taking an `image_span<T>`, where the
    /// type of the underlying data is `T`.  This is a convenience wrapper
    /// around the `write_scanline()` that takes an `image_span<const
    /// std::byte>`.
    template<typename T> bool write_scanline(int y, const image_span<T>& data)
    {
        // reduce to type + image_span<byte>
        return write_scanline(y, TypeDescFromC<T>::value(),
                              as_image_span_bytes(data));
    }

    /// A version of `write_scanline()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `write_scanline()` that takes an `image_span<const T>`.
    template<typename T> bool write_scanline(int y, span<T> data)
    {
        // reduce to type + image_span<byte>
        return write_scanline(y, image_span<T>(data.data(), m_spec.nchannels,
                                               m_spec.width, 1, 1));
    }

    /// Write multiple scanlines that include pixels (*,y) for all `ybegin
    /// <= y < yend`, from `data`, which is an `image_span` of un-typed bytes
    /// incorporating its bounded dimensions and strides. The data type is
    /// given explicity by the `format` argument, and will be automatically
    /// converted to the type being stored in the file. You can write a single
    /// scanline by passing `yend == ybegin + 1`, but passing multiple
    /// scanlines in each call often has performance advantages for many file
    /// formats.
    ///
    /// The image_span must have a width equal to a full scanline width,
    /// and its height must be yend - ybegin.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_scanlines that takes raw pointers.
    ///
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that the data is already in the native
    ///                     format and needs no type conversion.
    /// @param  data        An `image_span<byte>` describing the buffer and
    ///                     including its sizes and byte strides for each
    ///                     dimension (channel, x, y, z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    virtual bool write_scanlines(int ybegin, int yend, TypeDesc format,
                                 const image_span<const std::byte>& data);

    /// A version of `write_scanlines()` taking an `image_span<T>`, where the
    /// type of the underlying data is `T`.  This is a convenience wrapper
    /// around the `write_scanlines()` that takes an `image_span<const
    /// std::byte>`.
    template<typename T>
    bool write_scanlines(int ybegin, int yend, const image_span<T>& data)
    {
        // image_span<T>: reduces to type + byte_buffer
        return write_scanlines(ybegin, yend, TypeDescFromC<T>::value(),
                               data.as_image_span_bytes());
    }

    /// A version of `write_scanlines()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `write_scanlines()` that takes an `image_span<const T>`.
    template<typename T>
    bool write_scanlines(int ybegin, int yend, span<T> data)
    {
        auto ispan = image_span<T>(data.data(), m_spec.nchannels, m_spec.width,
                                   yend - ybegin, 1);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return write_scanlines(ybegin, yend, ispan);
    }


    /// Write a single tile of pixels whose upper left pixel coordinate is (x,
    /// y, z) from a buffer described by `data`, which is an `image_span` of
    /// un-typed bytes incorporating its bounded dimensions and strides. The
    /// data type is given explicity by the `format` argument, and will be
    /// automatically converted to the type being stored in the file. The
    /// (x,y,z) coordinates must be the pixel coordinates of the first (upper
    /// left corner) pixel of a tile.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_tile that takes raw pointers.
    ///
    /// @param  x/y/z       The x range of the pixels being passed.
    /// @param  ybegin/yend The y range of the pixels being passed.
    /// @param  zbegin/zend The z range of the pixels being passed
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that the data is already in the native
    ///                     format and needs no type conversion.
    /// @param  data        An `image_span<byte>` describing the buffer and
    ///                     including its sizes and byte strides for each
    ///                     dimension (channel, x, y, z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_tile that takes raw pointers.
    ///
    virtual bool write_tile(int x, int y, int z, TypeDesc format,
                            const image_span<const std::byte>& data);

    /// A version of `write_tile()` taking an `image_span<T>`, where the type
    /// of the underlying data is `T`.  This is a convenience wrapper around
    /// the `write_tile()` that takes an `image_span<const std::byte>`.
    template<typename T>
    bool write_tile(int x, int y, int z, const image_span<T>& data)
    {
        return write_tile(x, y, z, TypeDescFromC<T>::value(),
                          as_image_span_bytes(data));
    }

    /// A version of `write_tile()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `write_tile()` that takes an `image_span<const T>`.
    template<typename T> bool write_tile(int x, int y, int z, span<T> data)
    {
        auto ispan = image_span<T>(data.data(), m_spec.nchannels,
                                   m_spec.tile_width, m_spec.tile_height,
                                   m_spec.tile_depth);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return write_tile(x, y, z, ispan);
    }

    /// Write a rectangular region of tiles of pixels given by the range
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// contained in `data`, which is an `image_span` of un-typed bytes
    /// incorporating its bounded dimensions and strides. The data type is
    /// given explicity by the `format` argument, and will be automatically
    /// converted to the type being stored in the file. The begin/end
    /// coordinates must be at tile or image boundaries.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_tiles that takes raw pointers.
    ///
    /// @param  xbegin/xend The x range of the pixels being passed.
    /// @param  ybegin/yend The y range of the pixels being passed.
    /// @param  zbegin/zend The z range of the pixels being passed
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that the data is already in the native
    ///                     format and needs no type conversion.
    /// @param  data        An `image_span<byte>` describing the buffer and
    ///                     including its sizes and byte strides for each
    ///                     dimension (channel, x, y, z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    virtual bool write_tiles(int xbegin, int xend, int ybegin, int yend,
                             int zbegin, int zend, TypeDesc format,
                             const image_span<const std::byte>& data);

    /// A version of `write_tiles()` taking an `image_span<T>`, where the type
    /// of the underlying data is `T`.  This is a convenience wrapper around
    /// the `write_tiles()` that takes an `image_span<const std::byte>`.
    template<typename T>
    bool write_tiles(int xbegin, int xend, int ybegin, int yend, int zbegin,
                     int zend, const image_span<T>& data)
    {
        return write_tiles(xbegin, xend, ybegin, yend, zbegin, zend,
                           TypeDescFromC<T>::value(),
                           as_image_span_bytes(data));
    }

    /// A version of `write_tiles()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `write_tiles()` that takes an `image_span<const T>`.
    template<typename T>
    bool write_tiles(int xbegin, int xend, int ybegin, int yend, int zbegin,
                     int zend, span<T> data)
    {
        auto ispan = image_span<T>(data.data(), m_spec.nchannels, xend - xbegin,
                                   yend - ybegin, zend - zbegin);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return write_tiles(xbegin, xend, ybegin, yend, zbegin, zend, ispan);
    }

    /// Write a rectangle of pixels given by the range
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// from a buffer described by `data`, which is an `image_span`
    /// incorporating its bounded dimensions, strides, and data type.
    ///
    /// The begin/end coordinates do not need to be tile boundaries, but the
    /// call is only supported for format plugins that return true for
    /// `supports("rectangles")`.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of write_rectangle that takes raw pointers.
    ///
    /// @param  xbegin/xend The x range of the pixels being passed.
    /// @param  ybegin/yend The y range of the pixels being passed.
    /// @param  format      A TypeDesc describing the type of the pixel data
    ///                     that `data`'s memory contains. Use `TypeUnknown`
    ///                     to indicate that the data is already in the native
    ///                     format and needs no type conversion.
    /// @param  data        An `image_span<byte>` describing the buffer and
    ///                     including its sizes and byte strides for each
    ///                     dimension (channel, x, y, z).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    virtual bool write_rectangle(int xbegin, int xend, int ybegin, int yend,
                                 int zbegin, int zend, TypeDesc format,
                                 const image_span<const std::byte>& data);

    /// A version of `write_rectangle()` taking an `image_span<T>`, where the
    /// type of the underlying data is `T`.  This is a convenience wrapper
    /// around the `write_rectangle()` that takes an `image_span<const
    /// std::byte>`.
    template<typename T>
    bool write_rectangle(int xbegin, int xend, int ybegin, int yend, int zbegin,
                         int zend, const image_span<T>& data)
    {
        return write_rectangle(xbegin, xend, ybegin, yend, zbegin, zend,
                               TypeDescFromC<T>::value(),
                               as_image_span_bytes(data));
    }

    /// A version of `write_rectangle()` taking a `cspan<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `write_rectangle()` that takes an `image_span<const T>`.
    template<typename T>
    bool write_rectangle(int xbegin, int xend, int ybegin, int yend, int zbegin,
                         int zend, span<T> data)
    {
        auto ispan = image_span<const T>(data.data(), m_spec.nchannels,
                                         xend - xbegin, yend - ybegin,
                                         zend - zbegin);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return write_rectangle(xbegin, xend, ybegin, yend, zbegin, zend, ispan);
    }

    // clang-format off

    /// Write deep scanlines containing pixels (*,y,z), for all y in the
    /// range [ybegin,yend), to a deep file. This will fail if it is not a
    /// deep file.
    ///
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  deepdata    A `DeepData` object with the data for these
    ///                     scanlines.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_deep_scanlines (int ybegin, int yend, int z,
                                       const DeepData &deepdata);

    /// Write the block of deep tiles that include all pixels in
    /// the range
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size.
    ///
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles passed.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  deepdata    A `DeepData` object with the data for the tiles.
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool write_deep_tiles (int xbegin, int xend, int ybegin, int yend,
                                   int zbegin, int zend,
                                   const DeepData &deepdata);

    /// Write the entire deep image described by `deepdata`. Depending on
    /// the spec, this will write either all tiles or all scanlines.
    ///
    /// @param  deepdata    A `DeepData` object with the data for the image.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_deep_image (const DeepData &deepdata);

    /// Specify a reduced-resolution ("thumbnail") version of the image.
    /// Note that many image formats may require the thumbnail to be
    /// specified prior to writing the pixels.
    ///
    /// @param thumb
    ///         A reference to an `ImageBuf` containing the thumbnail image.
    /// @returns
    ///         `true` upon success, `false` if it was not possible to write
    ///         the thumbnail, or if this file format (or writer) does not
    ///         support thumbnails.
    ///
    /// @note This method was added to OpenImageIO 2.3.
    virtual bool set_thumbnail(const ImageBuf& thumb) { return false; }

    /// @}

    /// @{
    /// @name Writing pixels (unsafe methods with pointers and strides)
    ///
    /// These methods are the "unsafe" versions of the `write` methods, which
    /// take raw pointers and strides.  They are provided for backwards
    /// compatibility with older code, but the preferred interface is to use
    /// the `write` methods that take `image_span<const T>` or `cspan<T>`
    /// arguments, which are bounds-safe and type-aware.
    ///
    /// These pointer-based versions are considered "soft-deprecated" in
    /// OpenImageIO 3.1, will be marked/warned as deprecated in 3.2, and will
    /// be removed in 4.0.
    ///
    /// Common features of all the `write` methods:
    ///
    /// * The `format` parameter describes the data type of the `data[]`. The
    ///   write methods automatically convert the data from the specified
    ///   `format` to the actual output data type of the file (as was
    ///   specified by the ImageSpec passed to `open()`).  If `format` is
    ///   `TypeUnknown`, then rather than converting from `format`, it will
    ///   just copy pixels assumed to already be in the file's native data
    ///   layout (including, possibly, per-channel data formats as specified
    ///   by the ImageSpec's `channelformats` field).
    ///
    /// * The `stride` values describe the layout of the `data` buffer:
    ///   `xstride` is the distance in bytes between successive pixels
    ///   within each scanline. `ystride` is the distance in bytes between
    ///   successive scanlines. For volumetric images `zstride` is the
    ///   distance in bytes between successive "volumetric planes".  Strides
    ///   set to the special value `AutoStride` imply contiguous data, i.e.,
    ///
    ///       xstride = format.size() * nchannels
    ///       ystride = xstride * width
    ///       zstride = ystride * height
    ///
    /// * Any *range* parameters (such as `ybegin` and `yend`) describe a
    ///   "half open interval", meaning that `begin` is the first item and
    ///   `end` is *one past the last item*. That means that the number of
    ///   items is `end - begin`.
    ///
    /// * For ordinary 2D (non-volumetric) images, any `z` or `zbegin`
    ///   coordinates should be 0 and any `zend` should be 1, indicating
    ///   that only a single image "plane" exists.
    ///
    /// * Scanlines or tiles must be written in successive increasing
    ///   coordinate order, unless the particular output file driver allows
    ///   random access (indicated by `supports("random_access")`).
    ///
    /// * All write functions return `true` for success, `false` for failure
    ///   (after which a call to `geterror()` may retrieve a specific error
    ///   message).
    ///

    /// Write the full scanline that includes pixels (*,y,z).  For 2D
    /// non-volume images, `z` should be 0. The `xstride` value gives the
    /// distance between successive pixels (in bytes).  Strides set to
    /// `AutoStride` imply "contiguous" data.
    ///
    /// @param  y/z         The y & z coordinates of the scanline.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride     The distance in bytes between successive
    ///                     pixels in `data` (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_scanline (int y, int z, TypeDesc format,
                                 const void *data, stride_t xstride=AutoStride);

    /// Write multiple scanlines that include pixels (*,y,z) for all ybegin
    /// <= y < yend, from data.  This is analogous to
    /// `write_scanline(y,z,format,data,xstride)` repeatedly for each of the
    /// scanlines in turn (the advantage, though, is that some image file
    /// types may be able to write multiple scanlines more efficiently or
    /// in parallel, than it could with one scanline at a time).
    ///
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride
    ///                     The distance in bytes between successive pixels
    ///                     and scanlines (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_scanlines (int ybegin, int yend, int z,
                                  TypeDesc format, const void *data,
                                  stride_t xstride=AutoStride,
                                  stride_t ystride=AutoStride);

    /// Write the tile with (x,y,z) as the upper left corner.  The three
    /// stride values give the distance (in bytes) between successive
    /// pixels, scanlines, and volumetric slices, respectively.  Strides set
    /// to AutoStride imply 'contiguous' data in the shape of a full tile,
    /// i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * spec.tile_width
    ///     zstride = ystride * spec.tile_height
    ///
    /// @param  x/y/z       The upper left coordinate of the tile being passed.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride` to
    ///                     indicate a "contiguous" single tile).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note This call will fail if the image is not tiled, or if (x,y,z)
    /// is not the upper left corner coordinates of a tile.
    virtual bool write_tile (int x, int y, int z, TypeDesc format,
                             const void *data, stride_t xstride=AutoStride,
                             stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride);

    /// Write the block of multiple tiles that include all pixels in
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// This is analogous to calling `write_tile(x,y,z,...)` for each tile
    /// in turn (but for some file formats, passing multiple tiles may allow
    /// it to write more efficiently or in parallel).
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size. The
    /// stride values give the data spacing of adjacent pixels, scanlines,
    /// and volumetric slices (measured in bytes). Strides set to AutoStride
    /// imply contiguous data in the shape of the [begin,end) region, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * (xend-xbegin)
    ///     zstride = ystride * (yend-ybegin)
    ///
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles passed.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool write_tiles (int xbegin, int xend, int ybegin, int yend,
                              int zbegin, int zend, TypeDesc format,
                              const void *data, stride_t xstride=AutoStride,
                              stride_t ystride=AutoStride,
                              stride_t zstride=AutoStride);

    /// Write a rectangle of pixels given by the range
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// The stride values give the data spacing of adjacent pixels,
    /// scanlines, and volumetric slices (measured in bytes). Strides set to
    /// AutoStride imply contiguous data in the shape of the [begin,end)
    /// region, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * (xend-xbegin)
    ///     zstride = ystride * (yend-ybegin)
    ///
    /// @param  xbegin/xend The x range of the pixels being passed.
    /// @param  ybegin/yend The y range of the pixels being passed.
    /// @param  zbegin/zend The z range of the pixels being passed
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail for a format plugin that does not return
    /// true for `supports("rectangles")`.
    virtual bool write_rectangle (int xbegin, int xend, int ybegin, int yend,
                                  int zbegin, int zend, TypeDesc format,
                                  const void *data, stride_t xstride=AutoStride,
                                  stride_t ystride=AutoStride,
                                  stride_t zstride=AutoStride);

    /// Write the entire image of `spec.width x spec.height x spec.depth`
    /// pixels, from a buffer with the given strides and in the desired
    /// format.
    ///
    /// Depending on the spec, this will write either all tiles or all
    /// scanlines. Assume that data points to a layout in row-major order.
    ///
    /// Because this may be an expensive operation, a progress callback
    /// may be passed.  Periodically, it will be called as follows:
    ///
    ///     progress_callback (progress_callback_data, float done);
    ///
    /// where `done` gives the portion of the image (between 0.0 and 1.0)
    /// that has been written thus far.
    ///
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @param  progress_callback/progress_callback_data
    ///                     Optional progress callback.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_image (TypeDesc format, const void *data,
                              stride_t xstride=AutoStride,
                              stride_t ystride=AutoStride,
                              stride_t zstride=AutoStride,
                              ProgressCallback progress_callback=nullptr,
                              void *progress_callback_data=nullptr);

    /// @}

    /// Read the pixels of the current subimage of `in`, and write it as the
    /// next subimage of `*this`, in a way that is efficient and does not
    /// alter pixel values, if at all possible.  Both `in` and `this` must
    /// be a properly-opened `ImageInput` and `ImageOutput`, respectively,
    /// and their current images must match in size and number of channels.
    ///
    /// If a particular ImageOutput implementation does not supply a
    /// `copy_image` method, it will inherit the default implementation,
    /// which is to simply read scanlines or tiles from `in` and write them
    /// to `*this`.  However, some file format implementations may have a
    /// special technique for directly copying raw pixel data from the input
    /// to the output, when both are the same file type and the same pixel
    /// data type.  This can be more efficient than `in->read_image()`
    /// followed by `out->write_image()`, and avoids any unintended pixel
    /// alterations, especially for formats that use lossy compression.
    ///
    /// If the function fails and returns `false`, an error message can be
    /// retrieved from `this->geterror()`, even if the actual error was
    /// related to reading from `in` (i.e., reading errors are automatically
    /// transferrred to `*this`).
    ///
    /// Note: this method is NOT thread-safe (against other threads that may
    /// also be using `in`), since it depends on persistent state in the
    /// ImageInput.
    ///
    /// @param  in          A pointer to the open `ImageInput` to read from.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool copy_image (ImageInput *in);

    // General message passing between client and image output server. This
    // is currently undefined and is reserved for future use.
    virtual int send_to_output (const char *format, ...);
    int send_to_client (const char *format, ...);

    /// Set an IOProxy for this writer. This must be called prior to
    /// `open()`, and only for writers that support them
    /// (`supports("ioproxy")`). The caller retains ownership of the proxy.
    ///
    /// @returns `true` for success, `false` for failure.
    virtual bool set_ioproxy (Filesystem::IOProxy* ioproxy);

    /// Is there a pending error message waiting to be retrieved, that
    /// resulted from an ImageOutput API call made by the this thread?
    ///
    /// Note that any `error()` calls issued are thread-specific, and the
    /// `geterror()/has_error()` are expected to be called by the same
    /// thread that called whichever API function encountered an error.
    bool has_error() const;

    /// Return the text of all pending error messages issued against this
    /// ImageOutput by the calling thread, and clear the pending error
    /// message unless `clear` is false. If no error message is pending, it
    /// will return an empty string.
    ///
    /// Note that any `error()` calls issued are thread-specific, and the
    /// `geterror()/has_error()` are expected to be called by the same
    /// thread that called whichever API function encountered an error.
    std::string geterror(bool clear = true) const;

    /// Error reporting for the plugin implementation: call this with
    /// std::format-like arguments. It is not necessary to have the error
    /// message contain a trailing newline.
    template<typename... Args>
    void errorfmt(const char* fmt, const Args&... args) const {
        append_error(Strutil::fmt::format (fmt, args...));
    }

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2, 6, 3) && \
    !defined(OIIO_INTERNAL) && !defined(OIIO_DOXYGEN)
    /// Error reporting for the plugin implementation: call this with
    /// `Strutil::format`-like arguments. It is not necessary to have the
    /// error message contain a trailing newline.
    /// Use with caution! Some day this will change to be fmt-like rather
    /// than printf-like.
    template<typename... Args>
    OIIO_DEPRECATED("use errorfmt instead, with std::format-like arguments (3.0)")
    void error(const char* fmt, const Args&... args) const {
        append_error(Strutil::format (fmt, args...));
    }

    /// Error reporting for the plugin implementation: call this with
    /// printf-like arguments. It is not necessary to have the error message
    /// contain a trailing newline.
    template<typename... Args>
    OIIO_DEPRECATED("use errorfmt instead, with std::format-like arguments (3.0)")
    void errorf(const char* fmt, const Args&... args) const {
        append_error(Strutil::sprintf (fmt, args...));
    }

    // Error reporting for the plugin implementation: call this with
    // std::format-like arguments. It is not necessary to have the error
    // message contain a trailing newline.
    template<typename... Args>
    OIIO_DEPRECATED("use `errorfmt` instead (2.3)")
    void fmterror(const char* fmt, const Args&... args) const {
        append_error(Strutil::fmt::format (fmt, args...));
    }
#endif

    /// Set the threading policy for this ImageOutput, controlling the
    /// maximum amount of parallelizing thread "fan-out" that might occur
    /// during large write operations. The default of 0 means that the
    /// global `attribute("threads")` value should be used (which itself
    /// defaults to using as many threads as cores; see Section
    /// `Global Attributes`_).
    ///
    /// The main reason to change this value is to set it to 1 to indicate
    /// that the calling thread should do all the work rather than spawning
    /// new threads. That is probably the desired behavior in situations
    /// where the calling application has already spawned multiple worker
    /// threads.
    void threads(int n);

    /// Retrieve the current thread-spawning policy.
    /// @see  `threads(int)`
    int threads() const;

    // Custom new and delete to ensure that allocations & frees happen in
    // the OpenImageIO library, not in the app or plugins (because Windows).
    void* operator new (size_t size);
    void operator delete (void *ptr);

    /// Call signature of a function that creates and returns an
    /// `ImageOutput*`.
    typedef ImageOutput* (*Creator)();

    /// Memory tracking method.
    /// Return the total heap memory allocated by `ImageOutput`.
    /// Overridable version of heapsize defined in memory.h.
    virtual size_t heapsize() const;

    /// Memory tracking method.
    /// Return the total memory footprint of `ImageOutput`.
    /// Overridable version of footprint defined in memory.h.
    virtual size_t footprint() const;

protected:
    /// @{
    /// @name Helper functions for ImageOutput implementations.
    ///
    /// This set of utility functions are not meant to be called by user code.
    /// They are protected methods of ImageOutput, and are used internally by
    /// the ImageOutput implementation to help it properly implement support
    /// of IOProxy.
    ///

    /// Helper: convenience boilerplate for several checks and operations that
    /// every implementation of ImageOutput::open() will need to do.
    ///
    /// 1. Check if the open `mode` is one allowed by `supports("multiimage")`
    ///    and `supports("mipmap")`.
    /// 2. Copy the passed spec to the internal m_spec.
    /// 3. Do all possible validity checks based on `supports()` (for example,
    ///    is the request to write volumetric data but the format writer
    ///    doesn't support it).
    ///
    /// Returns true if ok, false if the open request can't be satisfied (and
    /// makes appropriate calls to this->errorfmt() to record the errors).
    ///
    /// Having a central helper method for this is beneficial:
    ///
    /// * Less repeated code in the many open() implementations, which also
    ///   means less opportunity for bugs.
    /// * Ensures that all image writers perform the full set of possible
    ///   validity checks.
    /// * Ensures that error messages are consistent across all writers and
    ///   can be improved in a single location.
    /// * Better code coverage for testing, because the error handling that is
    ///   done centrally means we don't need to separately test every possible
    ///   error condition in every writer.
    ///
    /// @param mode
    ///     The mode in which the file is to be opened (`Create`,
    ///     `AppendSubimage`, or `AppendMIPLevel`).
    ///
    /// @param spec
    ///     The ImageSpec that we are validating.
    ///
    /// @param range
    ///     An ROI that describes the allowable pixel coordinates and channel
    ///     indices as half-open intervals.  For example, the default value
    ///     `{0, 65535, 0, 65535, 0, 1, 0, 4}` means that pixel coordinates
    ///     must be non-negative and the width and height be representable by
    ///     a uint16 value, up to 4 channels are allowed, and volumes are not
    ///     permitted (z coordinate may only be 0). File formats that can
    ///     handle larger resolutions, or volumes, or >4 channels must
    ///     override these limits!
    ///
    /// @param flags
    ///     A bitfield flag (bits defined by `enum OpenChecks`) that can
    ///     indicate additional checks to perform, or checks that should be
    ///     skipped.
    ///
    /// @returns
    ///     Return `true` if the spec is valid and passes all checks,
    ///     otherwise return `false` and make appropriate calls to
    ///     this->errorfmt() to record the errors.
    ///
    /// Checks performed include:
    /// 
    /// * Whether the open `mode` is one allowed by `supports("multiimage")`
    ///   and `supports("mipmap")`.
    /// * Whether the resolution and channel count is within the range
    ///   implied by `range`. If `spec.depth < 1`, it will be set to 1.
    /// * Whether the request for volumes or deep images can be accommodated
    ///   by the format (according to its `supports()` queries).
    /// * If per-channel data types are supplied (and not all the same), but
    ///   but the file format does not not `supports("channelformats")`. If
    ///   `spec.channelformats` is used but all formats are equal, then
    ///   the `channelformats` vector will be cleared and only `spec.format`
    ///   will be used.
    /// * If any of the "full" size fields are negative or zero, they will be
    ///   set to the corresponding pixel data size fields.
    /// * Whether the pixel origin offset (`spec.x`, `spec.y`, `spec.z`) is
    ///   allowed to be non-zero (according to `supports("origin")` or
    ///   negative (`supports("negativeorigin")`) -- if it is not allowed,
    ///   it is an error if flags includes `Strict`, otherwise it will simply
    ///   be adjusted to 0.
    /// * Whether the `extra_attribs` contains a request to use an IOProxy,
    ///   but the format writer does not report `supports("ioproxy")`.
    bool check_open(OpenMode mode, const ImageSpec &spec,
                    ROI range = {0, 65535, 0, 65535, 0, 1, 0, 4},
                    uint64_t flags = 0);

    /// Bit field definitions for the `flags` argument to `check_open()`.
    enum class OpenChecks : uint64_t {
        Defaults = 0,
        Disallow1Channel = 1,
        Disallow2Channel = 2,
        Disallow1or2Channel = Disallow1Channel | Disallow2Channel,
        Strict = (uint64_t(1) << 32)
    };

    /// Helper routines used by write_* implementations: convert data (in
    /// the given format and stride) to the "native" format of the file
    /// (described by the 'spec' member variable), in contiguous order. This
    /// requires a scratch space to be passed in so that there are no memory
    /// leaks.  Returns a pointer to the native data, which may be the
    /// original data if it was already in native format and contiguous, or
    /// it may point to the scratch space if it needed to make a copy or do
    /// conversions. For float->uint8 conversions only, if dither is
    /// nonzero, random dither will be added to reduce quantization banding
    /// artifacts; in this case, the specific nonzero dither value is used
    /// as a seed for the hash function that produces the per-pixel dither
    /// amounts, and the optional [xyz]origin parameters help it to align
    /// the pixels to the right position in the dither pattern.
    const void *to_native_scanline (TypeDesc format,
                                    const void *data, stride_t xstride,
                                    std::vector<unsigned char> &scratch,
                                    unsigned int dither=0,
                                    int yorigin=0, int zorigin=0);
    const void *to_native_tile (TypeDesc format, const void *data,
                                stride_t xstride, stride_t ystride,
                                stride_t zstride,
                                std::vector<unsigned char> &scratch,
                                unsigned int dither=0,
                                int xorigin=0, int yorigin=0, int zorigin=0);
    const void *to_native_rectangle (int xbegin, int xend, int ybegin, int yend,
                                     int zbegin, int zend,
                                     TypeDesc format, const void *data,
                                     stride_t xstride, stride_t ystride,
                                     stride_t zstride,
                                     std::vector<unsigned char> &scratch,
                                     unsigned int dither=0,
                                     int xorigin=0, int yorigin=0, int zorigin=0);

    // clang-format on

    /// Helper routine used by write_* implementations: convert data (in
    /// the given format and strides) to the "native" format of the file
    /// (described by the 'spec' member variable), in contiguous order. This
    /// requires a scratch space to be passed in so that there are no memory
    /// leaks.  Returns a span referring to the native data, which may be the
    /// original data if it was already in native format and contiguous, or
    /// it may point to the scratch space if it needed to make a copy or do
    /// conversions. For float->uint8 conversions only, if dither is
    /// nonzero, random dither will be added to reduce quantization banding
    /// artifacts; in this case, the specific nonzero dither value is used
    /// as a seed for the hash function that produces the per-pixel dither
    /// amounts, and the optional [xyz]origin parameters help it to align
    /// the pixels to the right position in the dither pattern.
    template<typename T>
    cspan<std::byte> to_native(int xbegin, int xend, int ybegin, int yend,
                               int zbegin, int zend, const image_span<T>& data,
                               std::vector<unsigned char>& scratch,
                               unsigned int dither = 0, int xorigin = 0,
                               int yorigin = 0, int zorigin = 0)
    {
        auto ispan = image_span<T>(data.data(), m_spec.nchannels, xend - xbegin,
                                   yend - ybegin, zend - zbegin);
        OIIO_DASSERT(data.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return to_native(xbegin, xend, ybegin, yend, zbegin, zend, ispan,
                         scratch, dither, xorigin, yorigin, zorigin);
    }

    /// This version of the to_native helper takes an explicit data type
    /// `format` and an image_span of generic (std::byte) data.
    cspan<std::byte> to_native(int xbegin, int xend, int ybegin, int yend,
                               int zbegin, int zend, TypeDesc format,
                               const image_span<const std::byte>& data,
                               std::vector<unsigned char>& scratch,
                               unsigned int dither = 0, int xorigin = 0,
                               int yorigin = 0, int zorigin = 0);
    // clang-format off

    /// Helper function to copy a rectangle of data into the right spot in
    /// an image-sized buffer. In addition to copying to the right place,
    /// this handles data format conversion and dither (if the spec's
    /// "oiio:dither" is nonzero, and if it's converting from a float-like
    /// type to UINT8). The buf_format describes the type of image_buffer,
    /// if it's TypeDesc::UNKNOWN it will be assumed to be spec.format.
    bool copy_to_image_buffer (int xbegin, int xend, int ybegin, int yend,
                               int zbegin, int zend, TypeDesc format,
                               const void *data, stride_t xstride,
                               stride_t ystride, stride_t zstride,
                               void *image_buffer,
                               TypeDesc buf_format = TypeDesc::UNKNOWN);
    /// Helper function to copy a tile of data into the right spot in an
    /// image-sized buffer. This is really just a wrapper for
    /// copy_to_image_buffer, passing all the right parameters to copy
    /// exactly one tile.
    bool copy_tile_to_image_buffer (int x, int y, int z, TypeDesc format,
                                    const void *data, stride_t xstride,
                                    stride_t ystride, stride_t zstride,
                                    void *image_buffer,
                                    TypeDesc buf_format = TypeDesc::UNKNOWN);

    /// @}

    /// @{
    /// @name IOProxy aids for ImageOutput implementations.
    ///
    /// This set of utility functions are not meant to be called by user code.
    /// They are protected methods of ImageOutput, and are used internally by
    /// the ImageOutput implementation to help it properly implement support
    /// of IOProxy.
    ///

    /// Get the IOProxy being used underneath.
    Filesystem::IOProxy* ioproxy();
    const Filesystem::IOProxy* ioproxy() const;

    /// Is this file currently opened (active proxy)?
    bool ioproxy_opened() const;

    /// Clear the proxy ptr, and close/destroy any "local" proxy.
    void ioproxy_clear();

    /// Retrieve any ioproxy request from the configuration hint spec, and
    /// make `m_io` point to it. But if no IOProxy is found in the config,
    /// don't overwrite one we already have.
    void ioproxy_retrieve_from_config(const ImageSpec& config);

    /// Presuming that `ioproxy_retrieve_from_config` has already been called,
    /// if `m_io` is still not set (i.e., wasn't found in the config), open a
    /// IOFile local proxy with the given read/write `mode`. Return true if a
    /// proxy is set up. If it can't be done (i.e., no proxy passed, file
    /// couldn't be opened), issue an error and return false.
    bool ioproxy_use_or_open(string_view name);

    /// Helper: write to the proxy akin to fwrite(). Return true on success,
    /// false upon failure and issue a helpful error message. NOTE: this is
    /// not the same return value as std::fwrite, which returns the number of
    /// items written.
    bool iowrite(const void* buf, size_t itemsize, size_t nitems = 1);

    /// Helper: seek the proxy, akin to fseek. Return true on success, false
    /// upon failure and issue an error message. (NOTE: this isionot the same
    /// return value as std::fseek, which returns 0 on success.)
    bool ioseek(int64_t pos, int origin = SEEK_SET);

    /// Helper: retrieve the current position of the proxy, akin to ftell.
    int64_t iotell() const;

    /// Write a formatted string to the output proxy. Return true on success,
    /// false upon failure and issue an error message.
    template<typename Str, typename... Args>
    inline bool iowritefmt(const Str& fmt, Args&&... args)
    {
        std::string s = Strutil::fmt::format(fmt, args...);
        return iowrite(s.data(), s.size());
    }

    /// @}

protected:
    ImageSpec m_spec;           // format spec of the currently open image

private:
    // PIMPL idiom -- this lets us hide details of the internals of the
    // ImageInput parent class so that changing them does not break the
    // ABI.
    class Impl;
    static void impl_deleter(Impl*);
    std::unique_ptr<Impl, decltype(&impl_deleter)> m_impl;

    void append_error(string_view message) const; // add to m_errmessage

    /// declare friend heapsize and footprint definitions
    template <typename T> friend size_t pvt::heapsize(const T&);
};



/// Memory tracking. Specializes the base memory tracking functions from memory.h.

// heapsize specialization for `ImageSpec`
template <> OIIO_API size_t pvt::heapsize<ImageSpec>(const ImageSpec&);

// heapsize and footprint specializations for `ImageInput`
template <> OIIO_API size_t pvt::heapsize<ImageInput>(const ImageInput&);
template <> OIIO_API size_t pvt::footprint<ImageInput>(const ImageInput&);

// heapsize and footprint specializations for `ImageOutput`
template <> OIIO_API size_t pvt::heapsize<ImageOutput>(const ImageOutput&);
template <> OIIO_API size_t pvt::footprint<ImageOutput>(const ImageOutput&);



// Utility functions

/// `OIIO::shutdown` prepares OpenImageIO for shutdown. Before exiting an 
/// application that utilizes OpenImageIO the `OIIO::shutdown` function must be 
/// called, which will perform shutdown of any running thread-pools. Failing 
/// to call `OIIO::shutdown` could lead to a sporadic dead-lock during 
/// application shutdown on certain platforms such as Windows. 
OIIO_API void shutdown ();

/// Returns a numeric value for the version of OpenImageIO, 10000 for each
/// major version, 100 for each minor version, 1 for each patch.  For
/// example, OpenImageIO 1.2.3 would return a value of 10203. One example of
/// how this is useful is for plugins to query the version to be sure they
/// are linked against an adequate version of the library.
OIIO_API int openimageio_version ();

/// Is there a pending global error message waiting to be retrieved?
OIIO_API bool has_error();

/// Returns any error string describing what went wrong if
/// `ImageInput::create()` or `ImageOutput::create()` failed (since in such
/// cases, the ImageInput or ImageOutput itself does not exist to have its
/// own `geterror()` function called). This function returns the last error
/// for this particular thread, and clear the pending error message unless
/// `clear` is false; separate threads will not clobber each other's global
/// error messages.
OIIO_API std::string geterror(bool clear = true);

/// `OIIO::attribute()` sets a global attribute (i.e., a property or
/// option) of OpenImageIO. The `name` designates the name of the attribute,
/// `type` describes the type of data, and `val` is a pointer to memory
/// containing the new value for the attribute.
///
/// If the name is known, valid attribute that matches the type specified,
/// the attribute will be set to the new value and `attribute()` will return
/// `true`.  If `name` is not recognized, or if the types do not match
/// (e.g., `type` is `TypeFloat` but the named attribute is a string), the
/// attribute will not be modified, and `attribute()` will return `false`.
///
/// The following are the recognized attributes:
///
/// - `string options`
///
///    This catch-all is simply a comma-separated list of `name=value`
///    settings of named options, which will be parsed and individually set.
///    For example,
///
///        OIIO::attribute ("options", "threads=4,log_times=1");
///
///    Note that if an option takes a string value that must itself contain
///    a comma, it is permissible to enclose the value in either 'single'
///    or "double" quotes.
///
/// - `int threads`
///
///    How many threads to use for operations that can be sped up by being
///    multithreaded. (Examples: simultaneous format conversions of multiple
///    scanlines read together, or many ImageBufAlgo operations.) The
///    default is 0, meaning to use the full available hardware concurrency
///    detected.
///
///    Situations where the main application logic is essentially single
///    threaded (i.e., one top-level call into OIIO at a time) should leave
///    this at the default value, or some reasonable number of cores, thus
///    allowing lots of threads to fill the cores when OIIO has big tasks to
///    complete. But situations where you have many threads at the
///    application level, each of which is expected to be making separate
///    OIIO calls simultaneously, should set this to 1, thus having each
///    calling thread do its own work inside of OIIO rather than spawning
///    new threads with a high overall "fan out."
///
/// - `int exr_threads`
///
///    Sets the internal OpenEXR thread pool size. The default is to use as
///    many threads as the amount of hardware concurrency detected. Note
///    that this is separate from the OIIO `"threads"` attribute.
///
/// - `string font_searchpath`
///
///    Colon-separated (or semicolon-separated) list of directories to search
///    if fonts are needed. (Such as for `ImageBufAlgo::render_text()`.)
///
/// - `int use_tbb`
///
///    If nonzero and TBB was found and support configured when OIIO was
///    compiled, parallel processing within OIIO (including inside the
///    parallel.h utilities) will try to use TBB by default where possible.
///    If zero, they will try to use OIIO's native thread pool even if TBB
///    is available.
///
/// - `string plugin_searchpath`
///
///    Colon-separated (or semicolon-separated) list of directories to search
///    for dynamically-loaded format plugins.
///
/// - `int try_all_readers`
///
///    When nonzero (the default), a call to `ImageInput::create()` or
///    `ImageInput::open()` that does not succeed in opening the file with the
///    format reader implied by the file extension will try all available
///    format readers to see if one of them can open the file. If this is
///    zero, the only reader that will be tried is the one implied by the file
///    extension.
///
/// - `int read_chunk`
///
///    When performing a `read_image()`, this is the number of scanlines it
///    will attempt to read at a time (some formats are more efficient when
///    reading and decoding multiple scanlines).  The default is 256. The
///    special value of 0 indicates that it should try to read the whole
///    image if possible.
///
/// - `float[] missingcolor`, `string missingcolor`
///
///    This attribute may either be an array of float values, or a string
///    containing a comma-separated list of the values. Setting this option
///    globally is equivalent to always passing an `ImageInput`
///    open-with-configuration hint `"oiio:missingcolor"` with the value.
///
///    When set, it gives some `ImageInput` readers the option of ignoring
///    any *missing* tiles or scanlines in the file, and instead of treating
///    the read failure of an individual tile as a full error, will
///    interpret is as an intentionally missing tile and proceed by simply
///    filling in the missing pixels with the color specified. If the first
///    element is negative, it will use the absolute value, but draw
///    alternating diagonal stripes of the color. For example,
///
///        float missing[4] = { -1.0, 0.0, 0.0, 0.0 }; // striped red
///        OIIO::attribute ("missingcolor", TypeDesc("float[4]"), &missing);
///
///    Note that only some file formats support files with missing tiles or
///    scanlines, and this is only taken as a hint. Please see
///    chap-bundledplugins_ for details on which formats accept a
///    `"missingcolor"` configuration hint.
///
/// - `int debug`
///
///    When nonzero, various debug messages may be printed. The default is 0
///    for release builds, 1 for DEBUG builds (values > 1 are for OIIO
///    developers to print even more debugging information), This attribute
///    but also may be overridden by the OPENIMAGEIO_DEBUG environment
///    variable.
///
/// - `int tiff:half`
///
///    When nonzero, allows TIFF to write `half` pixel data. N.B. Most apps
///    may not read these correctly, but OIIO will. That's why the default
///    is not to support it.
///
/// - `int dds:bc5normal`
///
///    When nonzero, treats BC5/ATI2 format files as normal maps (loads as
///    3 channels, computes blue from red and green). Default is 0.
///
/// - `int openexr:core`
///
///    When nonzero, use the new "OpenEXR core C library" when available,
///    for OpenEXR >= 3.1. This is experimental, and currently defaults to 0.
///
/// - `int jpeg:com_attributes`
///
///    When nonzero, try to parse JPEG comment blocks as key-value attributes,
///    and only set ImageDescription if the parsing fails. Otherwise, always
///    set ImageDescription to the first comment block. Default is 1.
///
/// - `int png:linear_premult` (0)
///
///    If nonzero, will convert perform any necessary premultiplication by
///    alpha steps needed of the PNG reader/writer in a linear color space.
///    If zero (the default), any needed premultiplication will happen
///    directly on the values, even if they are sRGB or gamma-corrected.
///    For more information, please see OpenImageIO's documentation on the
///    built-in PNG format support.
///
/// - `int limits:channels` (1024)
///
///    When nonzero, the maximum number of color channels in an image. Image
///    files whose headers indicate they have more channels might be assumed
///    to be corrupted or malicious files.  In situations when more channels
///    are expected to be encountered, the application should raise this
///    limit. The default is 1024 channels.
///
/// - `int limits:imagesize_MB` (32768)
///
///    When nonzero, the maximum expected size in MB of the uncompressed pixel
///    data of a single 2D image. Images whose headers indicate that they are
///    larger than this might be assumed to be corrupted or malicious files.
///    The default is 32768 (32 GB of uncompressed pixel data -- equivalent to
///    64k x 64k x 4 channel x half), or the total amount of total physical
///    memory available to the running process, whichever is smaller. In
///    situations when images larger than this are expected to be encountered,
///    you should raise this limit. Setting the limit to 0 means having no
///    limit.
///
/// - `int log_times` (0)
///
///    When the `"log_times"` attribute is nonzero, `ImageBufAlgo` functions
///    are instrumented to record the number of times they were called and
///    the total amount of time spent executing them. It can be overridden
///    by environment variable `OPENIMAGEIO_LOG_TIMES`. The totals will be
///    recorded and can be retrieved as a string by using
///    `OIIO::getattribute("timing_report", ...)`. Additionally, if the
///    value is 2 or more, the timing report will be printed to `stdout`
///    upon application exit (not advised in contexts where it isn't ok to
///    print to the terminal via stdout, such as GUI apps or libraries).
///
///    When enabled, there is a slight runtime performance cost due to
///    checking the time at the start and end of each of those function
///    calls, and the locking and recording of the data structure that holds
///    the log information. When the `log_times` attribute is disabled,
///    there is no additional performance cost.
///
/// - `oiio:print_uncaught_errors` (1)
///
///   If nonzero, upon program exit, any error messages that would have been
///   retrieved by a call to `OIIO::geterror()`, but never were, will be
///   printed to stdout. While this may seem chaotic, we are presuming that
///   any well-written library or application will proactively check error
///   codes and retrieve errors, so this will never print anything upon exit.
///   But for less sophisticated applications (or users), this is very useful
///   for forcing display of error messages so that users can see relevant
///   errors even if they never check them explicitly, thus self-diagnose
///   their troubles before asking the project dev deam for help. Advanced
///   users who for some reason desire to neither retrieve errors themselves
///   nor have them printed in this manner can disable the behavior by setting
///   this attribute to 0.
///
/// - `imagebuf:print_uncaught_errors` (1)
///
///   If nonzero, an `ImageBuf` upon destruction will print any error messages
///   that were never retrieved by its `geterror()` method. While this may
///   seem chaotic, we are presuming that any well-written library or
///   application will proactively check error codes and retrieve errors, so
///   will never print anything upon destruction. But for less sophisticated
///   applications (or users), this is very useful for forcing display of
///   error messages so that users can see relevant errors even if they never
///   check them explicitly, thus self-diagnose their troubles before asking
///   the project dev deam for help. Advanced users who for some reason desire
///   to neither retrieve errors themselves nor have them printed in this
///   manner can disable the behavior by setting this attribute to 0.
///
/// - `imagebuf:use_imagecache` (0)
///
///   If nonzero, an `ImageBuf` that references a file but is not given an
///   ImageCache will read the image through the default ImageCache.
///
/// - `imageinput:strict` (int: 0)
///
///   If zero (the default), ImageInput readers will try to be very tolerant
///   of minor flaws or invalidity in image files being read, if possible just
///   skipping something erroneous it encounters in the hopes that the rest of
///   the file's data will be usable. If nonzero, anything clearly invalid in
///   the file will be understood to be a corrupt file with unreliable data at
///   best, and possibly malicious construction, and so will not attempt to
///   further decode anything in the file. This may be a better choice to
///   enable globally in an environment where security is a higher priority
///   than being tolerant of partially broken image files.
OIIO_API bool attribute(string_view name, TypeDesc type, const void* val);

/// Shortcut attribute() for setting a single integer.
inline bool attribute (string_view name, int val) {
    return attribute (name, TypeInt, &val);
}
/// Shortcut attribute() for setting a single float.
inline bool attribute (string_view name, float val) {
    return attribute (name, TypeFloat, &val);
}
/// Shortcut attribute() for setting a single string.
inline bool attribute (string_view name, string_view val) {
    std::string valstr = val;
    const char *s = valstr.c_str();
    return attribute (name, TypeString, &s);
}

/// Get the named global attribute of OpenImageIO, store it in `*val`.
/// Return `true` if found and it was compatible with the type specified,
/// otherwise return `false` and do not modify the contents of `*val`.  It
/// is up to the caller to ensure that `val` points to the right kind and
/// size of storage for the given type.
///
/// In addition to being able to retrieve all the attributes that are
/// documented as settable by the `OIIO::attribute()` call, `getattribute()`
/// can also retrieve the following read-only attributes:
///
/// - `string version`
///
///   The version designation of the OpenImageIO library, as a string.
///
/// - `string format_list`
/// - `string input_format_list`
/// - `string output_format_list`
///
///   A comma-separated list of all the names of, respectively, all
///   supported image formats, all formats accepted as inputs, and all
///   formats accepted as outputs.
///
/// - `string extension_list`
///
///   For each format, the format name, followed by a colon, followed by a
///   comma-separated list of all extensions that are presumed to be used
///   for that format.  Semicolons separate the lists for formats.  For
///   example,
///
///        "tiff:tif;jpeg:jpg,jpeg;openexr:exr"
///
/// - `string library_list`
///
///   For each format that uses a dependent library, the format name,
///   followed by a colon, followed by the name and version of the
///   dependency. Semicolons separate the lists for formats.  For example,
///
///        "tiff:LIBTIFF 4.0.4;gif:gif_lib 4.2.3;openexr:OpenEXR 2.2.0"
///
/// - `string font_list`
/// - `string font_file_list`
/// - `string font_dir_list`
///
///   A semicolon-separated list of, respectively, all the fonts that
///   OpenImageIO can find, all the font files that OpenImageIO can find (with
///   full paths), and all the directories that OpenImageIO will search for
///   fonts.  (Added in OpenImageIO 2.5)
///
/// - `string font_family_list`
///
///   A semicolon-separated list of all the font family names that
///   OpenImageIO can find.  (Added in OpenImageIO 3.0)
///
/// - `string font_style_list:family`
///
///   A semicolon-separated list of all the font style names that
///   belong to the given font family.  (Added in OpenImageIO 3.0)
///
/// - `string font_filename:family:style`
///
///   The font file (with full path) that defines the given font
///   family and style.  (Added in OpenImageIO 3.0)
///
/// - `string filter_list`
///
///   A semicolon-separated list of all built-in 2D filters. (Added in
///   OpenImageIO 2.5.9)
///
/// - int64_t IB_local_mem_current
/// - int64_t IB_local_mem_peak
///
///   Current and peak size (in bytes) of how much memory was consumed by
///   ImageBufs that owned their own allcoated local pixel buffers. (Added in
///   OpenImageIO 2.5.)
///
/// - float IB_total_open_time
/// - float IB_total_image_read_time
///
///   Total amount of time (in seconds) that ImageBufs spent opening
///   (including reading header information) and reading pixel data from files
///   that they opened and read themselves (that is, excluding I/O from IBs
///   that were backed by ImageCach.  (Added in OpenImageIO 2.5.)
///
/// - `string opencolorio_version`
///
///   Returns the version (such as "2.2.0") of OpenColorIO that is used by
///   OpenImageiO, or "0.0.0" if no OpenColorIO support has been enabled.
///   (Added in OpenImageIO 2.4.6)
///
/// - `string hw:simd`
/// - `string build:simd` (read-only)
///
///   A comma-separated list of hardware CPU features for SIMD (and some
///   other things). The `"build:simd"` attribute is similarly a list of
///   which features this build of OIIO was compiled to support.
///
///   These were added in OpenImageIO 1.8. The `"build:simd"` attribute was
///   added added in OpenImageIO 2.5.8 as a preferred synonym for what
///   previously was called `"oiio:simd"`, which is now deprecated.
///
/// - `string build:platform` (read-only)
///
///   THe name of the OS and CPU architecture that OpenImageIO was built
///   for (e.g., `"Linux/x86_64"`).  (Added in OpenImageIO 2.5.8.)
///
/// - `string build:compiler` (read-only)
///
///   THe name and version of the compiler used to build OIIO.
///   (Added in OpenImageIO 2.5.8.)
///
/// - `string build:dependencies` (read-only)
///
///   List of library dependencieis (where known) and versions, separatd by
///   semicolons. (Added in OpenImageIO 2.5.8.)
///
/// - `int resident_memory_used_MB`
///
///   This read-only attribute can be used for debugging purposes to report
///   the approximate process memory used (resident) by the application, in
///   MB.
///
/// - `string timing_report`
///
///    Retrieving this attribute returns the timing report generated by the
///    `log_timing` attribute (if it was enabled). The report is sorted
///    alphabetically and for each named instrumentation region, prints the
///    number of times it executed, the total runtime, and the average per
///    call, like this:
///
///        IBA::computePixelStats        2   2.69ms  (avg   1.34ms)
///        IBA::make_texture             1  74.05ms  (avg  74.05ms)
///        IBA::mul                      8   2.42ms  (avg   0.30ms)
///        IBA::over                    10  23.82ms  (avg   2.38ms)
///        IBA::resize                  20   0.24s   (avg  12.18ms)
///        IBA::zero                     8   0.66ms  (avg   0.08ms)
///
OIIO_API bool getattribute(string_view name, TypeDesc type, void* val);

/// Shortcut getattribute() for retrieving a single integer.
/// The value is placed in `val`, and the function returns `true` if the
/// attribute was found and was legally convertible to an int.
inline bool getattribute (string_view name, int &val) {
    return getattribute (name, TypeInt, &val);
}
/// Shortcut getattribute() for retrieving a single float.
/// The value is placed in `val`, and the function returns `true` if the
/// attribute was found and was legally convertible to a float.
inline bool getattribute (string_view name, float &val) {
    return getattribute (name, TypeFloat, &val);
}
/// Shortcut getattribute() for retrieving a single string as a
/// `std::string`. The value is placed in `val`, and the function returns
/// `true` if the attribute was found.
inline bool getattribute (string_view name, std::string &val) {
    ustring s;
    bool ok = getattribute (name, TypeString, &s);
    if (ok)
        val = s.string();
    return ok;
}
/// Shortcut getattribute() for retrieving a single string as a `char*`.
inline bool getattribute (string_view name, char **val) {
    return getattribute (name, TypeString, val);
}
/// Shortcut getattribute() for retrieving a single integer, with a supplied
/// default value that will be returned if the attribute is not found or
/// could not legally be converted to an int.
inline int get_int_attribute (string_view name, int defaultval=0) {
    int val;
    return getattribute (name, TypeInt, &val) ? val : defaultval;
}
/// Shortcut getattribute() for retrieving a single float, with a supplied
/// default value that will be returned if the attribute is not found or
/// could not legally be converted to a float.
inline float get_float_attribute (string_view name, float defaultval=0) {
    float val;
    return getattribute (name, TypeFloat, &val) ? val : defaultval;
}
/// Shortcut getattribute() for retrieving a single string, with a supplied
/// default value that will be returned if the attribute is not found.
inline string_view get_string_attribute (string_view name,
                                 string_view defaultval = string_view()) {
    ustring val;
    return getattribute (name, TypeString, &val) ? string_view(val) : defaultval;
}


/// Set the metadata of the `spec` to presume that color space is `name` (or
/// to assume nothing about the color space if `name` is empty). The core
/// operation is to set the "oiio:ColorSpace" attribute, but it also removes
/// or alters several other attributes that may hint color space in ways that
/// might be contradictory or no longer true. This uses the current default
/// color config to adjudicate color space name equivalencies.
///
/// @version 3.0
OIIO_API void set_colorspace(ImageSpec& spec, string_view name);

/// Set the metadata of the `spec` to reflect Rec709 color primaries and the
/// given gamma. The core operation is to set the "oiio:ColorSpace" attribute,
/// but it also removes or alters several other attributes that may hint color
/// space in ways that might be contradictory or no longer true. This uses the
/// current default color config to adjudicate color space name equivalencies.
///
/// @version 3.0
OIIO_API void set_colorspace_rec709_gamma(ImageSpec& spec, float gamma);


/// Are the two named color spaces equivalent, based on the default color
/// config in effect?
///
/// @version 3.0
OIIO_API bool equivalent_colorspace(string_view a, string_view b);


/// Register the input and output 'create' routines and list of file
/// extensions for a particular format.
OIIO_API void declare_imageio_format (const std::string &format_name,
                                      ImageInput::Creator input_creator,
                                      const char **input_extensions,
                                      ImageOutput::Creator output_creator,
                                      const char **output_extensions,
                                      const char *lib_version);

/// Is `name` one of the known format names?
OIIO_API bool is_imageio_format_name(string_view name);

/// Utility: Parse the "extension_list" attribute into a std::map string
/// keys are the names of all the file formats OIIO knows how to read, and
/// whose values are vectors of strings of the file extensions associated
/// with the file format. (Added in OIIO 2.2.13.)
inline std::map<std::string, std::vector<std::string>>
get_extension_map()
{
    std::map<std::string, std::vector<std::string>> map;
    auto all_extensions = OIIO::get_string_attribute("extension_list");
    for (auto oneformat : OIIO::Strutil::splitsv(all_extensions, ";")) {
        auto format_exts = OIIO::Strutil::splitsv(oneformat, ":", 2);
        if (format_exts.size() != 2)
            continue;   // something went wrong
        map[format_exts[0]] = OIIO::Strutil::splits(format_exts[1], ",");
    }
    return map;
}



/// Helper function: convert contiguous data between two arbitrary pixel
/// data types (specified by TypeDesc's). Return true if ok, false if it
/// didn't know how to do the conversion.  If dst_type is UNKNOWN, it will
/// be assumed to be the same as src_type.
///
/// The conversion is of normalized (pixel-like) values -- for example
/// 'UINT8' 255 will convert to float 1.0 and vice versa, not float 255.0.
/// If you want a straight C-like data cast conversion (e.g., uint8 255 ->
/// float 255.0), then you should prefer the un-normalized convert_type()
/// utility function found in typedesc.h.
OIIO_API bool convert_pixel_values (TypeDesc src_type, const void *src,
                                    TypeDesc dst_type, void *dst, int n = 1);

/// Helper function: copy values from spans `src` to `dst`, converting between
/// types as appropriate. Return true if ok, false if it didn't know how to do
/// the conversion.
///
/// The conversion is of normalized (pixel-like) values -- for example 'UINT8'
/// 255 will convert to float 1.0 and vice versa, not float 255.0. If you want
/// a straight C-like data cast conversion (e.g., uint8 255 -> float 255.0),
/// then you should prefer the un-normalized convert_type() utility function
/// found in typedesc.h.
template<typename SrcType, typename DstType>
bool
convert_pixel_values(cspan<SrcType> src, span<DstType> dst)
{
    OIIO_DASSERT(dst.size() >= src.size());
    return convert_pixel_values(TypeDescFromC_v<SrcType>, src.data(),
                                TypeDescFromC_v<DstType>, dst.data(),
                                std::min(dst.size(), src.size()));
}


/// Helper routine for data conversion: Convert an image of nchannels x
/// width x height x depth from src to dst.  The src and dst may have
/// different data formats and layouts.  Clever use of this function can
/// not only exchange data among different formats (e.g., half to 8-bit
/// unsigned), but also can copy selective channels, copy subimages,
/// etc.  If you're lazy, it's ok to pass AutoStride for any of the
/// stride values, and they will be auto-computed assuming contiguous
/// data.  Return true if ok, false if it didn't know how to do the
/// conversion.
OIIO_API bool convert_image (int nchannels, int width, int height, int depth,
                             const void *src, TypeDesc src_type,
                             stride_t src_xstride, stride_t src_ystride,
                             stride_t src_zstride,
                             void *dst, TypeDesc dst_type,
                             stride_t dst_xstride, stride_t dst_ystride,
                             stride_t dst_zstride);

/// Helper routine for data conversion: Convert an image described by
/// image_span `src` into image_span `dst`, which must be the same dimensions
/// but possibly differing data type and strides.  Clever use of this function
/// can not only exchange data among different formats (e.g., half to 8-bit
/// unsigned), but also can copy selective channels, copy subimages, etc.
/// Return true if ok, false if it didn't know how to do the conversion.
template<typename SrcType, typename DstType>
bool
convert_image(const image_span<SrcType>& src, const image_span<DstType>& dst)
{
    // For now, just implement by wrapping the pointer-based version.
    OIIO_DASSERT(src.nchannels() == dst.nchannels()
                 && src.width() == dst.width() && src.height() == dst.height()
                 && src.depth() == dst.depth());
    return convert_image(src.nchannels(), src.width(), src.height(),
                         src.depth(), src.data(), TypeDescFromC_v<SrcType>,
                         src.xstride(), src.ystride(), src.zstride(),
                         dst.data(), TypeDescFromC_v<DstType>, dst.xstride(),
                         dst.ystride(), dst.zstride());
}


/// Helper routine for data conversion: Convert an image described by
/// image_span `src` into image_span `dst`, which must be the same dimensions
/// but possibly differing data type and strides. The data types are passed as
/// `TypeDesc`, and the spans are untyped bytes that provide the dimensions
/// and memory layout.
inline bool
convert_image(const image_span<const std::byte>& src, TypeDesc src_type,
              const image_span<std::byte>& dst, TypeDesc dst_type)
{
    // For now, just implement by wrapping the pointer-based version.
    OIIO_DASSERT(src.nchannels() == dst.nchannels()
                 && src.width() == dst.width() && src.height() == dst.height()
                 && src.depth() == dst.depth());
    OIIO_DASSERT(src_type.size() == src.chansize()
                 && dst_type.size() == dst.chansize());
    return convert_image(src.nchannels(), src.width(), src.height(),
                         src.depth(), src.data(), src_type, src.xstride(),
                         src.ystride(), src.zstride(), dst.data(), dst_type,
                         dst.xstride(), dst.ystride(), dst.zstride());
}



/// A version of convert_image that will break up big jobs into multiple
/// threads.
OIIO_API bool parallel_convert_image (
               int nchannels, int width, int height, int depth,
               const void *src, TypeDesc src_type,
               stride_t src_xstride, stride_t src_ystride,
               stride_t src_zstride,
               void *dst, TypeDesc dst_type,
               stride_t dst_xstride, stride_t dst_ystride,
               stride_t dst_zstride, int nthreads=0);

/// A version of convert_image that will break up big jobs into multiple
/// threads. The data types are taken from the spans.
template<typename SrcType, typename DstType>
bool
parallel_convert_image(const image_span<SrcType>& src,
                       const image_span<DstType>& dst, int nthreads = 0)
{
    // For now, just implement by wrapping the pointer-based version.
    OIIO_DASSERT(src.nchannels() == dst.nchannels()
                 && src.width() == dst.width() && src.height() == dst.height()
                 && src.depth() == dst.depth());
    return parallel_convert_image(src.nchannels(), src.width(), src.height(),
                                  src.depth(), src.data(),
                                  TypeDescFromC_v<SrcType>, src.xstride(),
                                  src.ystride(), src.zstride(), dst.data(),
                                  TypeDescFromC_v<SrcType>, dst.xstride(),
                                  dst.ystride(), dst.zstride(), nthreads);
}

/// A version of convert_image that will break up big jobs into multiple
/// threads. The data types are passed as `TypeDesc`, and the spans are
/// untyped bytes that provide the dimensions and memory layout.
inline bool
parallel_convert_image(const image_span<const std::byte>& src,
                       TypeDesc src_type, const image_span<std::byte>& dst,
                       TypeDesc dst_type, int nthreads = 0)
{
    // For now, just implement by wrapping the pointer-based version.
    OIIO_DASSERT(src.nchannels() == dst.nchannels()
                 && src.width() == dst.width() && src.height() == dst.height()
                 && src.depth() == dst.depth());
    OIIO_DASSERT(src_type.size() == src.chansize()
                 && dst_type.size() == dst.chansize());
    return parallel_convert_image(src.nchannels(), src.width(), src.height(),
                                  src.depth(), src.data(),
                                  src_type, src.xstride(),
                                  src.ystride(), src.zstride(), dst.data(),
                                  dst_type, dst.xstride(),
                                  dst.ystride(), dst.zstride(), nthreads);
}



/// Add random [-ditheramplitude,ditheramplitude] dither to the color channels
/// of the image.  Dither will not be added to the alpha or z channel.  The
/// image origin and dither seed values allow a reproducible (or variable)
/// dither pattern.  If the strides are set to AutoStride, they will be
/// assumed to be contiguous floats in data of the given dimensions.
OIIO_API void add_dither (int nchannels, int width, int height, int depth,
                          float *data,
                          stride_t xstride, stride_t ystride, stride_t zstride,
                          float ditheramplitude,
                          int alpha_channel = -1, int z_channel = -1,
                          unsigned int ditherseed = 1,
                          int chorigin=0, int xorigin=0,
                          int yorigin=0, int zorigin=0);

/// Convert unassociated to associated alpha by premultiplying all color
/// (non-alpha, non-z) channels by alpha. The nchannels, width, height, and
/// depth parameters describe the "shape" of the image data (along with
/// optional stride overrides). The chbegin/chend describe which range of
/// channels to actually premultiply.
OIIO_API void premult (int nchannels, int width, int height, int depth,
                       int chbegin, int chend,
                       TypeDesc datatype, void *data, stride_t xstride,
                       stride_t ystride, stride_t zstride,
                       int alpha_channel = -1, int z_channel = -1);

/// Helper routine for data conversion: Copy an image of nchannels x
/// width x height x depth from src to dst.  The src and dst may have
/// different data layouts, but must have the same data type.  Clever
/// use of this function can change layouts or strides, copy selective
/// channels, copy subimages, etc.  If you're lazy, it's ok to pass
/// AutoStride for any of the stride values, and they will be
/// auto-computed assuming contiguous data.  Return true if ok, false if
/// it didn't know how to do the conversion.
///
/// Note: when possible, the image_span based copy_image should be preferred.
/// This one will eventually be deprecated.
OIIO_API bool copy_image (int nchannels, int width, int height, int depth,
                          const void *src, stride_t pixelsize,
                          stride_t src_xstride, stride_t src_ystride,
                          stride_t src_zstride,
                          void *dst, stride_t dst_xstride,
                          stride_t dst_ystride, stride_t dst_zstride);

/// Helper routine for data conversion: Copy the contents of the `src` image
/// span to `dst` that has exactly the same dimensions but may have different
/// strides. Return true if ok, false if it couldn't do it. (Reserved for
/// future use; currently is always succeeds)
template<typename D, size_t Drank, typename S, size_t Srank>
bool copy_image(const image_span<D, Drank>& dst,
                const image_span<S, Srank>& src)
{
    // Arbitrary types are handled by just converting to generic byte
    // image_spans.
    return copy_image(as_image_span_writable_bytes(dst),
                      as_image_span_bytes(src));
}

/// copy_image base case: generic span of bytes.
OIIO_API bool
copy_image(const image_span<std::byte> &dst,
           const image_span<const std::byte>& src);



/// Helper: manufacture a span given an image pointer, format, size, and
/// strides. Use with caution! This is making a lot of assumptions that the
/// data pointer really does point to memory that's ok to access according to
/// the sizes and strides you give.
OIIO_API span<std::byte>
span_from_buffer(void* data, TypeDesc format, int nchannels, int width,
                 int height, int depth, stride_t xstride = AutoStride, stride_t ystride = AutoStride,
                 stride_t zstride = AutoStride);
OIIO_API cspan<std::byte>
cspan_from_buffer(const void* data, TypeDesc format, int nchannels, int width,
                 int height, int depth, stride_t xstride = AutoStride, stride_t ystride = AutoStride,
                 stride_t zstride = AutoStride);


// All the wrap_foo functions implement a wrap mode, wherein coord is
// altered to be origin <= coord < origin+width.  The return value
// indicates if the resulting wrapped value is valid (example, for
// wrap_black, values outside the region are invalid and do not modify
// the coord parameter).
OIIO_API bool wrap_black (int &coord, int origin, int width);
OIIO_API bool wrap_clamp (int &coord, int origin, int width);
OIIO_API bool wrap_periodic (int &coord, int origin, int width);
OIIO_API bool wrap_periodic_pow2 (int &coord, int origin, int width);
OIIO_API bool wrap_mirror (int &coord, int origin, int width);

// Typedef for the function signature of a wrap implementation.
typedef bool (*wrap_impl) (int &coord, int origin, int width);


/// `debug(format, ...)` prints debugging message when attribute "debug" is
/// nonzero, which it is by default for DEBUG compiles or when the
/// environment variable OPENIMAGEIO_DEBUG is set. This is preferred to raw
/// output to stderr for debugging statements.
OIIO_API void debug (string_view str);

/// debug output with `std::format` conventions.
/// This is just a wrapped synonym for Strutil::debug().
template<typename... Args>
void debugfmt (const char* fmt, Args&&... args)
{
    Strutil::debug(fmt, std::forward<Args>(args)...);
}

namespace pvt {
// For internal use - use errorfmt() below for a nicer interface.
OIIO_API void append_error(string_view message);
}

/// error logging (mostly for OIIO internals) with `std::format` conventions.
template<typename... Args>
inline void errorfmt(const char* fmt, Args&&... args)
{
    pvt::append_error(string_view(Strutil::fmt::format(fmt, args...)));
}

/// Internal function to log time recorded by an OIIO::timer(). It will only
/// trigger a read of the time if the "log_times" attribute is set or the
/// OPENIMAGEIO_LOG_TIMES env variable is set.
OIIO_API void log_time(string_view key, const Timer& timer, int count = 1);

// to force correct linkage on some systems
OIIO_API void _ImageIO_force_link ();


//////////////////////////////////////////////////////////////////////////
// DEPRECATED things
//
// These are all hidden from ocumentation and internal use, and should trigger
// deprecation warnings if used externally. They will most likely be removed
// entirely before the final release of OIIO 3.0.
//
#if !defined(OIIO_INTERNAL) && !defined(OIIO_DOXYGEN)

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2, 1, 0)
// DEPRECATED(2.1): old name
OIIO_DEPRECATED("use convert_pixel_values instead (2.1)")
inline bool convert_types (TypeDesc src_type, const void *src,
                           TypeDesc dst_type, void *dst, int n = 1) {
    return convert_pixel_values (src_type, src, dst_type, dst, n);
}
#endif

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2, 6, 3)
// (Unfortunate old synonym)
template<typename... Args>
OIIO_DEPRECATED("use `debugfmt` instead")
inline void fmtdebug (const char* fmt, const Args&... args)
{
    debug (Strutil::fmt::format(fmt, args...));
}

/// debug output with printf conventions.
template<typename... Args>
OIIO_DEPRECATED("use `debugfmt` instead, with std::format-like arguments (3.0)")
inline void debugf (const char* fmt, const Args&... args)
{
    debug (Strutil::sprintf(fmt, args...));
}

/// debug output with the same conventions as Strutil::format. Beware, this
/// will change one day!
template<typename T1, typename... Args>
OIIO_DEPRECATED("use `debugfmt` instead, with std::format-like arguments (3.0)")
inline void debug (const char* fmt, const T1& v1, const Args&... args)
{
    debug (Strutil::format(fmt, v1, args...));
}
#endif

#endif
//
//////////////////////////////////////////////////////////////////////////

OIIO_NAMESPACE_END


#if FMT_VERSION >= 100000
FMT_BEGIN_NAMESPACE
template<> struct formatter<OIIO::ROI> : ostream_formatter {};
FMT_END_NAMESPACE
#endif
