
#include <swegl/Projection/MatrixStack.h>

namespace swegl
{

	MatrixStack::MatrixStack()
	{
		m_nb = 0;
	}

	void MatrixStack::PushMatrix(const Matrix4x4 & m)
	{
		if (this->m_nb >= 10)
			return;

		if (this->m_nb == 0)
		{
			this->m_matrices[0] = m;
			this->m_nb++;
		}
		else
		{
			this->m_matrices[m_nb] = this->m_matrices[m_nb-1] * m;
			this->m_nb++;
		}
	}

	void MatrixStack::PopMatrix()
	{
		if (m_nb > 0)
			m_nb--;
	}

	const Matrix4x4 MatrixStack::GetTopMatrix()
	{
		if (m_nb == 0)
		{
			Matrix4x4 result;
			result.SetIdentity();
			return result;
		}
		else
			return m_matrices[m_nb-1];
	}

}
