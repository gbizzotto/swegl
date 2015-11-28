
#pragma once

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

		operator freon::Matrix<float,4,4>() { return *static_cast<freon::Matrix<float,4,4>*>(this); }

		static const freon::Matrix<float,4,4> & Identity;
	};

	//Matrix4x4 operator*(const Matrix4x4 & left, const Matrix4x4 & up);

}
