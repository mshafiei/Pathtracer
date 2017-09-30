#include "rtfloat4.h"
#include "assert.h"
#include "point.h"
#include "vector.h"
#include "windowsdefines.h"
namespace rt
{
    Float4::Float4(float x, float y, float z, float w)
    {
        vec[0] = x;
        vec[1] = y;
        vec[2] = z;
        vec[3] = w;
    }
    Float4::Float4(const Point& p)
    {
        vec[0] = p.x;
        vec[1] = p.y;
        vec[2] = p.z;
        vec[3] = 1;
    }

    Float4::Float4(const Vector& v)
    {
        vec[0] = v.x;
        vec[1] = v.y;
        vec[2] = v.z;
        vec[3] = 0;
    }

    static Float4 rep(float v) { return Float4(v, v, v, v); }

    float& Float4::operator[](int idx)
    {
        return vec[idx];
    }
    float Float4::operator[](int idx) const
    {
        return vec[idx];
    }

    Float4 Float4::operator+(const Float4& b) const
    {
        return Float4(vec[0] + b[0], vec[1] + b[1], vec[2] + b[2], vec[3] + b[3]);
    }
    Float4 Float4::operator-(const Float4& b) const
    {
        return Float4(vec[0] - b[0], vec[1] - b[1], vec[2] - b[2], vec[3] - b[3]);
    }

    Float4 Float4::operator*(const Float4& b) const
    {
        return Float4(vec[0] * b[0], vec[1] * b[1], vec[2] * b[2], vec[3] * b[3]);
    }
    
    Float4 Float4::operator/(const Float4& b) const
    {
        Assert(b[0] != 0 && b[1] != 0 && b[2] != 0 && b[3] != 0) << " devision by zero in Float4\n";
        
        return Float4(vec[0] / b[0], vec[1] / b[1], vec[2] / b[2], vec[3] / b[3]);
    }

    Float4 Float4::operator-() const
    {
        return Float4(-vec[0], -vec[1], -vec[2], -vec[3]);
    }

    bool Float4::operator==(const Float4& b) const
    {
        return vec[0] == b[0] && vec[1] == b[1] && vec[2] == b[2] && vec[3] == b[3];
    }

    bool Float4::operator != (const Float4& b) const
    {
        return vec[0] != b[0] && vec[1] != b[1] && vec[2] != b[2] && vec[3] != b[3];
    }

    Float4 operator*(float scalar, const Float4& b)
    {
        return Float4(b[0] * scalar, b[1] * scalar, b[2] * scalar, b[3] * scalar);
    }
    Float4 operator*(const Float4& a, float scalar)
    {
        return Float4(a[0] * scalar, a[1] * scalar, a[2] * scalar, a[3] * scalar);
    }

    Float4 operator/(const Float4& a, float scalar)
    {
        Assert(scalar != 0) << " devision by zero Float4";
        return Float4(a[0] / scalar, a[1] / scalar, a[2] / scalar, a[3] / scalar);
    }
    
    float dot(const Float4& a, const Float4& b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    }

    Float4 min(const Float4& a, const Float4& b)
    {
        return Float4(std::min(a[0], b[0]), std::min(a[1], b[1]), std::min(a[2], b[2]), std::min(a[3], b[3]));
    }

    Float4 max(const Float4& a, const Float4& b)
    {
        return Float4(std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2]), std::max(a[3], b[3]));
    }

}