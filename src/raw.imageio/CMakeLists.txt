# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

if (LIBRAW_FOUND)
    add_oiio_plugin (rawinput.cpp
                     INCLUDE_DIRS ${LibRaw_INCLUDE_DIR}
                     LINK_LIBRARIES ${LibRaw_r_LIBRARIES}
                     COMPILE_OPTIONS ${LibRaw_r_DEFINITIONS})
else ()
    message (WARNING "Raw plugin will not be built")
endif ()
