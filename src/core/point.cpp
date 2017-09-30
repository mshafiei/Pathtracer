#include "point.h"
#include "vector.h"
#include "rtfloat4.h"
#include "assert.h"
namespace rt
{
    Point::Point(const Float4& f4)
    {
        Assert(f4[3] != 0) << " devision by zero Point\n";
        x = f4[0] / f4[3];
        y = f4[1] / f4[3];
        z = f4[2] / f4[3];
    }

    Vector Point::operator-(const Point& b) const
    {
        return Vector(x-b.x, y-b.y, z-b.z);
    }
    Point Point::operator+(const Point& b) const
    {
        return Point(x + b.x, y + b.y, z + b.z);
    }
    bool Point::operator==(const Point& b) const
    {
        return ((x == b.x) && (y == b.y) && (z == b.z));        
    }
    
    bool Point::operator!=(const Point& b) const
    {
        return ((x != b.x) || (y != b.y) || (z != b.z));
    }

    Point Point::operator/(float w) const
    {
        return Point(x/w, y/w, z/w);
    }

    Point Point::operator/=(float w)
    {
        x = x / w;
        y = y / w;
        z = z / w;
        return *this;
    }


    float Point::operator[](int index) const
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

    
    Point operator*(float scalar, const Point& b)
    {
        return Point(b.x*scalar, b.y*scalar, b.z*scalar);
    }
        
    Point operator*(const Point& a, float scalar)
    {
        return Point(a.x*scalar, a.y*scalar, a.z*scalar);
    }
    Point operator-(float scalar, const Point& b)
    {
        return Point(scalar - b.x, scalar - b.y, scalar - b.z);
    }
    Point operator-(const Point& a, float scalar)
    {
        return Point(a.x - scalar, a.y - scalar, a.z - scalar);
    }

    Point operator+(const Point& a, float scalar)
    {
        return Point(a.x + scalar, a.y + scalar, a.z + scalar);
    }

    Point operator+(float scalar, const Point& a)
    {
        return Point(a.x + scalar, a.y + scalar, a.z + scalar);
    }

    Point min(const Point& a, const Point& b)
    {
        return Point(a.x <= b.x? a.x : b.x,
                      a.y <= b.y? a.y : b.y,
                      a.z <= b.z? a.z : b.z);
    }
        
    Point max(const Point& a, const Point& b)
    {
        return Point(a.x >= b.x? a.x : b.x,
                     a.y >= b.y? a.y : b.y,
                     a.z >= b.z? a.z : b.z);
    }
}
