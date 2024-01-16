
#include <memory.h>
#include <math.h>
#include <swegl/projection/matrix44.hpp>

namespace swegl
{

	const freon::Matrix<float,4,4> & matrix44_t::Identity = freon::MatrixIdentity<float>::_4;

	void matrix44_t::RotateX(float a)
	{
		// fast version : 16 mul instead of 64, and no object copy
		float cosa = (float)cos(a);
		float sina = (float)sin(a);
		float d4 = (*this)[1][0];
		float d5 = (*this)[1][1];
		float d6 = (*this)[1][2];
		float d7 = (*this)[1][3];
		(*this)[1][0] =  cosa*d4 + sina*(*this)[2][0];
		(*this)[1][1] =  cosa*d5 + sina*(*this)[2][1];
		(*this)[1][2] =  cosa*d6 + sina*(*this)[2][2];
		(*this)[1][3] =  cosa*d7 + sina*(*this)[2][3];
		(*this)[2][0] = -sina*d4 + cosa*(*this)[2][0];
		(*this)[2][1] = -sina*d5 + cosa*(*this)[2][1];
		(*this)[2][2] = -sina*d6 + cosa*(*this)[2][2];
		(*this)[2][3] = -sina*d7 + cosa*(*this)[2][3];
	}

	void matrix44_t::RotateY(float a)
	{
		// fast version : 16 mul instead of 64, and no object copy
		float cosa = (float)cos(a);
		float sina = (float)sin(a);
		float d0 = (*this)[0][0];
		float d1 = (*this)[0][1];
		float d2 = (*this)[0][2];
		float d3 = (*this)[0][3];
		(*this)[0][0] =  cosa*d0 + sina*(*this)[2][0];
		(*this)[0][1] =  cosa*d1 + sina*(*this)[2][1];
		(*this)[0][2] =  cosa*d2 + sina*(*this)[2][2];
		(*this)[0][3] =  cosa*d3 + sina*(*this)[2][3];
		(*this)[2][0] = -sina*d0 + cosa*(*this)[2][0];
		(*this)[2][1] = -sina*d1 + cosa*(*this)[2][1];
		(*this)[2][2] = -sina*d2 + cosa*(*this)[2][2];
		(*this)[2][3] = -sina*d3 + cosa*(*this)[2][3];
	}

	void matrix44_t::RotateZ(float a)
	{
		// fast version : 16 mul instead of 64, and no object copy
		float cosa = (float)cos(a);
		float sina = (float)sin(a);
		float d0 = (*this)[0][0];
		float d1 = (*this)[0][1];
		float d2 = (*this)[0][2];
		float d3 = (*this)[0][3];
		(*this)[0][0] =  cosa*d0 + sina*(*this)[1][0];
		(*this)[0][1] =  cosa*d1 + sina*(*this)[1][1];
		(*this)[0][2] =  cosa*d2 + sina*(*this)[1][2];
		(*this)[0][3] =  cosa*d3 + sina*(*this)[1][3];
		(*this)[1][0] = -sina*d0 + cosa*(*this)[1][0];
		(*this)[1][1] = -sina*d1 + cosa*(*this)[1][1];
		(*this)[1][2] = -sina*d2 + cosa*(*this)[1][2];
		(*this)[1][3] = -sina*d3 + cosa*(*this)[1][3];
	}

	void matrix44_t::SetRotateXY(float x, float y)
	{
		float cosx = (float)cos(x);
		float sinx = (float)sin(x);
		float cosy = (float)cos(y);
		float siny = (float)sin(y);
		(*this)[3][3] = 1;
		(*this)[0][3] = (*this)[1][0] = (*this)[1][3] = (*this)[2][3] = (*this)[3][0] = (*this)[3][1] = (*this)[3][2] = 0;
		(*this)[0][0] =  cosy;
		(*this)[0][1] = -sinx*siny;
		(*this)[0][2] =  siny*cosx;
		(*this)[1][1] =  cosx;
		(*this)[1][2] =  sinx;
		(*this)[2][0] = -siny;
		(*this)[2][1] = -sinx*cosy;
		(*this)[2][2] =  cosx*cosy;
	}

	void matrix44_t::Translate(float x, float y, float z)
	{
		(*this)[0][3] += x;
		(*this)[1][3] += y;
		(*this)[2][3] += z;
	}

	/*matrix44_t operator*(const matrix44_t & left, const matrix44_t & up)
	{
		matrix44_t result;
		__m128 u0 = _mm_load_ps(&up[0][0]);
		__m128 u1 = _mm_load_ps(&up[1][0]);
		__m128 u2 = _mm_load_ps(&up[2][0]);
		__m128 u3 = _mm_load_ps(&up[3][0]);

		__m128 _f = _mm_load1_ps(&left[0][0]);
		__m128 r = _mm_mul_ps(_f, u0);
		_f = _mm_load1_ps(&left[0][1]);
		__m128 t = _mm_mul_ps(_f, u1);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[0][2]);
		t = _mm_mul_ps(_f, u2);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[0][3]);
		t = _mm_mul_ps(_f, u3);
		r = _mm_add_ps(r, t);
		_mm_store_ps(result[0], r);

		_f = _mm_load1_ps(&left[1][0]);
		r = _mm_mul_ps(_f, u0);
		_f = _mm_load1_ps(&left[1][1]);
		t = _mm_mul_ps(_f, u1);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[1][2]);
		t = _mm_mul_ps(_f, u2);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[1][3]);
		t = _mm_mul_ps(_f, u3);
		r = _mm_add_ps(r, t);
		_mm_store_ps(result[1], r);

		_f = _mm_load1_ps(&left[2][0]);
		r = _mm_mul_ps(_f, u0);
		_f = _mm_load1_ps(&left[2][1]);
		t = _mm_mul_ps(_f, u1);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[2][2]);
		t = _mm_mul_ps(_f, u2);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[2][3]);
		t = _mm_mul_ps(_f, u3);
		r = _mm_add_ps(r, t);
		_mm_store_ps(result[2], r);

		_f = _mm_load1_ps(&left[3][0]);
		r = _mm_mul_ps(_f, u0);
		_f = _mm_load1_ps(&left[3][1]);
		t = _mm_mul_ps(_f, u1);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[3][2]);
		t = _mm_mul_ps(_f, u2);
		r = _mm_add_ps(r, t);
		_f = _mm_load1_ps(&left[3][3]);
		t = _mm_mul_ps(_f, u3);
		r = _mm_add_ps(r, t);
		_mm_store_ps(result[3], r);

		return result;
	}*/

}
