#include "vector.h"
#include "point.h"
#include "vector.h"
#include "assert.h"
#include "rtfloat4.h"
#include "scalar.h"
namespace rt
{
    Vector::Vector(const Float4& f4)
    {
        Assert(f4[3] != 0) << " devision by zero Point\n";
        x = f4[0] / f4[3];
        y = f4[1] / f4[3];
        z = f4[2] / f4[3];
    }
    Vector Vector::operator+(const Vector& b) const
    {
        return Vector(x+b.x, y+b.y, z+b.z);;
    }

    Vector Vector::operator-(const Vector& b) const
    {
        return Vector(x-b.x, y-b.y, z-b.z);
    }

    Vector Vector::normalize() const
    {
        
        return Vector(x/length(), y/length(), z/length());
    }

    Vector Vector::operator-() const
    {
        return Vector(-x, -y, -z);
    }

    float Vector::lensqr() const
    {
        return (x*x + y*y + z*z);
    }

    float Vector::length() const
    {
        return sqrt(x*x + y*y + z*z);
    }

    bool Vector::operator==(const Vector& b) const
    {
        return ((x - b.x) <= EPSILON && (y - b.y) <= EPSILON && (z - b.z) <= EPSILON);
    }

    bool Vector::operator!=(const Vector& b) const
    {
        return ((x - b.x) > EPSILON || (y - b.y) > EPSILON || (z - b.z) > EPSILON);
    }

    float Vector::operator[](int index) const
    {
        // TODO: test using the fast that x, y, z are next to each
        // other in memory and use pointer arithmatic instead of
        // branching
        if(index == 0)
        {
            return x;
        }
        else if(index == 1)
        {
            return y;
        }
        else
        {
            return z;
        }
    }
    
    Vector operator*(float scalar, const Vector& b)
    {
        return Vector(b.x*scalar, b.y*scalar, b.z*scalar);
    }

    Vector operator*(const Vector& a, float scalar)
    {
        return Vector(a.x*scalar, a.y*scalar, a.z*scalar);
    }

    Vector operator/(const Vector& a, float scalar)
    {
        return Vector(a.x/scalar, a.y/scalar, a.z/scalar);
    }

    Vector cross(const Vector& a, const Vector& b)
    {
        Vector c;
        c.x = a.y*b.z - a.z*b.y;
        c.y = a.z*b.x - a.x*b.z;
        c.z = a.x*b.y - a.y*b.x;
        return c;
    }

    float dot(const Vector& a, const Vector& b)
    {
        return (a.x*b.x + a.y*b.y + a.z*b.z);
    }

    Vector min(const Vector& a, const Vector& b)
    {
        return Vector(a.x <= b.x? a.x : b.x,
                      a.y <= b.y? a.y : b.y,
                      a.z <= b.z? a.z : b.z);
    }

    Vector max(const Vector& a, const Vector& b)
    {
        return Vector(a.x >= b.x? a.x : b.x,
                      a.y >= b.y? a.y : b.y,
                      a.z >= b.z? a.z : b.z);
    }

    Point operator+(const Point& a, const Vector& b)
    {
        return Point(a.x+b.x, a.y+b.y, a.z+b.z);
    }

    Point operator+(const Vector& a, const Point& b)
    {
        return Point(a.x+b.x, a.y+b.y, a.z+b.z);
    }

    Point operator-(const Point& a, const Vector& b)
    {
        return Point(a.x-b.x, a.y-b.y, a.z-b.z);
    }

    Point operator*(const Float4& scale, const Point& p)
    {
        Point sc(scale);
        return  Point(sc.x * p.x,sc.y * p.y,sc.z * p.z);
    }
}
