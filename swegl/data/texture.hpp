
#pragma once

#include <memory>
#include <vector>

namespace swegl
{

	#define MIPMAPS_COUNT 5

	struct mipmap_t
	{
		unsigned int *m_bitmap = nullptr;
		unsigned int m_width;
		unsigned int m_height;
		inline mipmap_t(unsigned int * b, unsigned int w, unsigned int h)
			: m_bitmap(b)
			, m_width(w)
			, m_height(h)
		{}
		~mipmap_t()
		{
			if (m_bitmap)
				delete[] m_bitmap;
		}
	};

	class texture_t
	{
	public:
		std::vector<std::shared_ptr<mipmap_t>> m_mipmaps;

		texture_t(unsigned int rgb);
		texture_t(unsigned * data, int w, int h);

		void produce_mipmaps();
	};

}
