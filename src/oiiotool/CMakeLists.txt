# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO


# Add individual dependency-related definitions just where they're needed. By
# limiting it to just the files where they are necessary, it increases the
# chance of ccache reusing previous results from other runs and
# configurations.
set_property (SOURCE oiiotool.cpp
              APPEND PROPERTY COMPILE_DEFINITIONS
                 $<$<BOOL:USE_OPENCV>:USE_OPENCV=1>
             )


fancy_add_executable (SYSTEM_INCLUDE_DIRS
                        ${ROBINMAP_INCLUDES}
                        ${OpenCV_INCLUDES}
                      LINK_LIBRARIES
                        OpenImageIO
                        $<TARGET_NAME_IF_EXISTS:OpenEXR::OpenEXR>
                        ${OpenCV_LIBRARIES}
                     )
