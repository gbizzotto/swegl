
#include <stdlib.h>
#include <stdio.h>
#include <png.h>

#include <swegl/misc/image.hpp>
#include <swegl/render/colors.hpp>

namespace swegl
{

texture_t read_png_file(FILE *fp)
{
	int width, height;
	png_byte color_type;
	png_byte bit_depth;

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png)
		return texture_t(nullptr, 0, 0);

	png_infop info = png_create_info_struct(png);
	if(!info)
		return texture_t(nullptr, 0, 0);
	if(setjmp(png_jmpbuf(png)))
		return texture_t(nullptr, 0, 0);

	png_init_io(png, fp);

	png_read_info(png, info);

	width      = png_get_image_width(png, info);
	height     = png_get_image_height(png, info);
	color_type = png_get_color_type(png, info);
	bit_depth  = png_get_bit_depth(png, info);

	// Read any color_type into 8bit depth, RGBA format.
	// See http://www.libpng.org/pub/png/libpng-manual.txt

	if(bit_depth == 16)
		png_set_strip_16(png);

	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

		// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	auto row_pointers = std::make_unique<png_bytep[]>(sizeof(int) * height);
	if ( ! row_pointers)
		return texture_t(nullptr, 0, 0);

	unsigned int * texture_data = new unsigned int[width*height];
	if ( ! texture_data)
		return texture_t(nullptr, 0, 0);

	for(int y = 0; y < height; y++)
		row_pointers[y] = (unsigned char*)&texture_data[y*width];

	png_read_image(png, (unsigned char **)row_pointers.get());

	fclose(fp);

	png_destroy_read_struct(&png, &info, NULL);

	// adapt rgb order for our purposes
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			pixel_colors * colors = (pixel_colors*) &texture_data[(int) ( y*width + x)];
			std::swap(colors->o.r, colors->o.b);
		}
	}
	return texture_t(texture_data, width, height);
}

texture_t read_png_file(const char *filename, int offset)
{
	FILE *fp = fopen(filename, "rb");
	fseek(fp, offset, SEEK_SET);
	return read_png_file(fp);
}

} // namespace
