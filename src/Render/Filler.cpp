
#include <cmath>
#include <swegl/Render/Filler.h>
#include <swegl/Render/ZInterpolator.h>
#include <swegl/Projection/Vec2f.h>

namespace swegl
{

	void Filler::FillPoly(const Vec3f & v0, const Vec3f & v1, const Vec3f & v2,
						  const Vec2f & t0, const Vec2f & t1, const Vec2f & t2,
						  bool neighb0visible, bool neighb1visible, bool neighb2visible, 
						  Texture *t, Texture *tb, ViewPort * vp,
						  const Vec3f & facenormal, float *zbuffer)
	{
		Vec3f l_v0, l_v1, l_v2;
		Vec2f l_t0, l_t1, l_t2;
		bool nb0visible, nb1visible, nb2visible;

		if (v0.y <= v1.y){
			if (v1.y <= v2.y) {
				l_v0 = v0;
				l_v1 = v1;
				l_v2 = v2;
				l_t0 = t0;
				l_t1 = t1;
				l_t2 = t2;
				nb0visible = neighb0visible;
				nb1visible = neighb1visible;
				nb2visible = neighb2visible;
			} else if (v0.y <= v2.y) {
				l_v0 = v0;
				l_v1 = v2;
				l_v2 = v1;
				l_t0 = t0;
				l_t1 = t2;
				l_t2 = t1;
				nb0visible = neighb2visible;
				nb1visible = neighb1visible;
				nb2visible = neighb0visible;
			} else {
				l_v0 = v2;
				l_v1 = v0;
				l_v2 = v1;
				l_t0 = t2;
				l_t1 = t0;
				l_t2 = t1;
				nb0visible = neighb2visible;
				nb1visible = neighb0visible;
				nb2visible = neighb1visible;
			}
		} else {
			if (v2.y <= v1.y) {
				l_v0 = v2;
				l_v1 = v1;
				l_v2 = v0;
				l_t0 = t2;
				l_t1 = t1;
				l_t2 = t0;
				nb0visible = neighb1visible;
				nb1visible = neighb0visible;
				nb2visible = neighb2visible;
			} else if (v2.y <= v0.y) {
				l_v0 = v1;
				l_v1 = v2;
				l_v2 = v0;
				l_t0 = t1;
				l_t1 = t2;
				l_t2 = t0;
				nb0visible = neighb1visible;
				nb1visible = neighb2visible;
				nb2visible = neighb0visible;
			} else {
				l_v0 = v1;
				l_v1 = v0;
				l_v2 = v2;
				l_t0 = t1;
				l_t1 = t0;
				l_t2 = t2;
				nb0visible = neighb0visible;
				nb1visible = neighb2visible;
				nb2visible = neighb1visible;
			}
		}

		int y0 = (int) floor(l_v0.y);
		int y1 = (int) floor(l_v1.y);
		int y2 = (int) floor(l_v2.y);
		int xa = (int) floor(l_v0.x);
		int xb = (int) floor(l_v1.x);
		int xc = (int) floor(l_v2.x);
		bool line2_on_right; // true if line2 is on the right and we must choose its highest x value

		if (y0==y2) return; // All on 1 scanline, not worth drawing

		int y, ymax;
		float x1f, x2f; // reference points for the (u,v)s
		float x1fnexthalf, x1fprevhalf, x2fnexthalf, x2fprevhalf; // Value of x1f and x2f between scanlines
		bool n1visible; // for antialiasing
		float ratio1, ratio2;
		ZInterpolator zi1, zi2;
		QInterpolator qpixel;
		float displacement;

		// Bump mapping variables
		Vec3f frontvec;// = mesh->m_vertexbuffer[strip->m_indexbuffer[i-2]].Mul3x3(mstacknormals.GetTopMatrix());
		frontvec.z = 1;
		Matrix4x4 light;
	
		if (tb == NULL)
			light.SetIdentity();

		// Init lines
		displacement = y0+0.5f - l_v0.y;
		ratio2 = (l_v2.x-l_v0.x) / (l_v2.y-l_v0.y); // never div#0 because y0!=y2
		zi2.Init(ZInterpolator::VERTICAL, l_v0, l_v2, l_t0, l_t2);
		zi2.DisplaceStartingPoint(displacement);
		x2f = l_v0.x + ratio2*displacement;

		if (y1 <= vp->m_y || y0 == y1) {
			// Start drawing at y1
			if (y1 <= vp->m_y) {
				// Skip first half of triangle
				zi2.DisplaceStartingPoint((float)(y1-y0));
				x2f += ratio2*(y1-y0);
			}
			displacement = y1+0.5f - l_v1.y;
			ratio1 = (l_v2.x-l_v1.x) / (l_v2.y-l_v1.y); // never div#0 because y1!=y2
			zi1.Init(ZInterpolator::VERTICAL, l_v1, l_v2, l_t1, l_t2);
			zi1.DisplaceStartingPoint(displacement);
			x1f = l_v1.x + ratio1*displacement;
			n1visible = nb1visible;
			line2_on_right = ratio2 < ratio1;
			y = y1;
			ymax = y2;
		} else {
			// Normal case (y0 != y1)
			ratio1 = (l_v1.x-l_v0.x) / (l_v1.y-l_v0.y); // never div#0 because y1!=y0
			zi1.Init(ZInterpolator::VERTICAL, l_v0, l_v1, l_t0, l_t1);
			zi1.DisplaceStartingPoint(displacement);
			x1f = l_v0.x + ratio1*displacement;
			n1visible = nb0visible;
			line2_on_right = ratio2 > ratio1;
			y = y0;
			ymax = y1;
		}

		if (y < vp->m_y) {
			zi1.DisplaceStartingPoint((float)(vp->m_y-y));
			zi2.DisplaceStartingPoint((float)(vp->m_y-y));
			x1f += ratio1 * (vp->m_y-y);
			x2f += ratio2 * (vp->m_y-y);
			y = vp->m_y;
		}
	
		x2fprevhalf = x2f - 0.5f*ratio2;
		x2fnexthalf = x2fprevhalf + ratio2;
		x1fprevhalf = x1f - 0.5f*ratio1;
		x1fnexthalf = x1fprevhalf + ratio1;

		unsigned int *scanline = &((unsigned int*)vp->m_screen->pixels)[(int) ( (y*vp->m_screen->pitch)>>2 )];
		unsigned int lineslength = vp->m_screen->pitch>>2;
		while (y <= ymax  &&  y < vp->m_y + vp->m_h)
		{
			int x1, x2;
			int alpha1=256, alpha2=256;
			int alpha1diff=0, alpha2diff=0;
		
			// Set of variables used for mipmapping
			float ualphastep_x;
			int mmp; // divider, to get texture coordinates right in the filler loop
			int mmpe; // 1<<mmp
			unsigned int *tbitmap;
			unsigned int *tbumpmap = NULL;
			unsigned int twidth;
			unsigned int theight;
			unsigned int *video;
			// End if variables used for mipmapping
			float floornh, floorph;
			if (line2_on_right) {
				if (ratio1<0) {
					if (n1visible == false) {
						if (y == y2) {
							x1 = xc;
						} else {
							x1 = (int) x1fnexthalf;
						}
						floornh = floor(x1fnexthalf);
						floorph = floor(x1fprevhalf);
						if (floornh == floorph) {
							alpha1 = (int) (256 * (1.0f-(x1fnexthalf-floornh+x1fprevhalf-floorph)/2.0f));
							alpha1diff = 256;
						} else {
							alpha1 = (int) (256 * (floornh+1-x1fnexthalf) * (1.0f-(floornh+1-x1fprevhalf)/ratio1) /2.0f);
							alpha1diff = (int) (-256 / ratio1);
							if (alpha1diff == 0) alpha1diff = 1;
						}
					} else {
						if (y == y1) {
							x1 = xb;
						} else if (y == y2) {
							x1 = xc;
						} else {
							x1 = (int) (x1f + 0.5f);
						}
					}
				} else {
					if (n1visible == false) {
						if (y == y1) {
							x1 = xb;
						} else if (y == y0) {
							x1 = xa;
						} else {
							x1 = (int) x1fprevhalf;
						}
						floornh = floor(x1fnexthalf);
						floorph = floor(x1fprevhalf);
						if (floornh == floorph) {
							alpha1 = (int) (256 * (1.0f-(x1fnexthalf-floornh+x1fprevhalf-floorph)/2.0f));
							alpha1diff = 256;
						} else {
							alpha1 = (int) (256 * (floorph+1-x1fprevhalf) * ((floorph+1-x1fprevhalf)/ratio1) /2.0f);
							alpha1diff = (int) (256 / ratio1);
							if (alpha1diff == 0) alpha1diff = 1;
						}
					} else {
						if (y == y1) {
							x1 = xb;
						} else if (y == y0) {
							x1 = xa;
						} else {
							x1 = (int) (x1f + 0.5f);
						}
					}
				}
				if (ratio2>0) {
					if (nb2visible == false) {
						// Do antialiasing
						if (y == y2) {
							x2 = xc;
						} else {
							x2 = (int) x2fnexthalf;
						}
						floornh = floor(x2fnexthalf);
						floorph = floor(x2fprevhalf);
						if (floornh == floorph) {
							alpha2 = (int) (256 * ((x2fnexthalf-floornh+x2fprevhalf-floorph)/2.0f));
							alpha2diff = -256;
							alpha2 += -(x2-x1) * alpha2diff;
						} else {
							alpha2 = (int) (256 * (x2fnexthalf-floornh) * (1.0f-(floornh-x2fprevhalf)/ratio2) /2.0f);
							alpha2diff = (int) (-256 / ratio2);
							if (alpha2diff == 0) alpha2diff = -1;
							alpha2 += -(x2-x1) * alpha2diff;
						}
					} else {
						if (y == y2) {
							x2 = xc;
						} else {
							// No antialiasing, skip lots of pixels
							x2 = (int) (x2f - 0.5f);
						}
					}
				} else {
					if (nb2visible == false) {
						if (y == y0) {
							x2 = xa;
						} else {
							x2 = (int) x2fprevhalf;
						}
						floornh = floor(x2fnexthalf);
						floorph = floor(x2fprevhalf);
						if (floornh == floorph) {
							alpha2 = (int) (256 * ((x2fnexthalf-floornh+x2fprevhalf-floorph)/2.0f));
							alpha2diff = -256;
							alpha2 += -(x2-x1) * alpha2diff;
						} else {
							alpha2 = (int) (256 * (x2fprevhalf-floorph) * ((floorph-x2fprevhalf)/ratio2) /2.0f);
							alpha2diff = (int) (256 / ratio2);
							if (alpha2diff == 0) alpha2diff = -1;
							alpha2 += -(x2-x1) * alpha2diff;
						}
					} else {
						if (y == y0) {
							x2 = xa;
						} else {
							x2 = (int) (x2f - 0.5f);
						}
					}
				}
			} else {
				if (ratio2<0) {
					if (y == y2) {
						x1 = xc;
					} else if (nb2visible == false) {
						x1 = (int) x2fnexthalf;
						floornh = floor(x2fnexthalf);
						floorph = floor(x2fprevhalf);
						if (floornh == floorph) {
							alpha1 = (int) (256 * (1.0f-(x2fnexthalf-floornh+x2fprevhalf-floorph)/2.0f));
							alpha1diff = 256;
						} else {
							alpha1 = (int) (256 * (floornh+1-x2fnexthalf) * (1.0f-(floornh+1-x2fprevhalf)/ratio2) /2.0f);
							alpha1diff = (int) (-256 / ratio2);
							if (alpha1diff == 0) alpha1diff = 1;
						}
					} else {
						x1 = (int) (x2f + 0.5f);
					}
				} else {
					if (y == y0) {
						x1 = xa;
					} else if (nb2visible == false) {
						x1 = (int) x2fprevhalf;
						floornh = floor(x2fnexthalf);
						floorph = floor(x2fprevhalf);
						if (floornh == floorph) {
							alpha1 = (int) (256 * (1.0f-(x2fnexthalf-floornh+x2fprevhalf-floorph)/2.0f));
							alpha1diff = 256;
						} else {
							alpha1 = (int) (256 * (floorph+1-x2fprevhalf) * ((floorph+1-x2fprevhalf)/ratio2) /2.0f);
							alpha1diff = (int) (256 / ratio2);
							if (alpha1diff == 0) alpha1diff = 1;
						}
					} else {
						x1 = (int) (x2f + 0.5f);
					}
				}
				if (ratio1>0) {
					if (n1visible == false) {
						// Do antialiasing
						if (y == y1) {
							x2 = xb;
						} else if (y == y2) {
							x2 = xc;
						} else {
							x2 = (int) x1fnexthalf;
						}				
						floornh = floor(x1fnexthalf);
						floorph = floor(x1fprevhalf);
						if (floornh == floorph) {
							alpha2 = (int) (256 * ((x1fnexthalf-floornh+x1fprevhalf-floorph)/2.0f));
							alpha2diff = -256;
							alpha2 += -(x2-x1) * alpha2diff;
						} else {
							alpha2 = (int) (256 * (x1fnexthalf-floornh) * (1.0f-(floornh-x1fprevhalf)/ratio1) /2.0f);
							alpha2diff = (int) (-256 / ratio1);
							if (alpha2diff == 0) alpha2diff = -1;
							alpha2 += -(x2-x1) * alpha2diff;
						}
					} else {
						if (y == y1) {
							x2 = xb;
						} else if (y == y2) {
							x2 = xc;
						} else {
							// No antialiasing, skip lots of pixels
							x2 = (int) (x1f - 0.5f);
						}
					}
				} else {
					if (n1visible == false) {
						if (y == y1) {
							x2 = xb;
						} else if (y == y0) {
							x2 = xa;
						} else {
							x2 = (int) x1fprevhalf;
						}
						floornh = floor(x1fnexthalf);
						floorph = floor(x1fprevhalf);
						if (floornh == floorph) {
							alpha2 = (int) (256 * ((x1fnexthalf-floornh+x1fprevhalf-floorph)/2.0f));
							alpha2diff = -256;
							alpha2 += -(x2-x1) * alpha2diff;
						} else {
							alpha2 = (int) (256 * (x1fprevhalf-floorph) * ((floorph-x1fprevhalf)/ratio1) /2.0f);
							alpha2diff = (int) (256 / ratio1);
							if (alpha2diff == 0) alpha2diff = -1;
							alpha2 += -(x2-x1) * alpha2diff;
						}
					} else {
						if (y == y1) {
							x2 = xb;
						} else if (y == y0) {
							x2 = xa;
						} else {
							x2 = (int) (x1f - 0.5f);
						}
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
				alpha1 += alpha1diff * (vp->m_x - x1);
				alpha2 += alpha2diff * (vp->m_x - x1);
				x1 = vp->m_x;
			}
			qpixel.DisplaceStartingPoint(displacement/*, x2-x1*/);

			if (x2 >= vp->m_x + vp->m_w) {
				x2 = vp->m_x + vp->m_w - 1;
			}

			// Init mipmapping
			//ualphastep_x = (int)sqrt( ((qpixel.ualphastep.x*qpixel.ualphastep.x)/8) + ((qpixel.ualphastep.y*qpixel.ualphastep.y)/8) );
			ualphastep_x = abs(qpixel.ualphastep.x);
			//ualphastep_x = abs((int)( (((qpixel.topalpha+qpixel.topstep)/(qpixel.bottomalpha+qpixel.bottomstep))-qpixel.ualpha).x ));
			mmp = 0;
			mmpe = 1;
			while (ualphastep_x > 1.5f && mmp+1 < t->m_mipmapsCount) {
				if (t->m_mipmaps[mmp].m_bitmap == NULL)
					break;
				mmp++;
				mmpe <<= 1;
				ualphastep_x /= 2.0f;
			}
			tbitmap = t->m_mipmaps[mmp].m_bitmap;
			if (tb != 0)
				tbumpmap = tb->m_mipmaps[mmp].m_bitmap;
			twidth = t->m_mipmaps[mmp].m_width;
			theight = t->m_mipmaps[mmp].m_height;
			// End of mipmapping init

			video = scanline + x1;

			for (int line_offset=y*vp->m_w+x1 ; x1<=x2 ; x1++ )
			{
				int u, v;
				int r, g, b, alpha;
				float actualx, actualy;
				float nextx, nexty;
				float z = qpixel.ualpha.z;
				//float normaldotproduct;

				if (linev0.z < linev1.z) {
					if (z < linev0.z) {
						z = linev0.z;
					}
				} else {
					if (z < linev1.z) {
						z = linev1.z;
					}
				}
				if (z > 0.001 // Ugly z-near culling
					&& (z < zbuffer[line_offset] || ((*video)>>24) != 0) )
				{
					actualx = ((qpixel.ualpha.x) / mmpe) -0.5f;
					actualy = ((qpixel.ualpha.y) / mmpe) -0.5f;

					u = (int)(floor(actualx));
					v = (int)(floor(actualy));

					if (actualx<0) {
						u = 0;
						actualx = 1;
					} else if (actualx>(int)twidth-1) {
						u = (int) (twidth-1);
						actualx = 1;
					} else {
						actualx = u+1 - actualx;
					}
					nextx = 1 - actualx;
					if (actualy<0) {
						v = 0;
						actualy = 1;
					} else if (actualy>(int)theight-1) {
						v = (int) (theight-1);
						actualy = 1;
					} else {
						actualy = v+1 - actualy;
					}
					nexty = 1 - actualy;
				
					// antialiasing
					alpha = 0;
					if (alpha1 < 256 && alpha2 < 256) {
						alpha = 256-((alpha1*alpha2) >> 8);
					} else if (alpha1 < 256) {
						alpha = 256-alpha1;
					} else if (alpha2 < 256) {
						alpha = 256-alpha2;
					}

					v *= twidth;
					unsigned int *directbitmap = &tbitmap[u+v];
					r = (int)( (((*(directbitmap         ))>>16)&0xFF) * actualx*actualy
							  +(((*(directbitmap+twidth  ))>>16)&0xFF) * actualx*nexty
							  +(((*(directbitmap       +1))>>16)&0xFF) * nextx*actualy
							  +(((*(directbitmap+twidth+1))>>16)&0xFF) * nextx*nexty );
					g = (int)( (((*(directbitmap         ))>> 8)&0xFF) * actualx*actualy
							  +(((*(directbitmap+twidth  ))>> 8)&0xFF) * actualx*nexty
							  +(((*(directbitmap       +1))>> 8)&0xFF) * nextx*actualy
							  +(((*(directbitmap+twidth+1))>> 8)&0xFF) * nextx*nexty );
					b = (int)(  ((*(directbitmap         ))     &0xFF) * actualx*actualy
							  + ((*(directbitmap+twidth  ))     &0xFF) * actualx*nexty
							  + ((*(directbitmap       +1))     &0xFF) * nextx*actualy
							  + ((*(directbitmap+twidth+1))     &0xFF) * nextx*nexty );

					/*
					if (tbumpmap != 0)
					{
						light.SetRotateXY((((unsigned char)tbumpmap[u+1+v])-((unsigned char)tbumpmap[u+v])) * 0.0031415926f,
										  (((unsigned char)tbumpmap[u+v+twidth])-((unsigned char)tbumpmap[u+v])) * 0.0031415926f);
						normaldotproduct = frontvec.Dot(facenormal*light);
					}
					else
					{
						normaldotproduct = frontvec.Dot(facenormal);
					}
					if (normaldotproduct > 0) normaldotproduct = 0;
					unsigned char shade = (unsigned char) (normaldotproduct * -225)+30;
					*/
					unsigned char shade = 255;

					// Camera lighting
					r = ((r*shade) << 8) & 0xFF0000;
					g =  (g*shade)       & 0x00FF00;
					b = ((b*shade) >> 8) & 0x0000FF;
					// End of camera lighting

					// Antialiasing
					if (alpha == 0) {
						if (z < zbuffer[line_offset]) {
							*video = r|g|b;
						} else {
							alpha = 256 - ((*video)>>24);
							r = (( (((*video)>>16)&0xFF) * (alpha) ) >> 8) | (((256-alpha) * ((r>>16)&0xFF))>>8);
							g = (( (((*video)>> 8)&0xFF) * (alpha) ) >> 8) | (((256-alpha) * ((g>> 8)&0xFF))>>8);
							b = (( (((*video)    )&0xFF) * (alpha) ) >> 8) | (((256-alpha) * ((b    )&0xFF))>>8);
							alpha = ((alpha*(256-alpha)) << 16) & 0xFF000000;
							*video = alpha | (r<<16) | (g<<8) | b;
						}
					} else {
						if (z < zbuffer[line_offset]) {
							r = (( (((*video)>>16)&0xFF) * (alpha) ) >> 8) + (((256-alpha) * ((r>>16)&0xFF))>>8);
							g = (( (((*video)>> 8)&0xFF) * (alpha) ) >> 8) + (((256-alpha) * ((g>> 8)&0xFF))>>8);
							b = (( (((*video)    )&0xFF) * (alpha) ) >> 8) + (((256-alpha) * ((b    )&0xFF))>>8);
							*video = (alpha<<24) | (r<<16) | (g<<8) | b;
						} else {
							// TODO: Mix alphas
						}
					}
				
					zbuffer[line_offset] = z;
				}

				qpixel.Step();
				line_offset++;
				video++;
				alpha1 += alpha1diff;
				alpha2 += alpha2diff;

				if (qpixel.quake == 0)
				{
					// Time to recalculate mipmapping
					//ualphastep_x = (int)sqrt( ((qpixel.ualphastep.x*qpixel.ualphastep.x)/8) + ((qpixel.ualphastep.y*qpixel.ualphastep.y)/8) );
					ualphastep_x = abs(qpixel.ualphastep.x);
					//ualphastep_x = abs((int)( (((qpixel.topalpha+qpixel.topstep)/(qpixel.bottomalpha+qpixel.bottomstep))-qpixel.ualpha).x ));
					mmp = 0;
					mmpe = 1;
					while (ualphastep_x > 1.5f && mmp+1 < t->m_mipmapsCount) {
						if (t->m_mipmaps[mmp].m_bitmap == NULL)
							break;
						mmp++;
						mmpe <<= 1;
						ualphastep_x /= 2.0f;
					}
					tbitmap = t->m_mipmaps[mmp].m_bitmap;
					if (tb != 0)
						tbumpmap = tb->m_mipmaps[mmp].m_bitmap;
					twidth = t->m_mipmaps[mmp].m_width;
					theight = t->m_mipmaps[mmp].m_height;
				}
			}

			x1f += ratio1;
			x2f += ratio2;
			x1fprevhalf = x1fnexthalf;
			x1fnexthalf += ratio1;
			x2fprevhalf = x2fnexthalf;
			x2fnexthalf += ratio2;
			zi1.Step();
			zi2.Step();
			y++;
			scanline += lineslength;
			if (y == y1 && y1 != y2) {
				displacement = y1+0.5f - l_v1.y;
				ratio1 = (l_v2.x-l_v1.x) / (l_v2.y-l_v1.y); // never div#0 because y1!=y2
				zi1.Init(ZInterpolator::VERTICAL, l_v1, l_v2, l_t1, l_t2);
				zi1.DisplaceStartingPoint(displacement);
				n1visible = nb1visible;
				x1f = l_v1.x + ratio1*displacement;
				x1fnexthalf = x1f + 0.5f*ratio1;
				ymax = y2;
			}
		}
	}

}
