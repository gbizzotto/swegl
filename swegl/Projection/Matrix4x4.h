
#ifndef SWE_MATRIX
#define SWE_MATRIX

namespace swegl
{

	class Vec3f;

	class Matrix4x4
	{
	public:
		float m_data[16];

		Matrix4x4();
		~Matrix4x4();

		void Set(float *data);
		const Matrix4x4 operator*(const Matrix4x4 & other) const;
		Vec3f operator*(const Vec3f & v) const;
		void Set(const Matrix4x4 & other);
		void RotateX(float a);
		void RotateY(float a);
		void RotateZ(float a);
		void SetRotateXY(float x, float y);
		void Translate(float x, float y, float z);
		void SetIdentity();
	};

}

#endif // SWE_MATRIX