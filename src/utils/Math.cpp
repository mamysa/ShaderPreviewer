#include "Math.h"
#include <math.h>

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


