#ifndef CG1RAYTRACER_MATRIX_HEADER
#define CG1RAYTRACER_MATRIX_HEADER
#include <iostream>
#include "rtfloat4.h"

namespace rt {

class Matrix {
public:
    Matrix() {
        for (int i = 0; i < 4; i++)
            mat[i] = Float4(0, 0, 0, 0);
    }
    Matrix(const Float4& r1, const Float4& r2, const Float4& r3, const Float4& r4);

    Float4& operator[](int idx);
    Float4 operator[](int idx) const;

    Matrix operator+(const Matrix& b) const;
    Matrix operator-(const Matrix& b) const;
    
    Matrix transpose() const;
    Matrix invert() const;

    bool operator==(const Matrix& b) const;
    bool operator!=(const Matrix& b) const;


    Float4 operator*(const Float4& b) const;
    Vector operator*(const Vector& b) const;
    Point operator*(const Point& b) const;

    std::ostream& operator<< (std::ostream& out) const
    {
        for (int i = 0; i < 4; i++)
        {
            mat[i] << out;
            out << std::endl;
        }

        return out;
    }
    
    float det() const;

    static Matrix zero();
    static Matrix identity();

    static Matrix system(const Vector& e1, const Vector& e2, const Vector& e3);

    Float4 mat[4];
};

Matrix product(const Matrix& a, const Matrix& b);
Matrix operator*(const Matrix& a, float scalar);
Matrix operator*(float scalar, const Matrix& a);
    
}


#endif
