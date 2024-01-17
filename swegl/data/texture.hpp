
#pragma once

#include <memory>

namespace swegl
{

	#define MIPMAPS_COUNT 5

	struct mipmap_t
	{
		unsigned int *m_bitmap = nullptr;
		unsigned int m_width;
		unsigned int m_height;
		~mipmap_t()
		{
			if (m_bitmap)
				delete[] m_bitmap;
		}
	};

	class texture_t
	{
	public:
		int m_mipmapsCount;
		std::unique_ptr<mipmap_t[]> m_mipmaps;

		texture_t(const char *filename);
		texture_t(unsigned int rgb);
		texture_t(unsigned * data, int w, int h);
	};

}
