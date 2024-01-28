
#pragma once

#include <iostream>
#include <memory.h>
#include <initializer_list>

#include <freon/Matrix.hpp>

namespace swegl
{

	class matrix44_t : public freon::Matrix<float,4,4>
	{
	public:
		matrix44_t()
		{}
		matrix44_t(const freon::Matrix<float,4,4> & other)
			:freon::Matrix<float,4,4>(other)
		{}
		template <typename... P>
		matrix44_t(P... ts)
			:freon::Matrix<float,4,4>{ ts... }
		{}
		void rotate_x(float a);
		void rotate_y(float a);
		void rotate_z(float a);
		void set_rotate_xy(float x, float y);
		void translate(float x, float y, float z);

		//operator freon::Matrix<float,4,4>() { return *static_cast<freon::Matrix<float,4,4>*>(this); }

		static const freon::Matrix<float,4,4> & Identity;
		static matrix44_t from_quaternion(float q0, float q1, float q2, float q3)
		{
			return matrix44_t
					{
						2*(q0*q0+q1*q1)-1,2*(q1*q2-q0*q3)  ,2*(q1*q3+q0*q2)  , 0.0f,
						2*(q1*q2+q0*q3)  ,2*(q0*q0+q2*q2)-1,2*(q2*q3-q0*q1)  , 0.0f,
						2*(q1*q3-q0*q2)  ,2*(q2*q3+q0*q1)  ,2*(q0*q0+q3*q3)-1, 0.0f,
						             0.0f,             0.0f,             0.0f, 1.0f,
					};
		}
	};

	//matrix44_t operator*(const matrix44_t & left, const matrix44_t & up);

	template<typename O>
	O & operator<<(O & out, const matrix44_t & m)
	{
		out << m[0][0] << ',' << m[0][1] << ',' << m[0][2] << "\n"
			<< m[1][0] << ',' << m[1][1] << ',' << m[1][2] << "\n"
			<< m[2][0] << ',' << m[2][1] << ',' << m[2][2] << "\n"
			;
		return out;
	}

}
