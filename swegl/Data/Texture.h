
#ifndef SWE_TEXTURE
#define SWE_TEXTURE

namespace swegl
{

	#define MIPMAPS_COUNT 5

	typedef struct
	{
		unsigned int *m_bitmap;
		unsigned int m_width;
		unsigned int m_height;
	
	}Mipmap;

	class Texture
	{	
	public:
		int m_mipmapsCount;
		Mipmap *m_mipmaps;

		Texture(const char *filename);
		Texture(unsigned int rgb);
		Texture(unsigned * data, int w, int h);
	};

}

#endif // SWE_TEXTURE
