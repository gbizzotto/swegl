
#include <thread>
#include <vector>
#include <cmath>
#include <swegl/Render/F001CustomNoArtefact.h>
#include <swegl/Render/ZInterpolator.h>
#include <swegl/Projection/Vec2f.h>
#include <mmintrin.h>
//#include <smmintrin.h>
#include <xmmintrin.h>

namespace swegl
{

	struct Scanline
	{
		int y;
		int x1, x2;
		QInterpolator qpixel;

		Scanline(int y, int x1, int x2, QInterpolator qp)
			:y(y), x1(x1), x2(x2), qpixel(qp)
		{}
	};

	void F001CustomNoArtefact::FillPoly(const Vec3f & _v0, const Vec3f & _v1, const Vec3f & _v2,
									    const Vec2f & _t0, const Vec2f & _t1, const Vec2f & _t2,
									    const Texture *t, ViewPort * vp, unsigned char shade, float * zbuffer)
	{
		Vec3f v0, v1, v2;
		Vec2f t0, t1, t2;

		if (_v0.y <= _v1.y && _v1.y <= _v2.y) {
			v0 = _v0;
			v1 = _v1;
			v2 = _v2;
			t0 = _t0;
			t1 = _t1;
			t2 = _t2;
		} else if (_v0.y <= _v2.y && _v2.y <= _v1.y) {
			v0 = _v0;
			v1 = _v2;
			v2 = _v1;
			t0 = _t0;
			t1 = _t2;
			t2 = _t1;
		} else if (_v1.y <= _v0.y && _v0.y <= _v2.y) {
			v0 = _v1;
			v1 = _v0;
			v2 = _v2;
			t0 = _t1;
			t1 = _t0;
			t2 = _t2;
		} else if (_v1.y <= _v2.y && _v2.y <= _v0.y) {
			v0 = _v1;
			v1 = _v2;
			v2 = _v0;
			t0 = _t1;
			t1 = _t2;
			t2 = _t0;
		} else if (_v2.y <= _v0.y && _v0.y <= _v1.y) {
			v0 = _v2;
			v1 = _v0;
			v2 = _v1;
			t0 = _t2;
			t1 = _t0;
			t2 = _t1;
		} else if (_v2.y <= _v1.y && _v1.y <= _v0.y) {
			v0 = _v2;
			v1 = _v1;
			v2 = _v0;
			t0 = _t2;
			t1 = _t1;
			t2 = _t0;
		}

		int y0 = (int) floor(v0.y);
		int y1 = (int) floor(v1.y);
		int y2 = (int) floor(v2.y);
		int xa = (int) floor(v0.x);
		int xb = (int) floor(v1.x);
		int xc = (int) floor(v2.x);
		bool line2_on_right; // true if line2 is on the right and we must choose its highest x value

		if (y0==y2) return; // All on 1 scanline, not worth drawing

		int y, ymax;
		float x1f, x2f; // reference points for the (u,v)s
		float ratio1, ratio2;
		ZInterpolator zi1, zi2;
		QInterpolator qpixel;
		float displacement;

		unsigned int *tbitmap;
		unsigned int twidth;
		unsigned int theight;

		// Init lines
		displacement = y0+0.5f - v0.y;
		ratio2 = (v2.x-v0.x) / (v2.y-v0.y); // never div#0 because y0!=y2
		zi2.Init(ZInterpolator::VERTICAL, v0, v2, t0, t2);
		zi2.DisplaceStartingPoint(displacement);
		x2f = v0.x + ratio2*displacement;

		if (y1 <= vp->m_y || y0 == y1) {
			// Start drawing at y1
			if (y1 <= vp->m_y) {
				// Skip first half of triangle
				zi2.DisplaceStartingPoint((float)(y1-y0));
				x2f = v0.x + ratio2*(y1-y0);
			}
			displacement = y1+0.5f - v1.y;
			ratio1 = (v2.x-v1.x) / (v2.y-v1.y); // never div#0 because y1!=y2
			zi1.Init(ZInterpolator::VERTICAL, v1, v2, t1, t2);
			zi1.DisplaceStartingPoint(displacement);
			x1f = v1.x + ratio1*displacement;
			line2_on_right = ratio2 < ratio1;
			y = y1;
			ymax = y2;
		} else {
			// Normal case (y0 != y1)
			ratio1 = (v1.x-v0.x) / (v1.y-v0.y); // never div#0 because y1!=y0
			zi1.Init(ZInterpolator::VERTICAL, v0, v1, t0, t1);
			zi1.DisplaceStartingPoint(displacement);
			x1f = v0.x + ratio1*displacement;
			line2_on_right = ratio2 > ratio1;
			y = y0;
			ymax = y1;
		}

		//if (ratio1-ratio2 < 0.01 && ratio1-ratio2 > -0.01) {
		//	// Triangle too small, not worth drawing;
		//	return;
		//}

		if (y < vp->m_y) {
			zi1.DisplaceStartingPoint((float)(vp->m_y-y));
			zi2.DisplaceStartingPoint((float)(vp->m_y-y));
			x1f += ratio1 * (vp->m_y-y);
			x2f += ratio2 * (vp->m_y-y);
			y = vp->m_y;
		}

		tbitmap = t->m_mipmaps[0].m_bitmap;
		twidth  = t->m_mipmaps[0].m_width;
		theight = t->m_mipmaps[0].m_height;

		std::vector<Scanline> scanlines;
		scanlines.reserve(ymax - y + 1);

		while (y <= ymax  &&  y < vp->m_y + vp->m_h)
		{
			int x1, x2;

			//int ualphastep_x;

			if (line2_on_right) {
				if (ratio2>0) {
					if (y == y2) {
						x2 = xc;
					} else {
						x2 = (int) (x2f + ratio2*0.5f);
					}
				} else {
					if (y == y0) {
						x2 = xa;
					} else {
						x2 = (int) (x2f - ratio2*0.5f);
					}
				}
				if (ratio1<0) {
					if (y == y1) {
						x1 = xb;
					} else if (y == y2) {
						x1 = xc;
					} else {
						x1 = (int) (x1f + ratio1*0.5f);
					}
				} else {
					if (y == y0) {
						x1 = xa;
					} else if (y == y1) {
						x1 = xb;
					} else {
						x1 = (int) (x1f - ratio1*0.5f);
					}
				}
			} else {
				if (ratio2<0) {
					if (y == y2) {
						x1 = xc;
					} else {
						x1 = (int) (x2f + ratio2*0.5f);
					}
				} else {
					if (y == y0) {
						x1 = xa;
					} else {
						x1 = (int) (x2f - ratio2*0.5f);
					}
				}
				if (ratio1>0) {
					if (y == y1) {
						x2 = xb;
					} else if (y == y2) {
						x2 = xc;
					} else {
						x2 = (int) (x1f + ratio1*0.5f);
					}
				} else {
					if (y == y0) {
						x2 = xa;
					} else if (y == y1) {
						x2 = xb;
					} else {
						x2 = (int) (x1f - ratio1*0.5f);
					}
				}
			}

			Vec3f linev0(x1f, 0, zi1.ualpha.z);
			Vec3f linev1(x2f, 0, zi2.ualpha.z);
			if (x1f <= x2f) {
				qpixel.Init(ZInterpolator::HORIZONTAL, linev0, linev1, zi1.ualpha, zi2.ualpha);
				displacement = x1+0.5f - linev0.x;
			} else {
				qpixel.Init(ZInterpolator::HORIZONTAL, linev1, linev0, zi2.ualpha, zi1.ualpha);
				displacement = x1+0.5f - linev1.x;
			}
			if (x1 < vp->m_x) {
				displacement += (float)(vp->m_x - x1);
				x1 = vp->m_x;
			}
			qpixel.DisplaceStartingPoint(displacement);

			if (x2 >= vp->m_x + vp->m_w) {
				x2 = vp->m_x + vp->m_w - 1;
			}

			//ASSERT(x1 <= x2 || x1 >= vp->m_x + vp->m_w);
			//ASSERT(x1 >= vp->m_x);

			scanlines.emplace_back(y, x1, x2, qpixel);
	
			x1f += ratio1;
			x2f += ratio2;
			zi1.Step();
			zi2.Step();
			y++;
			if (y == y1 && y1 != y2) {
				displacement = y1+0.5f - v1.y;
				ratio1 = (v2.x-v1.x) / (v2.y-v1.y); // never div#0 because y1!=y2
				zi1.Init(ZInterpolator::VERTICAL, v1, v2, t1, t2);
				zi1.DisplaceStartingPoint(displacement);
				x1f = v1.x + ratio1*displacement;
				ymax = y2;
			}
		}

		// Process scanlines in a parallel fashion
		auto scanline_filler = [vp,zbuffer,twidth,theight,tbitmap](std::vector<Scanline>::iterator it, std::vector<Scanline>::iterator end)
			{
				for ( ; it!=end ; ++it)
				{
					int y = it->y;
					int x1 = it->x1;
					int x2 = it->x2;
					unsigned int *video = &((unsigned int*)vp->m_screen->pixels)[(int) ( y*vp->m_screen->pitch/4 + x1)];
					float * zb = &zbuffer[(int) ( y*vp->m_w + x1)];
	
					for ( ; x1 <= x2 ; x1++ )
					{
						int u, v;

						if (it->qpixel.ualpha.z < *zb && it->qpixel.ualpha.z > 0.001) // Ugly z-near culling
						{
							u = ((int)it->qpixel.ualpha.x) % twidth;
							v = ((int)it->qpixel.ualpha.y) % theight;
							*video = tbitmap[v*twidth + u];
							*zb    = it->qpixel.ualpha.z;
						}
						video++;
						zb++;
		
						it->qpixel.Step();
					}
				}
			};

		auto concurrency = std::thread::hardware_concurrency();
		std::vector<std::thread> aux_threads;
		aux_threads.reserve(concurrency - 1);
		/*for (int i=0 ; i<concurrency-1 ; ++i)
			aux_threads.emplace_back([i, concurrency, &scanlines, &scanline_filler]()
				{
					scanline_filler(scanlines.begin() + scanlines.size() * i     / concurrency,
					                scanlines.begin() + scanlines.size() * (i+1) / concurrency);
				});
		scanline_filler(scanlines.begin() + scanlines.size() * (concurrency-1) / concurrency,
		                scanlines.end());*/
		scanline_filler(scanlines.begin(),
		                scanlines.end());
		for (auto it=aux_threads.begin(),end=aux_threads.end() ; it!=end ; ++it)
			it->join();
	}

}
