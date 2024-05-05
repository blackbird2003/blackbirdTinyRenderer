#pragma once

#include <vector>
#include <cmath>
#include <cassert>

template <class T> struct Vec2 {

    T x, y;
    Vec2<T>() : x(T()), y(T()) {}
    Vec2<T>(T _x, T _y) : x(_x), y(_y) {}
    Vec2<T>(const Vec2<T> &v) : x(T()), y(T()) { *this = v; }
    Vec2<T> & operator =(const Vec2<T> &v) {
        if (this != &v) {
            x = v.x;
            y = v.y;
        }
        return *this;
    }

    inline Vec2<T> operator +(const Vec2<T> &V) const { return Vec2<T>(x+V.x, y+V.y); }
    inline Vec2<T> operator -(const Vec2<T> &V) const { return Vec2<T>(x-V.x, y-V.y); }
    inline Vec2<T> operator *(float f)          const { return Vec2<T>(x*f, y*f); }
    T& operator[](int index) { return index == 0 ? x : y; }

    template <class > friend std::ostream& operator<<(std::ostream& s, Vec2<T>& v);
};

template <class T> struct Vec3 {
    T x, y, z;

    Vec3<T>() : x(T()), y(T()), z(T()) { }
    Vec3<T>(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

    template <class U> explicit Vec3<T>(const Vec3<U> &v);

    Vec3<T>(const Vec3<T> &v) : x(T()), y(T()), z(T()) { *this = v; }
    Vec3<T> & operator =(const Vec3<T> &v) {
        if (this != &v) {
            x = v.x;
            y = v.y;
            z = v.z;
        }
        return *this;
    }

    inline Vec3<T> operator ^(const Vec3<T> &v) const { return Vec3<T>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
    inline Vec3<T> operator +(const Vec3<T> &v) const { return Vec3<T>(x+v.x, y+v.y, z+v.z); }
    inline Vec3<T> operator -(const Vec3<T> &v) const { return Vec3<T>(x-v.x, y-v.y, z-v.z); }
    inline Vec3<T> operator *(float f)          const { return Vec3<T>(x*f, y*f, z*f); }
    inline T operator *(const Vec3<T> &v) const { return x*v.x + y*v.y + z*v.z; }
    float norm () const { return std::sqrt(x*x+y*y+z*z); }
    Vec3<T> & normalize(T l=1) { *this = (*this)*(l/norm()); return *this; }
    T& operator[](int index) { if (index == 0) return x; if (index == 1) return y; return z; }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec3<T>& v);

    Vec2<T> toVec2() {
        return Vec2<T> (x, y);
    };
};

typedef Vec2<float> Vec2f;
typedef Vec2<int>   Vec2i;
typedef Vec3<float> Vec3f;
typedef Vec3<int>   Vec3i;

template <> template <>
inline Vec3<int>::Vec3(const Vec3<float>& v) : x(int(v.x + .5)), y(int(v.y + .5)), z(int(v.z + .5)) {}

template <> template <>
inline Vec3<float>::Vec3(const Vec3<int>& v) : x(v.x), y(v.y), z(v.z) {}


template <class T> std::ostream& operator<<(std::ostream& s, Vec2<T>& v) {
    s << "(" << v.x << ", " << v.y << ")\n";
    return s;
}

template <class T> std::ostream& operator<<(std::ostream& s, Vec3<T>& v) {
    s << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
    return s;
}

const int DEFAULT_D = 4;
class Matrix {
    std::vector<std::vector<float>> m;
    int nrow, ncol;
public:
    Matrix(int r=DEFAULT_D, int c=DEFAULT_D) :
        m(std::vector<std::vector<float>> (r, std::vector<float>(c, 0.f))),
        nrow(r), ncol(c) {}

    int get_nrow() { return nrow; }
    int get_ncol() { return ncol; }

    static Matrix identity(int dimensions) {
        Matrix E(dimensions, dimensions);
        for (int i = 0; i < dimensions; i++)
            E[i][i] = 1;
        return E;
    }

    std::vector<float>& operator[](const int i) {
        assert(i >= 0 && i < nrow);
        return m[i];
    }

    const std::vector<float>& operator[](const int i) const {
        assert(i >= 0 && i < nrow);
        return m[i];
    }

    Matrix operator*(const Matrix& a) {
        assert(this->ncol == a.nrow);
        Matrix res(this->nrow, a.ncol);
        for (int i = 0; i < this->nrow; i++) {
            for (int j = 0; j < a.ncol; j++) {
                res.m[i][j] = 0;
                for (int k = 0; k < this->ncol; k++)
                    res.m[i][j] += this->m[i][k]*a.m[k][j];
            }
        }
        return res;
    }

    Matrix transpose() {
        Matrix res(ncol, nrow);
        for (int i = 0; i < ncol; i++)
            for (int j = 0; j < nrow; j++)
                res.m[i][j] = m[j][i];
        return res;
    }
    Matrix inverse() {
        assert(nrow==ncol);
        // augmenting the square matrix with the identity matrix of the same dimensions a => [ai]
        Matrix result(nrow, ncol*2);
        for(int i=0; i<nrow; i++)
            for(int j=0; j<ncol; j++)
                result[i][j] = m[i][j];
        for(int i=0; i<nrow; i++)
            result[i][i+ncol] = 1;
        // first pass
        for (int i=0; i<nrow-1; i++) {
            // normalize the first row
            for(int j=result.ncol-1; j>=0; j--)
                result[i][j] /= result[i][i];
            for (int k=i+1; k<nrow; k++) {
                float coeff = result[k][i];
                for (int j=0; j<result.ncol; j++) {
                    result[k][j] -= result[i][j]*coeff;
                }
            }
        }
        // normalize the last row
        for(int j=result.ncol-1; j>=nrow-1; j--)
            result[nrow-1][j] /= result[nrow-1][nrow-1];
        // second pass
        for (int i=nrow-1; i>0; i--) {
            for (int k=i-1; k>=0; k--) {
                float coeff = result[k][i];
                for (int j=0; j<result.ncol; j++) {
                    result[k][j] -= result[i][j]*coeff;
                }
            }
        }
        // cut the identity matrix back
        Matrix truncate(nrow, ncol);
        for(int i=0; i<nrow; i++)
            for(int j=0; j<ncol; j++)
                truncate[i][j] = result[i][j+ncol];
        return truncate;
    }

    friend std::ostream& operator<<(std::ostream& s, Matrix& m);
};

inline std::ostream& operator<<(std::ostream& s, Matrix& m) {
    for (int i = 0; i < m.nrow; i++)  {
        for (int j = 0; j < m.ncol; j++) {
            s << m[i][j];
            if (j<m.ncol-1) s << "\t";
        }
        s << "\n";
    }
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






