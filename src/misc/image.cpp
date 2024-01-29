
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <cctype>

#include <png.h>
#include <jpeglib.h>    
#include <jerror.h>
//#include <GL/gl.h>

#include <swegl/misc/image.hpp>
#include <swegl/render/colors.hpp>

namespace swegl
{

bool ends_with(const std::string & a, const std::string & b)
{
	if (b.size() > a.size())
		return false;
	return a.substr(a.size()-b.size()) == b;
}

texture_t read_image_file(const std::string & filename, int offset)
{
	std::string filename_lower = filename;
	std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), [](unsigned char c){ return std::tolower(c); });
	     if (ends_with(filename_lower, "png" )) return read_png_file (filename, offset);
	else if (ends_with(filename_lower, "jpg" )) return read_jpeg_file(filename, offset);
	else if (ends_with(filename_lower, "jpeg")) return read_jpeg_file(filename, offset);
	else if (ends_with(filename_lower, "bmp" )) return read_bmp_file (filename, offset);
	else                                        return texture_t(nullptr, 0, 0);
}

texture_t read_bmp_file(FILE *fp)
{
	if (fp == NULL)
		return texture_t(nullptr, 0, 0);

	unsigned int file_size;
	unsigned int data_offset;
	unsigned int data_size;
	unsigned short bits_per_pixel;
	int width;
	int height;
	int dummy;
	int texel_count;
	unsigned char r,g,b;
	unsigned int *texture_data;

	dummy = fread(&dummy, 1, 2, fp); // magic number
	dummy = fread(&file_size, 1, 4, fp);
	dummy = fread(&dummy, 1, 2, fp); // RFU
	dummy = fread(&dummy, 1, 2, fp); // RFU
	dummy = fread(&data_offset, 1, 4, fp); 
	dummy = fread(&dummy, 1, 4, fp); // header size
	dummy = fread(&width, 1, 4, fp);
	dummy = fread(&height, 1, 4, fp);
	dummy = fread(&dummy, 1, 2, fp); // color planes
	dummy = fread(&bits_per_pixel, 1, 2, fp);
	dummy = fread(&dummy, 1, 4, fp); // compression mode
	dummy = fread(&data_size, 1, 4, fp);
	dummy = fread(&dummy, 1, 4, fp); // horizontal resolution
	dummy = fread(&dummy, 1, 4, fp); // vertical resolution
	dummy = fread(&dummy, 1, 4, fp); // palette size
	dummy = fread(&dummy, 1, 4, fp); // number of important colors

	texel_count = width*height;
	texture_data = new unsigned int[texel_count];
	if (texture_data == nullptr)
		return texture_t(nullptr, 0, 0);

	int lineoffset = texel_count;
	while (lineoffset > 0)
	{
		lineoffset -= width;
		for (int i=0 ; i<width ; i++)
		{
			dummy = fread(&b, 1, 1, fp);
			dummy = fread(&g, 1, 1, fp);
			dummy = fread(&r, 1, 1, fp);
			texture_data[lineoffset+i] = (r<<16)|(g<<8)|b;
		}
		dummy = fread(&dummy, 1, (width*3)%4, fp); // skipping padding bytes aligning lines on 32bits
	}

	return texture_t(texture_data, width, height);
}

texture_t read_bmp_file(const std::string & filename, int offset)
{
	FILE *fp = fopen(filename.c_str(), "rb");
	fseek(fp, offset, SEEK_SET);
	return read_bmp_file(fp);
}


texture_t read_png_file(FILE *fp)
{
	if ( ! fp)
		return texture_t(nullptr, 0, 0);

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

texture_t read_png_file(const std::string & filename, int offset)
{
	FILE *fp = fopen(filename.c_str(), "rb");
	fseek(fp, offset, SEEK_SET);
	return read_png_file(fp);
}


// stolen from https://stackoverflow.com/a/22463461
texture_t read_jpeg_file(FILE * fp)
{
	if ( ! fp)
		return texture_t(nullptr, 0, 0);

	unsigned long x, y;
	//unsigned int texture_id;
	unsigned long data_size;     // length of the file
	//int channels;               //  3 =>RGB   4 =>RGBA 
	//unsigned int type;  
	unsigned char * rowptr[1];    // pointer to an array
	unsigned char * jdata;        // data for the image
	struct jpeg_decompress_struct info; //for our jpeg info
	struct jpeg_error_mgr err;          //the error handler

	info.err = jpeg_std_error(& err);     
	jpeg_create_decompress(& info);   //fills info structure
	jpeg_stdio_src(&info, fp);    
	jpeg_read_header(&info, TRUE);   // read jpeg file header
	jpeg_start_decompress(&info);    // decompress the file

	//set width and height
	x = info.output_width;
	y = info.output_height;
	//channels = info.num_components;
	//type = GL_RGB;
	//if(channels == 4) type = GL_RGBA;

	data_size = x * y * 3;

	//--------------------------------------------
	// read scanlines one at a time & put bytes 
	//    in jdata[] array. Assumes an RGB image
	//--------------------------------------------
	jdata = (unsigned char *)malloc(data_size);
	while (info.output_scanline < info.output_height) // loop
	{
		// Enable jpeg_read_scanlines() to fill our jdata array
		rowptr[0] = (unsigned char *)jdata // secret to method
		          + 3 * info.output_width * info.output_scanline; 

		jpeg_read_scanlines(&info, rowptr, 1);
	}
	//---------------------------------------------------

	jpeg_finish_decompress(&info);   //finish decompressing

	//----- create OpenGL tex map (omit if not needed) --------
	//glGenTextures(1,&texture_id);
	//glBindTexture(GL_TEXTURE_2D, texture_id);
	//gluBuild2DMipmaps(GL_TEXTURE_2D,3,x,y,GL_RGB,GL_UNSIGNED_BYTE,jdata);

	jpeg_destroy_decompress(&info);
	fclose(fp);                    //close the file

	unsigned int * texture_data = new unsigned int[x*y];
	// adapt rgb order for our purposes
	for(unsigned int j = 0; j < y; j++)
	{
		for(unsigned int i = 0; i < x; i++)
		{
			texture_data[j*info.output_width + i] = pixel_colors(jdata[j* 3 * info.output_width + i * 3 +2]
			                                                    ,jdata[j* 3 * info.output_width + i * 3 +1]
			                                                    ,jdata[j* 3 * info.output_width + i * 3   ]
			                                                    ,255
			                                                    ).to_int();
			//pixel_colors * colors = (pixel_colors*) &texture_data[(int) ( y*width + x)];
			//std::swap(colors->o.r, colors->o.b);
		}
	}

	free(jdata);

	return texture_t(texture_data, x, y);
}

texture_t read_jpeg_file(const std::string & filename, int offset)
{
	FILE *fp = fopen(filename.c_str(), "rb");
	fseek(fp, offset, SEEK_SET);
	return read_jpeg_file(fp);
}


} // namespace
