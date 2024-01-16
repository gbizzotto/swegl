
#pragma once

#include <iostream>
#include <memory.h>
#include <initializer_list>

#include <freon/Matrix.hpp>

namespace swegl
{

	class Matrix4x4 : public freon::Matrix<float,4,4>
	{
	public:
		Matrix4x4()
		{}
		Matrix4x4(const freon::Matrix<float,4,4> & other)
			:freon::Matrix<float,4,4>(other)
		{}
		void RotateX(float a);
		void RotateY(float a);
		void RotateZ(float a);
		void SetRotateXY(float x, float y);
		void Translate(float x, float y, float z);

		//operator freon::Matrix<float,4,4>() { return *static_cast<freon::Matrix<float,4,4>*>(this); }

		static const freon::Matrix<float,4,4> & Identity;
	};

	//Matrix4x4 operator*(const Matrix4x4 & left, const Matrix4x4 & up);

	template<typename O>
	O & operator<<(O & out, const Matrix4x4 & m)
	{
		out << m[0][0] << ',' << m[0][1] << ',' << m[0][2] << "\n"
			<< m[1][0] << ',' << m[1][1] << ',' << m[1][2] << "\n"
			<< m[2][0] << ',' << m[2][1] << ',' << m[2][2] << "\n"
			;
		return out;
	}

}
