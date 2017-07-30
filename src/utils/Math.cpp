#include "Math.h"
#include <math.h>

Vector3::Vector3(void): x(0.0), y(0.0), z(0.0) { } 
Vector3::Vector3(float _x, float _y, float _z): x(_x), y(_y), z(_z) { } 

// matrix funcs
float& Matrix3::operator()(int i, int j) {
	return buf[3*j+i];
}

const float& Matrix3::operator()(int i, int j) const {
	return buf[3*j+i];
}

Matrix3 operator*(const Matrix3& A, const Matrix3& B) {
	Matrix3 C; 
	for (int i = 0; i < 3; i++)
	for (int j = 0; j < 3; j++) {
		C(i,j) = 0.0f;
		for (int k = 0; k < 3; k++) {
			C(i,j) = C(i,j) + A(i,k) * B(k,j);
		}
	}

	return C;
}

Vector3 operator*(const Matrix3& A, const Vector3& b) {
	Vector3 c;
	c.x = A(0,0)*b.x + A(0,1)*b.y + A(0,2)*b.z;
	c.y = A(1,0)*b.x + A(1,1)*b.y + A(1,2)*b.z;
	c.z = A(2,0)*b.x + A(2,1)*b.y + A(2,2)*b.z;
	return c;
}


Vector3 operator*(const Vector3& a, float f) {
	Vector3 c; 
	c.x = a.x * f; c.y = a.y * f; c.z = a.z * f;
	return c;
}


Vector3 Vector3::normalize(void) const {
	float m = sqrt(x*x + y*y + z*z);
	return Vector3(x/m, y/m, z/m);
}

Matrix3 rotateX(float n) {
	float a = cos(n);
	float b = sin(n);

	Matrix3 A;
	A(0,0) = 1.0; A(0,1) = 0.0; A(0,2) = 0.0;
	A(1,0) = 0.0; A(1,1) = a;   A(1,2) = -b;
	A(2,0) = 0.0; A(2,1) = b;   A(2,2) = a;
	return A;
}

Matrix3 rotateY(float n) {
	float a = cos(n);
	float b = sin(n);

	Matrix3 A;
	A(0,0) = a;   A(0,1) = 0.0; A(0,2) = b;
	A(1,0) = 0.0; A(1,1) = 1.0; A(1,2) = 0.0;
	A(2,0) = -b;  A(2,1) = 0.0; A(2,2) = a;
	return A;
}


Vector3 operator+(const Vector3& a, const Vector3& b) {
	return Vector3(a.x+b.x, a.y+b.y, a.z+b.z);
}


