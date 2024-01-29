
#pragma once

#include <swegl/data/texture.hpp>

namespace swegl
{

texture_t read_image_file(const std::string & filename, int offset=0);

texture_t read_bmp_file(const std::string & filename, int offset=0);
texture_t read_bmp_file(FILE *fp);

texture_t read_png_file(const std::string & filename, int offset=0);
texture_t read_png_file(FILE *fp);

texture_t read_jpeg_file(const std::string & filename, int offset=0);
texture_t read_jpeg_file(FILE * fp);

} // namespace
