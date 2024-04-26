#pragma once

#include <vector>
#include <cmath>

template <class T> struct Vec2 {
    union {
        struct {T u, v;};
        struct {T x, y;};
        T raw[2];
    };
    Vec2() : u(0), v(0) {}
    Vec2(T _u, T _v) : u(_u),v(_v) {}
    inline Vec2<T> operator +(const Vec2<T> &V) const { return Vec2<T>(u+V.u, v+V.v); }
    inline Vec2<T> operator -(const Vec2<T> &V) const { return Vec2<T>(u-V.u, v-V.v); }
    inline Vec2<T> operator *(float f)          const { return Vec2<T>(u*f, v*f); }
    T& operator[](int index) { return raw[index]; }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec2<T>& v);
};

template <class T> struct Vec3 {
    union {
        struct {T x, y, z;};
        struct { T ivert, iuv, inorm; };
        T raw[3];
    };
    Vec3() : x(0), y(0), z(0) {}
    Vec3(T _x, T _y, T _z) : x(_x),y(_y),z(_z) {}
    inline Vec3<T> operator ^(const Vec3<T> &v) const { return Vec3<T>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
    inline Vec3<T> operator +(const Vec3<T> &v) const { return Vec3<T>(x+v.x, y+v.y, z+v.z); }
    inline Vec3<T> operator -(const Vec3<T> &v) const { return Vec3<T>(x-v.x, y-v.y, z-v.z); }
    inline Vec3<T> operator *(float f)          const { return Vec3<T>(x*f, y*f, z*f); }
    inline T       operator *(const Vec3<T> &v) const { return x*v.x + y*v.y + z*v.z; }
    float norm () const { return std::sqrt(x*x+y*y+z*z); }
    Vec3<T> & normalize(T l=1) { *this = (*this)*(l/norm()); return *this; }
    T& operator[](int index) { return raw[index]; }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec3<T>& v);

    Vec2<T> toVec2() {
        return Vec2<T> (x, y);
    };
};

typedef Vec2<float> Vec2f;
typedef Vec2<int>   Vec2i;
typedef Vec3<float> Vec3f;
typedef Vec3<int>   Vec3i;



template <class T> std::ostream& operator<<(std::ostream& s, Vec2<T>& v) {
    s << "(" << v.x << ", " << v.y << ")\n";
    return s;
}

template <class T> std::ostream& operator<<(std::ostream& s, Vec3<T>& v) {
    s << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
    return s;
}

template <class T>
class Polygon2D {
public:
    int n;
    std::vector<Vec2<T>> pt;
    Polygon2D(int _n, std::vector<Vec2<T>> _pt): n(_n), pt(_pt) {}
};

template <class T>
class Triangle2D: public Polygon2D<T> {
public:
    using Polygon2D<T>::pt;
    std::vector<T> depth;

    Triangle2D(std::vector<Vec2<T>> _pt, std::vector<Vec2<T>> _depth = {0, 0, 0}):
        Polygon2D<T>(3, _pt),
        depth(_depth) {}

    Triangle2D(std::vector<Vec3<float>> _pt):
        Polygon2D<T>(3, {_pt[0].toVec2(), _pt[1].toVec2(), _pt[2].toVec2()}),
        depth({_pt[0].z, _pt[1].z, _pt[2].z}) {}

    Vec3f baryCentric(Vec2f P) {
        Vec3f u = Vec3f(pt[2][0]-pt[0][0], pt[1][0]-pt[0][0], pt[0][0]-P[0])^Vec3f(pt[2][1]-pt[0][1], pt[1][1]-pt[0][1], pt[0][1]-P[1]);
        /* `pts` and `P` has integer value as coordinates
           so `abs(u[2])` < 1 means `u[2]` is 0, that means
           triangle is degenerate, in this case return something with negative coordinates */
        if (std::abs(u.z)<1) return Vec3f(-1,1,1);
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    }

    bool inInside(Vec2i P) {
        auto bc = baryCentric(P);
        if (bc.x<0 || bc.y<0 || bc.z<0) return false;
        return true;
    }
};








