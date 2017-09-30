#include <iostream>
#include "matrix.h"
#include "assert.h"
#include "point.h"
#include "vector.h"

using namespace rt;
Matrix Matrix::invert() const {
        
    int indxc[4], indxr[4];
    int ipiv[4] = { 0, 0, 0, 0 };
    Matrix minv = *this;
    for (int i = 0; i < 4; i++) {
        int irow = -1, icol = -1;
        float big = 0.;
        // Choose pivot
        for (int j = 0; j < 4; j++) {
            if (ipiv[j] != 1) {
                for (int k = 0; k < 4; k++) {
                    if (ipiv[k] == 0) {
                        if (fabsf(minv[j][k]) >= big) {
                            big = float(fabsf(minv[j][k]));
                            irow = j;
                            icol = k;
                        }
                    }
                    else if (ipiv[k] > 1)
                    {
                        std::cerr << "Singular matrix in MatrixInvert" << std::endl;
                    }
                }
            }
        }
        ++ipiv[icol];
        // Swap rows _irow_ and _icol_ for pivot
        if (irow != icol) {
            for (int k = 0; k < 4; ++k)
                std::swap(minv[irow][k], minv[icol][k]);
        }
        indxr[i] = irow;
        indxc[i] = icol;
        if (minv[icol][icol] == 0.)
        {
            std::cerr << "Singular matrix in MatrixInvert" << std::endl;
        }
        // Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
        float pivinv = 1.f / minv[icol][icol];
        minv[icol][icol] = 1.f;
        for (int j = 0; j < 4; j++)
            minv[icol][j] *= pivinv;

        // Subtract this row from others to zero out their columns
        for (int j = 0; j < 4; j++) {
            if (j != icol) {
                float save = minv[j][icol];
                minv[j][icol] = 0;
                for (int k = 0; k < 4; k++)
                    minv[j][k] -= minv[icol][k] * save;
            }
        }
    }
    // Swap columns to reflect permutation
    for (int j = 3; j >= 0; j--) {
        if (indxr[j] != indxc[j]) {
            for (int k = 0; k < 4; k++)
                std::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
        }
    }
    return minv;
}


Matrix::Matrix(const rt::Float4& r1, const rt::Float4& r2, const rt::Float4& r3, const rt::Float4& r4)
{
    mat[0] = r1;
    mat[1] = r2;
    mat[2] = r3;
    mat[3] = r4;
}

rt::Float4& Matrix::operator[](int idx)
{
    Assert(idx < 4) << " invalid indexing operator[]";
    return mat[idx];
}

rt::Float4 Matrix::operator[](int idx) const
{
    Assert(idx < 4) << " invalid indexing operator[]";
;        return mat[idx];
}

Matrix Matrix::operator+(const Matrix& b) const
{
    Matrix res;
    for (int i = 0; i < 4;++i)
        res[i] = mat[i] + b[i];
    return res;
}
Matrix Matrix::operator-(const Matrix& b) const
{
    Matrix res;
    for (int i = 0; i < 4; ++i)
        res[i] = mat[i] - b[i];
    return res;
}

Matrix Matrix::transpose() const
{
    Matrix res;
    for (int i = 0; i < 4;++i)
        for (int j = 0; j < 4; ++j)
        {
            res[i][j] = mat[j][i];
        }
    return res;
}

bool Matrix::operator==(const Matrix& b) const
{
    bool equal = true;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (mat[i][j] != mat[i][j])
                equal = false;
    return equal;
}

bool Matrix::operator!=(const Matrix& b) const
{
    bool equal = true;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (mat[i][j] != mat[i][j])
                equal = false;
    return !equal;
}


rt::Float4 Matrix::operator*(const rt::Float4& b) const
{
    rt::Float4 res;
    res[0] = dot(mat[0], b);
    res[1] = dot(mat[1], b);
    res[2] = dot(mat[2], b);
    res[3] = dot(mat[3], b);
    return res;
}

Vector Matrix::operator*(const Vector& b) const
{
    Vector res;
    rt::Float4 fltB(b);
    //how should this function be implemented according to vector constructor?
    //float w = dot(mat[3], fltB);
    //Assert(w == 0) << " zero scale Matrix\n";
    res.x = dot(mat[0], fltB);// / w;
    res.y = dot(mat[1], fltB);// / w;
    res.z = dot(mat[2], fltB);// / w;
    return res;
}

Point Matrix::operator*(const Point& b) const
{
    Point res;
    rt::Float4 fltB(b);
    //float w = dot(mat[3], fltB);
    //Assert(w != 0) << " zero scale Matrix\n";
    res.x = dot(mat[0], fltB);// / w;
    res.y = dot(mat[1], fltB);// / w;
    res.z = dot(mat[2], fltB);// / w;
    return res;
}

float Matrix::det() const
{
    return (mat[0][0] * mat[1][1] * mat[2][2] * mat[3][3] +
        mat[0][1] * mat[1][2] * mat[2][3] + mat[3][0] +
        mat[0][2] * mat[1][3] * mat[2][0] * mat[3][1] +
        mat[0][3] * mat[1][0] * mat[2][1] * mat[3][2]) -
        (mat[0][3] * mat[1][2] * mat[2][1] * mat[3][0] +
        mat[0][0] * mat[1][3] * mat[2][2] * mat[3][1] +
        mat[0][1] * mat[1][0] * mat[2][3] * mat[3][2] +
        mat[0][2] * mat[1][1] * mat[2][0] * mat[3][3]);
}

Matrix Matrix::zero()
{
        return Matrix();
}

Matrix Matrix::identity()
{
    Matrix mat = zero();
    mat[0][0] = 1; mat[1][1] = 1; mat[2][2] = 1; mat[3][3] = 1;
    return mat;
}

Matrix Matrix::system(const Vector& e1, const Vector& e2, const Vector& e3)
{
    Matrix mat;
    mat[0][0] = e1.x; mat[0][1] = e2.x; mat[0][2] = e3.x; mat[0][2] = 0;
    mat[1][0] = e1.y; mat[1][1] = e2.y; mat[1][2] = e3.y; mat[1][2] = 0;
    mat[2][0] = e1.z; mat[2][1] = e2.z; mat[2][2] = e3.z; mat[2][2] = 0;
    mat[3][0] = 0;    mat[3][1] = 0;    mat[3][2] = 0;    mat[3][2] = 1;
    return mat;

}
Matrix product(const Matrix& a, const Matrix& b)
{
    /*
    std::cout << "Start product" << std::endl;
        
    a << std::cout;
    std::cout << std::endl;

    b << std::cout;
    std::cout << std::endl;
    */
    Matrix res;
    for (int c = 0; c < 4; ++c)
    {
        rt::Float4 col(b[0][c], b[1][c], b[2][c], b[3][c]);
        for (int r = 0; r < 4; ++r)
            res[r][c] = dot(a[r], col);
    }

    //res << std::cout;
    //std::cout << std::endl;
        
    return res;
}
Matrix operator*(const Matrix& a, float scalar)
{
    Matrix b;
    for (int i = 0; i < 4;++i)
        b[0] = a[0] * scalar;
    return b;
}
Matrix operator*(float scalar, const Matrix& a)
{
    Matrix b;
    for (int i = 0; i < 4; ++i)
        b[0] = a[0] * scalar;
    return b;
}
