#ifndef CG1RAYTRACER_POINT_HEADER
#define CG1RAYTRACER_POINT_HEADER

#include "macros.h"
#include <iostream>

namespace rt {

class Vector;
class Float4;

class ALIGN(16) Point {
public:
    float x, y, z;

    Point() :x(0),y(0),z(0){}
    Point(float x, float y, float z) : x(x), y(y), z(z) {}
    Point(const Float4& f4);

    static Point rep(float v) { return Point(v,v,v); }

    Vector operator-(const Point& b) const;
    Point operator+(const Point& b) const;
    Point operator/(float w) const;
    Point operator/=(float w);
    
    bool operator==(const Point& b) const;
    bool operator!=(const Point& b) const;
    float operator[](int index) const;
    
    friend std::ostream& operator<<(std::ostream& out, const Point& p)
    {
        out << "Point(" << p.x << ", " << p.y << ", " << p.z << ")";
        return out;
    }

};

Point operator*(float scalar, const Point& b);
Point operator*(const Point& a, float scalar);

Point operator-(float scalar, const Point& b);
Point operator-(const Point& a, float scalar);

Point operator+(float scalar, const Point& b);
Point operator+(const Point& a, float scalar);

Point min(const Point& a, const Point& b);
Point max(const Point& a, const Point& b);

}

#endif
