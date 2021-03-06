#pragma once

struct Matrix3;
struct Vector3;

struct Vector3 {
	Vector3(void);
	Vector3(float, float, float);
	Vector3 normalize(void) const; 
	float x;
	float y;
	float z;
};

// dummy matrix3 class.
struct Matrix3 {
private:
	float buf[9];
public:
	float& operator()(int,int);
	const float& operator()(int,int) const;
	const float * getBuffer(void) const { return buf; }
};

Matrix3 operator*(const Matrix3&, const Matrix3&); 
Vector3 operator*(const Matrix3&, const Vector3&); 
Vector3 operator+(const Vector3&, const Vector3&); 
Vector3 operator*(const Vector3&, float); 
Matrix3 rotateX(float);
Matrix3 rotateY(float);