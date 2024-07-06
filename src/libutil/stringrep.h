// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include <string>

#include <OpenImageIO/platform.h>

// See https://devblogs.microsoft.com/oldnewthing/20240510-00/?p=109742 for a
// detailed explanation of different compilers' string implementations. Some
// of the struct definitions below are derived from there.
//
// More info here, too (though possibly outdated):
// https://shaharmike.com/cpp/std-string/


OIIO_NAMESPACE_BEGIN
namespace pvt {

// Bare bones data layout of std::string in various compilers and standard
// libraries.


// gcc libstdc++ 4.x and earlier (was not compliant with C++11), and later
// versions when _GLIBCXX_USE_CXX11_ABI defined.
struct gcc_old_string_rep {
    // See, for example:
    // https://gcc.gnu.org/onlinedocs/gcc-4.6.2/libstdc++/api/a00259.html
    struct Rep {
        size_t size_;
        size_t capacity_;
        int refcount_;
        // followed immediately by:
        // char chars[size];
    };

    std::string str_;

    // So here's how old gcc worked: std::string.c_str() points to the chars
    // field of a Rep. So
    const Rep* rep()
    {
        return (const Rep*)((const char*)str_.c_str() - sizeof(Rep));
    }

    bool is_large() const { return false; }  // never!
    auto data() const { return str_.data(); }
    auto size() const { return str_.size(); /* str_.rep()->size_ */ }
    auto capacity() const
    {
        return str_.capacity(); /* str_.rep()->capacity_ */
    }
};



// Modern C++11 compliant gcc libstdc++ (gcc 5 and later) when the old
// ABI is not being forced.
struct gcc_new_string_rep {
    char* ptr_;
    size_t size_;
    union {
        size_t capacity_;
        char buf_[16];
    };

    bool is_large() const { return ptr_ != buf_; }
    auto data() const { return ptr_; }
    auto size() const { return size_; }
    auto capacity() const { return is_large() ? capacity_ : 15; }
};



// MSVC's string representation
struct msvc_string_rep {
    union {
        char* ptr_;
        char buf_[16];
    };
    size_t size_;
    size_t capacity_;

    bool is_large() const { return capacity_ > 15; }
    auto data() const { return is_large() ? ptr_ : buf_; }
    auto size() const { return size_; }
    auto capacity() const { return capacity_; }
};



// Modern clang libc++ string representation. Note that in many toolchans,
// clang is the compiler but it's still using the libstdc++ standard library.
struct clang_string_rep {
    union {
        struct {
            size_t capacity_;
            size_t size_;
            char* ptr_;
        } large;

        struct {
            unsigned char is_large_ : 1;
            unsigned char size_ : 7;
            char buf[sizeof(large) - 1];
        } small;
    };

    bool is_large() const { return small.is_large_; }
    auto data() const { return is_large() ? large.ptr_ : small.buf; }
    auto size() const { return is_large() ? large.size_ : small.size_; }
    auto capacity() const
    {
        return is_large() ? large.capacity_ - 1: sizeof(large) - 2;
    }
};


// "Alternate" clang libc++ string representation. It's not clear when or if
// this is still used, but it does still appear as a possible layout in the
// clang libc++ header source code.
struct clang_alternate_string_rep  // old version of clang
{
    struct large {
        char* ptr_;
        size_t size_;
        size_t capacity_ : sizeof(size_t) * 8 - 1;
        unsigned char is_large_ : 1;
    };

    enum {
        min_cap = (sizeof(large) - 1) / sizeof(char) > 2
                      ? (sizeof(large) - 1) / sizeof(char)
                      : 2
    };

    struct small {
        char buf_[min_cap];
        unsigned char padding_[sizeof(char) - 1];
        unsigned char size_ : 7;
        unsigned char is_large_ : 1;
    };

    union {
        large large_;
        small small_;
    };

    bool is_large() const { return large_.is_large_; }
    auto data() const { return is_large() ? large_.ptr_ : small_.buf_; }
    auto size() const { return is_large() ? large_.size_ : small_.size_; }
    auto capacity() const
    {
        return is_large() ? large_.capacity_ : sizeof(large) - 2;
    }
};



//
// Now we define `string_rep` to be an alias to the appropriate string
// representation for the current compiler and standard library.
//

#if defined(_LIBCPP_ALTERNATE_STRING_LAYOUT)
using string_rep = clang_alternate_string_rep;
#elif defined(_LIBCPP_VERSION)
using string_rep = clang_string_rep;
#elif defined(__GNUC__) && defined(_GLIBCXX_USE_CXX11_ABI) \
    && _GLIBCXX_USE_CXX11_ABI
using string_rep = gcc_new_string_rep;
#elif defined(__GNUC__)
using string_rep = gcc_old_string_rep;
#elif defined(_MSC_VER)
using string_rep = msvc_string_rep;
#elif
#    error "Unknown compiler or std library -- don't know the string rep"
#endif


inline const string_rep&
as_internal_string_rep(const std::string& str)
{
    return *reinterpret_cast<const string_rep*>(&str);
        // reinterpret_cast<const char*>(&str));
}

};  // namespace pvt
OIIO_NAMESPACE_END
