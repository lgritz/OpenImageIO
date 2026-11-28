# glad OpenGL loader (generated, vendored)

Generator: https://github.com/Dav1dde/glad, glad2 version 2.0.8
License: the generated code is (WTFPL OR CC0-1.0) AND Apache-2.0 (Khronos
specifications), see the header of glad/gl.h.

glad/gl.h is generated, not hand written. To regenerate:

    pip install glad2
    python -m glad --reproducible --api gl:compatibility=2.1 \
        --extensions GL_ARB_half_float_pixel,GL_NV_half_float,GL_ARB_texture_float,GL_EXT_texture_sRGB,GL_ARB_texture_rg,GL_ARB_framebuffer_object \
        --out-path out c --header-only

and copy out/include/glad/gl.h here. OpenGL 2.1 compatibility profile is
what iv's rendering code needs; the extensions are the ones iv tests for or takes constants from.
