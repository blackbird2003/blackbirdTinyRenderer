## blackbirdTinyRender

通过手写软光栅渲染器加深对计算机图形学基本原理的理解，并练习C++面向对象程序设计。

该项目主要参考[Home · ssloy/tinyrenderer Wiki (github.com)](https://github.com/ssloy/tinyrenderer/wiki)编写

此外，还会参考[GAMES101: 现代计算机图形学入门 (ucsb.edu)](https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html)中的作业框架

## Lesson 0 Getting Started

### Using TGA image format

使用这个基本框架来生成TGA格式图像：
[ssloy/tinyrenderer at 909fe20934ba5334144d2c748805690a1fa4c89f (github.com)](https://github.com/ssloy/tinyrenderer/tree/909fe20934ba5334144d2c748805690a1fa4c89f)

只需 `#include "tgaimage.h"` ，并在编译时链接tgaimage.cpp即可。

例：在屏幕上将像素(52,41)设置为红色

```cpp
#include "tgaimage.h"
//Set color with RGB
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
int main(int argc, char** argv) {
    	//Set image size
        TGAImage image(100, 100, TGAImage::RGB);
    	//Set pixel color
        image.set(52, 41, red);
        //To have the origin at the left bottom corner of the image
        image.flip_vertically(); 
        image.write_tga_file("output.tga");
        return 0;
}
```

个人推荐的环境：Clion + CMake。（因为VsCode CMake调试功能实在搞不懂=.=)

CMakeLists.txt这样写就可以

```cmake
project(tinyRenderer)
add_executable(tinyRender main.cpp tgaimage.cpp)
```

由于图像生成在cmake-build-debug下，需在窗口设置“显示排除的文件”

![image-20240419181611460](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240419171612190-1290655735.png)

但我的Clion存在tga图像无法加载的bug。在设置->编辑器->文件类型中去掉.tga，然后选择用本地程序打开，虽然不太方便但勉强可用。

## Lesson 1 Bresenham’s Line Drawing Algorithm

使用Bresenham算法绘制线段。

原理：https://en.wikipedia.org/wiki/Bresenham's_line_algorithm

实现参考：https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C++

建议绘制斜率小于-1，-1到0,0到1,大于1，以及水平和垂直的直线来检验算法正确性。

```cpp
#include <cmath>

#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

void line(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor color)
{
    //Ensure that slope in (0, 1)
    const bool steep = (std::abs(y2 - y1) > std::abs(x2 - x1));
    if (steep) {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    const float dx = x2 - x1;
    const float dy = fabs(y2 - y1);

    float error = dx / 2.0f;
    const int ystep = (y1 < y2) ? 1 : -1;
    int y = (int)y1;

    const int maxX = (int)x2;

    for (int x = (int)x1; x <= maxX; x++) {
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }

        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

int main(int argc, char** argv)
{
    TGAImage image(100, 100, TGAImage::RGB);
    line(13, 20, 80, 40, image, red);
    line(55, 33, 22, 66, image, blue);
    line(33, 33, 66, 66, image, white);
    line(44, 20, 44, 80, image, white);
    line(20, 44, 80, 44, image, white);
    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}

```

效果

![image-20240419183705501](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240419173705372-1981687689.png)

## Lesson 2: Triangle rasterization and back face culling

三维物体模型通常以三角形为基础。为了方便表示点、向量、多边形，写一个geometry.h。

```cpp
#pragma once

template <class t> struct Vec2 {
    union {
        struct {t u, v;};
        struct {t x, y;};
        t raw[2];
    };
    Vec2() : u(0), v(0) {}
    Vec2(t _u, t _v) : u(_u),v(_v) {}
    inline Vec2<t> operator +(const Vec2<t> &V) const { return Vec2<t>(u+V.u, v+V.v); }
    inline Vec2<t> operator -(const Vec2<t> &V) const { return Vec2<t>(u-V.u, v-V.v); }
    inline Vec2<t> operator *(float f)          const { return Vec2<t>(u*f, v*f); }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec2<t>& v);
};

template <class t> struct Vec3 {
    union {
        struct {t x, y, z;};
        struct { t ivert, iuv, inorm; };
        t raw[3];
    };
    Vec3() : x(0), y(0), z(0) {}
    Vec3(t _x, t _y, t _z) : x(_x),y(_y),z(_z) {}
    inline Vec3<t> operator ^(const Vec3<t> &v) const { return Vec3<t>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
    inline Vec3<t> operator +(const Vec3<t> &v) const { return Vec3<t>(x+v.x, y+v.y, z+v.z); }
    inline Vec3<t> operator -(const Vec3<t> &v) const { return Vec3<t>(x-v.x, y-v.y, z-v.z); }
    inline Vec3<t> operator *(float f)          const { return Vec3<t>(x*f, y*f, z*f); }
    inline t       operator *(const Vec3<t> &v) const { return x*v.x + y*v.y + z*v.z; }
    float norm () const { return std::sqrt(x*x+y*y+z*z); }
    Vec3<t> & normalize(t l=1) { *this = (*this)*(l/norm()); return *this; }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec3<t>& v);
};

typedef Vec2<float> Vec2f;
typedef Vec2<int>   Vec2i;
typedef Vec3<float> Vec3f;
typedef Vec3<int>   Vec3i;

template <class t> std::ostream& operator<<(std::ostream& s, Vec2<t>& v) {
    s << "(" << v.x << ", " << v.y << ")\n";
    return s;
}

template <class t> std::ostream& operator<<(std::ostream& s, Vec3<t>& v) {
    s << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
    return s;
}
```



如何画出实心的三角形

对于多线程的CPU,采用下面的方式更为高效：先找到三角形的矩形包围盒，再逐点判断是否在三角形中

```cpp
triangle(vec2 points[3]) { 
    vec2 bbox[2] = find_bounding_box(points); 
    for (each pixel in the bounding box) { 
        if (inside(points, pixel)) { 
            put_pixel(pixel); 
        } 
    } 
}
```

因此，问题变成了给定三角形的三个点，如何判断点是否在三角形内部

一种最好的办法是，计算给定点关于给定三角形的重心坐标（或者叫面积坐标）。

维基百科：https://zh.wikipedia.org/wiki/%E9%87%8D%E5%BF%83%E5%9D%90%E6%A0%87

简单来说，它表示一个点所对的三条边形成的三角形面积比。如果点在三角形外部，则有一个维度是负的。

![image-20240419185822971](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240419175823218-1822502417.png)

由于tinyrenderer的作者写得有些丑陋，我在geometry.h里直接加入了polygon和triangle类，来实现重心坐标计算和点在三角形内的检测

```cpp
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
    Triangle2D(std::vector<Vec2<T>> _pt): Polygon2D<T>(3, _pt) {}
    Vec3f baryCentric(Vec2i P) {
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
```

在main.cpp里绘制实心三角形

```cpp
//Iterate all points in the rectangular bounding box of triangle, draw if the point is inside
void drawSolidTriangle(Triangle2D<int> tri, TGAImage &image, TGAColor color) {
    Vec2i bboxmin(image.get_width()-1,  image.get_height()-1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width()-1, image.get_height()-1);
    for (int i=0; i<3; i++) {
        bboxmin.x = std::max(0, std::min(bboxmin.x, tri.pt[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, tri.pt[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, tri.pt[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, tri.pt[i].y));
    }
    Vec2i P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc_screen  = tri.baryCentric(P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            image.set(P.x, P.y, color);
        }
    }
};
```

得到如图效果：

<img src="https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240419185714797-2050127591.png" alt="image-20240419195714296" style="zoom:50%;" />

三角形绘制完成后，可以尝试导入作者提供的由三角形构成的人脸模型。

.obj模型文件的格式如下

```
# List of geometric vertices, with (x, y, z, [w]) coordinates, w is optional and defaults to 1.0.
v 0.123 0.234 0.345 1.0
v ...
...
# List of texture coordinates, in (u, [v, w]) coordinates, these will vary between 0 and 1. v, w are optional and default to 0.
vt 0.500 1 [0]
vt ...
...
# List of vertex normals in (x,y,z) form; normals might not be unit vectors.
vn 0.707 0.000 0.707
vn ...
...
# Parameter space vertices in (u, [v, w]) form; free form geometry statement (see below)
vp 0.310000 3.210000 2.100000
vp ...
...
# Polygonal face element (see below)
f 1 2 3
f 3/1 4/2 5/3
f 6/4/1 3/5/3 7/6/5
f 7//1 8//2 9//3
f ...
...
# Line element (see below)
l 5 8 1 2 4 9
```

目前，我们暂时不关心模型的深度（z坐标），只是将模型正投影到XY平面上，则模型上的点对应的屏幕坐标可以这样简单的计算

```cpp
screen_coords[j] = Vec2i((v.x+1.)*width/2., (v.y+1.)*height/2.);
```

假设光从正前方射向正后方，即光线方向(0,0,-1)。

在这里，我们使用一种简化的亮度计算方法：我们忽略面与光源之间的距离差异，认为正对着光源的面（法线与光线方向相同）最亮，这样就可以计算每个三角形面的单位法向量与光线方向的叉积来代表亮度。

```cpp
int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    Vec3f light_dir(0,0,-1);
    int cnt = 0;
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec2i screen_coords[3];
        Vec3f world_coords[3];
        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec2i((v.x+1.)*width/2., (v.y+1.)*height/2.);
            world_coords[j]  = v;
        }
        Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
        n.normalize();
        float intensity = n*light_dir;
        if (intensity>0) {
            printf("ok %d\n", ++cnt);
            drawSolidTriangle(Triangle2D<int>({screen_coords[0], screen_coords[1], screen_coords[2]}), image, TGAColor(intensity*255, intensity*255, intensity*255, 255));
        }
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
```

在这种简化下，得到的渲染结果如下：
![image-20240419205641217](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240419195642124-634629665.png)

可以发现，位于口腔中的三角形遮住了嘴唇。下一节课中，我们将考虑深度测试，正确处理多边形的遮挡关系。



## Lesson 3: Hidden faces removal (z buffer)

深度检测算法的基本原理是，引入一个大小为像素数量的Z-Buffer数组，初始化所有像素点深度为负无穷。

在遍历像素点时，比较当前三角形上点的深度是否小于Z-Buffer的数值，如果小于，则更新该像素并更新Z-Buffer。

为此，我们需要为屏幕坐标增加一维深度（对于上面的人脸设置为模型的z即可）。在drawSolidTriangle()中增加对深度缓冲区的判断。

```cpp
//Iterate all points in the rectangular bounding box of triangle, draw if the point is inside
// 2024 04 26 2d->3d
void drawSolidTriangle(Triangle2D<float> tri, TGAImage &image, TGAColor color, float *zbuffer) {
    Vec2f bboxmin(image.get_width()-1,  image.get_height()-1);
    Vec2f bboxmax(0, 0);
    Vec2f clamp(image.get_width()-1, image.get_height()-1);
    for (int i=0; i<3; i++) {
        bboxmin.x = std::max((float)0, std::min(bboxmin.x, tri.pt[i].x));
        bboxmin.y = std::max((float)0, std::min(bboxmin.y, tri.pt[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, tri.pt[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, tri.pt[i].y));
    }
    Vec3f P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc_screen  = tri.baryCentric(P.toVec2());//toTriangle2D().baryCentric(P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            if (zbuffer[int(P.x+P.y*width)]<P.z) {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
};




Vec3f worldToScreen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    float *zbuffer = new float[width * height];
    for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());


    TGAImage image(width, height, TGAImage::RGB);
    Vec3f light_dir(0,0,-1);
    int cnt = 0;
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            //screen_coords[j] = Vec2i((v.x+1.)*width/2., (v.y+1.)*height/2.);
            world_coords[j]  = v;
            screen_coords[j] = worldToScreen(v);
        }

        Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
        n.normalize();
        float intensity = n*light_dir;

        if (intensity>0) {
            printf("ok %d\n", ++cnt);
            drawSolidTriangle(Triangle2D<float>({screen_coords[0], screen_coords[1], screen_coords[2]}), image, TGAColor(intensity*255, intensity*255, intensity*255, 255), zbuffer);
        }
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
```

同时，在Triangle2D类中加入深度参数

```cpp
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



```

效果如图所示：
![image-20240426175739547](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240426165741120-55012551.png)

### Bouns: Texture Mapping

在.obj文件中，有以“vt u v”开头的行，它们给出了一个纹理坐标数组。 

> The number in the middle (between the slashes) in the facet lines "f x/x/x x/x/x x/x/x" are the texture coordinates of this vertex of this triangle. Interpolate it inside the triangle, multiply by the width-height of the texture image and you will get the color to put in your render.

tinyrender作者提供了漫反射纹理： [african_head_diffuse.tga](..\Downloads\african_head_diffuse.tga) 

据此，我们可以给上述人脸模型添加纹理。此时，main函数中drawSolidTriangle函数里不需要再传入颜色，只需要传入intensity即可，另外需要传入当前三角形三个点的纹理坐标uv。

```cpp
//Iterate all points in the rectangular bounding box of triangle, draw if the point is inside
// 2024 04 26 2d->3d, texture mapping
void drawSolidTriangle(Triangle2D<float> tri, Vec2i* uv, TGAImage &image, float intensity, float *zbuffer) {
    Vec2f bboxmin(image.get_width()-1,  image.get_height()-1);
    Vec2f bboxmax(0, 0);
    Vec2f clamp(image.get_width()-1, image.get_height()-1);
    for (int i=0; i<3; i++) {
        bboxmin.x = std::max((float)0, std::min(bboxmin.x, tri.pt[i].x));
        bboxmin.y = std::max((float)0, std::min(bboxmin.y, tri.pt[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, tri.pt[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, tri.pt[i].y));
    }
    Vec3f P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc  = tri.baryCentric(P.toVec2());//toTriangle2D().baryCentric(P);
            if (bc.x<0 || bc.y<0 || bc.z<0) continue;
            if (zbuffer[int(P.x+P.y*width)]<P.z) {
                zbuffer[int(P.x + P.y * width)] = P.z;

                Vec2i P_uv = uv[0] * bc.x + uv[1] * bc.y + uv[2] * bc.z;
                TGAColor color = model->diffuse(P_uv);
                image.set(P.x, P.y, color);
            }
        }
    }
};




Vec3f worldToScreen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    float *zbuffer = new float[width * height];
    for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());


    TGAImage image(width, height, TGAImage::RGB);
    Vec3f light_dir(0,0,-1);
    int cnt = 0;
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            //screen_coords[j] = Vec2i((v.x+1.)*width/2., (v.y+1.)*height/2.);
            world_coords[j]  = v;
            screen_coords[j] = worldToScreen(v);
        }

        Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
        n.normalize();
        float intensity = n*light_dir;

        if (intensity>0) {
            printf("ok %d\n", ++cnt);
            Vec2i uv[3];
            for (int j = 0; j < 3; j++) uv[j] = model->uv(i, j);
            drawSolidTriangle(Triangle2D<float>({screen_coords[0], screen_coords[1], screen_coords[2]}), uv, image, intensity, zbuffer);
        }
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
```

model.h和model.cpp需要修改以支持纹理。将纹理.tga文件仿在模型同文件夹下。

下面的代码来自知乎

```cpp
/*
作者：吴腾
链接：https://zhuanlan.zhihu.com/p/523503467
来源：知乎
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
*/
//model.h
#pragma once

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<Vec3i>> faces_;
    std::vector<Vec3f> norms_;
    std::vector<Vec2f> uv_;
    TGAImage diffuseMap_;
    void loadTexture(std::string filename, const char* suffix, TGAImage& image);
public:
    Model(const char *filename);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int i);
    std::vector<int> face(int idx);
    Vec2i uv(int iface, int nvert);
    TGAColor diffuse(Vec2i uv);
};
//model.cpp
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), norms_(), uv_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        else if(!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
        }
        else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f normal;
            for (int i = 0; i < 3; i++) iss >> normal[i];
            norms_.push_back(normal);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; i++) tmp[i]--;
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
    loadTexture(filename, "_diffuse.tga", diffuseMap_);
}


Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    std::vector<Vec3i> tmp = faces_[idx];
    for (int i = 0; i < tmp.size(); i++)
        face.push_back(tmp[i][0]);
    return face;
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

void Model::loadTexture(std::string filename, const char* suffix, TGAImage& image)
{
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (image.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        image.flip_vertically();
    }
}

TGAColor Model::diffuse(Vec2i uv)
{
    return diffuseMap_.get(uv.x, uv.y);
}

Vec2i Model::uv(int iface, int nvert)
{
    int idx = faces_[iface][nvert][1];
    return Vec2i(uv_[idx].x * diffuseMap_.get_width(), uv_[idx].y * diffuseMap_.get_height());
}
```

效果
![image-20240426190246539](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240426180250012-1899969424.png)