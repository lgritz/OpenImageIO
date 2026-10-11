// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// Generate five-components.j2c: a 16x8, 5-component, 8-bit HTJ2K codestream
// with the color transform on (so the reader takes the interleaved path).
// Build: c++ -std=c++17 make_five_components.cpp -lopenjph
#include <openjph/ojph_codestream.h>
#include <openjph/ojph_file.h>
#include <openjph/ojph_mem.h>
#include <openjph/ojph_params.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv)
{
    const int W = 16, H = 8, NC = 5;
    ojph::codestream cs;
    ojph::param_siz siz = cs.access_siz();
    siz.set_image_extent(ojph::point(W, H));
    siz.set_num_components(NC);
    for (int c = 0; c < NC; ++c)
        siz.set_component(c, ojph::point(1, 1), 8, false);
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(W, H));
    siz.set_tile_offset(ojph::point(0, 0));
    ojph::param_cod cod = cs.access_cod();
    cod.set_num_decomposition(0);
    cod.set_reversible(true);
    cod.set_color_transform(true);
    cs.set_planar(false);
    ojph::j2c_outfile out;
    out.open(argv[1]);
    cs.write_headers(&out);
    ojph::ui32 next = 0;
    ojph::line_buf* line = cs.exchange(NULL, next);
    for (int y = 0; y < H; ++y)
        for (int c = 0; c < NC; ++c) {
            for (int x = 0; x < W; ++x)
                line->i32[x] = 10 * (c + 1) + x;
            line = cs.exchange(line, next);
        }
    cs.flush();
    cs.close();
    out.close();
    return 0;
}
