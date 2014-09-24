#pragma once

#include "Math/Vec.h"

//------------------------------------------------------------------------------
// Mat3 (column-major OpenGL-style)
//------------------------------------------------------------------------------

struct Mat3 {
	// m[row][column]
	union {
		struct {
			float m11, m21, m31; // first column
			float m12, m22, m32; // second column
			float m13, m23, m33; // third column
		};
		float data[9];
	};

	Mat3() = default;

	Mat3(
		float a11, float a21, float a31,
		float a12, float a22, float a32,
		float a13, float a23, float a33
	):
		m11(a11), m21(a21), m31(a31),
		m12(a12), m22(a22), m32(a32),
		m13(a13), m23(a23), m33(a33)
	{}

	explicit Mat3(const float *n):
		m11(n[0]),  m21(n[1]),  m31(n[2]),
		m12(n[3]),  m22(n[4]),  m32(n[5]),
		m13(n[6]),  m23(n[7]),  m33(n[8])
	{}

	float &operator[](int i) { return data[i]; }
	float operator[](int i) const { return data[i]; }
};

//------------------------------------------------------------------------------
// Mat4 (column-major OpenGL-style)
//------------------------------------------------------------------------------

struct Mat4 {
	// m[row][column]
	// translation is stored at m14 m24 m34 or m[12] m[13] m[14]

	union {
		struct {
			float m11, m21, m31, m41; // first column
			float m12, m22, m32, m42; // second column
			float m13, m23, m33, m43; // third column
			float m14, m24, m34, m44; // fourth column
		};
		float data[16];
	};

	Mat4() = default;

	Mat4(
		float a11, float a21, float a31, float a41,
		float a12, float a22, float a32, float a42,
		float a13, float a23, float a33, float a43,
		float a14, float a24, float a34, float a44
	):
		m11(a11), m21(a21), m31(a31), m41(a41),
		m12(a12), m22(a22), m32(a32), m42(a42),
		m13(a13), m23(a23), m33(a33), m43(a43),
		m14(a14), m24(a24), m34(a34), m44(a44)
	{}

	explicit Mat4(const float *n):
		m11(n[0]),  m21(n[1]),  m31(n[2]),  m41(n[3]),
		m12(n[4]),  m22(n[5]),  m32(n[6]),  m42(n[7]),
		m13(n[8]),  m23(n[9]),  m33(n[10]), m43(n[11]),
		m14(n[12]), m24(n[13]), m34(n[14]), m44(n[15])
	{}

	float &operator[](int i) { return data[i]; }
	float operator[](int i) const { return data[i]; }

	void dump() const;
};

static inline Mat4 Mat4_Identity()
{
	return {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
}

static inline Mat4 Mat4_Zero()
{
	return {
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
}

static inline Mat4 Mat4_YZSwap()
{
	return {
		1, 0, 0,  0,
		0, 0, -1, 0,
		0, 1, 0,  0,
		0, 0, 0,  1
	};
}

bool operator==(const Mat4 &lhs, const Mat4 &rhs);
bool operator!=(const Mat4 &lhs, const Mat4 &rhs);
Mat4 operator*(const Mat4 &lhs, const Mat4 &rhs);
Mat4 operator+(const Mat4 &lhs, const Mat4 &rhs);
Vec3 operator*(const Mat4 &lhs, const Vec3 &rhs);
Vec3 operator*(const Vec3 &lhs, const Mat4 &rhs);
Vec4 operator*(const Mat4 &lhs, const Vec4 &rhs);
Vec4 operator*(const Vec4 &lhs, const Mat4 &rhs);

Mat4 Mat4_Rotate(const Vec3 &axis, float angle);
Mat4 Mat4_RotateX(float angle);
Mat4 Mat4_RotateY(float angle);
Mat4 Mat4_RotateZ(float angle);
Mat4 Mat4_Scale(const Vec3 &v);
Mat4 Mat4_Scale(float f);
Mat4 Mat4_Translate(const Vec3 &v);
Mat4 Mat4_Perspective(float fov, float aspect, float znear, float zfar);
Mat4 Mat4_Ortho(float left, float right, float bottom, float top,
	float znear = -1.0f, float zfar = 1.0f);
Mat4 Mat4_LookAt(const Vec3 &eye, const Vec3 &center, const Vec3 &up);

Mat4 transpose(const Mat4 &m);
float determinant(const Mat4 &m);
Mat4 inverse(const Mat4 &m, bool *inversed = nullptr);

// MiniOrtho2D contains scale components at xy and translation at zw, to
// transform the vertex do this: v * m.xy + m.zw.
Vec4 Vec4_MiniOrtho2D(float left, float right, float bottom, float top,
	const Vec2 &offset = Vec2(0));
Vec4 mini_ortho_translate(const Vec4 &miniortho, const Vec2 &offset);

// MiniPerspective3D contains scale in xyz and z-offset in w, to transform the
// vertex do this: Vec4(v.xy * m.xy, v.z * m.z + m.w, -v.z)
Vec4 Vec4_MiniPerspective3D(float vfov, float aspect, float znear, float zfar);

Mat4 to_mat4(const Mat3 &m);
Mat3 to_mat3(const Mat4 &m);
