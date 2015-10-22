
#pragma once

#include <swegl/Projection/Matrix4x4.h>

namespace swegl
{

	class MatrixStack
	{
	public:
		Matrix4x4 m_matrices[10];
		unsigned int m_nb;

		MatrixStack();
		void PushMatrix(const Matrix4x4 & m);
		void PopMatrix();
		const Matrix4x4 GetTopMatrix();
	};

}
