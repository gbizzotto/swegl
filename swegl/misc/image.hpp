
#pragma once

#include <swegl/data/texture.hpp>

namespace swegl
{

texture_t read_png_file(char *filename, int offset=0);
texture_t read_png_file(FILE *fp);

} // namespace
