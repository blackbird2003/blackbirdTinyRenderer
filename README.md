## TinyRender学习笔记

通过手写软光栅渲染器加深对计算机图形学基本原理的理解，并练习C++面向对象程序设计。

该项目主要参考[Home · ssloy/tinyrenderer Wiki (github.com)](https://github.com/ssloy/tinyrenderer/wiki)编写

推荐先过一下GAMES101

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

涉及导入模型，需要将工作目录设置为工程文件夹

![image-20240505230016266](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505220018544-333214793.png)

![image-20240505230043985](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505220047826-450326712.png)

但我的Clion存在tga图像无法加载的bug。在设置->编辑器->文件类型中去掉.tga，然后选择用本地程序打开即可。

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

三维物体模型通常以三角形为基础。为了方便表示点、向量、多边形，写geometry.h。

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



如何画出实心的三角形？一般来说，有扫描线和边界函数两种算法。

对于多线程的CPU,采用边界函数法更为高效：先找到三角形的矩形包围盒，再逐点判断是否在三角形中

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
    Vec3i P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc_screen  = tri.baryCentric(P.toVec2());//toTriangle2D().baryCentric(P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            
            //bugfix
            P.z = tri.depth[0] * bc.x + tri.depth[1] * bc.y + tri.depth[2] * bc.z;

            int idx = P.x+P.y*width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
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

同时，在Triangle2D类中加入depth数组即可

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
    Vec3i P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc  = tri.baryCentric(P.toVec2());//toTriangle2D().baryCentric(P);
            if (bc.x<0 || bc.y<0 || bc.z<0) continue;
            
            P.z = tri.depth[0] * bc.x + tri.depth[1] * bc.y + tri.depth[2] * bc.z;

            int idx = P.x+P.y*width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;

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

model.h和model.cpp需要修改以支持纹理。作者在lesson4的结尾放出了代码。

效果：



![image-20240426190246539](https://img2023.cnblogs.com/blog/1928276/202404/1928276-20240426180250012-1899969424.png)

这是一个平行投影的结果，损失了一部分真实感，例如，虽然耳朵旁边的头发在xoy平面上不与脸部重叠，但实际上应该被前边的皮肤遮挡，因为人眼/相机本身是“点光源”，而不是“平行光源”，物体发出的光线最终汇聚于一点，也就是所谓的“透视”。下面将引入透视投影：



## Lesson 4: Perspective projection









齐次坐标

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/f10.png)





简单变换（图来自GAMES101）

![image-20240505174927834](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505164930200-682100308.png)

逆变换

![image-20240505175454080](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505165458526-1584226714.png)

复合变换

![image-20240505175019922](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505165021790-531013206.png)







实现矩阵类：

```cpp
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
```



一个简单投影矩阵的推导：

假设相机位置为（0,0,c）成像平面为z=0，如图

<img src="https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/525d3930435c3be900e4c7956edb5a1c.png" alt="img" style="zoom: 67%;" />



根据三角形相似，x'/c = x/(c-z)，即有

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/f15.png)

同理

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/f16.png)

为了实现z轴方向上靠近相机的线段被拉伸，远离相机的线段被压缩，投影矩阵具有这样的形式

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/f13.png)

根据齐次坐标的结果，得到对应的投影点坐标

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/f14.png)

根据上面的结果，可知r=-1/c。

我们可以得到一个简单情况下的投影矩阵，变换过程如图

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/f17.png)





在程序中，这个过程用如下方式实现：

```cpp
screen_coords[j] = hc2v(viewportMatrix * projectionMatrix * v2hc(v));
```

(普通坐标 → 齐次坐标)

世界坐标 → (经投影变换)投影坐标 → （经视口变换)屏幕坐标 

(齐次坐标 → 普通坐标)

这里的坐标包含位置(x,y)和深度z,深度交给z-buffer来处理

视口变化的目的是将投影区域映射到[-1,1]^3的立方体中，便于绘制

相关变化的实现：

```cpp

//Transition between coordinates (vector type) and homogeneous coordinates (matrix type)
Matrix v2hc(const Vec3f &v) {
    Matrix hc(4, 1);
    hc[0][0] = v.x;
    hc[1][0] = v.y;
    hc[2][0] = v.z;
    hc[3][0] = 1;
    return hc;
}
Vec3f hc2v(const Matrix &hc) {
    return Vec3f(hc[0][0], hc[1][0], hc[2][0]) * (1.f / hc[3][0]);
}

Vec3f light_dir(0,0,-1);
Vec3f camera(0, 0, 3);
//project to z = 0
Matrix projection(const Vec3f &camera) {
    Matrix m = Matrix::identity(4);
    m[3][2] = -1.f/camera.z;
    return m;
}

//viewport(width / 8, height / 8, width * 0.75, height * 0.75);
//窗口边缘留出1/8空隙
Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    //Translation
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;
    //scale to [0, 1]
    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}

int main() {
	...

    Matrix projectionMatrix = projection(camera);
    Matrix viewportMatrix = viewport(width / 8, height / 8, width * 0.75, height * 0.75);

    ...
        
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            //world -> screen:
            //3d coordinate -> homogeneous coordinates
            //-> projection trans(camera at (0,0,c), project to plane z = 0)
            //-> viewport trans(to make central at (w/2,h/2,d/2))

            world_coords[j]  = v;
            screen_coords[j] = hc2v(viewportMatrix * projectionMatrix * v2hc(v));
        }

        //Still simplified light intensity
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
    ...
}
```





效果

![image-20240504170937200](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240504160940105-1636523864.png)





注：TinyRenderer的透视投影与GAMES101处理方式不同，GAMES101是把M\[3\]\[2\]固定为1，求解M的第三行，而此处是固定第三行为（0 0 1 0），求解M\[3\]\[2\]。

此处并没有“近平面”的概念，认为n=0,f=c。

下面是GAMES101给出的结果（第三行为0 0 A B）：

<img src="https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505182227248-1392365094.png" alt="image-20240505192224927" style="zoom:50%;" />



<img src="https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505182300021-22198105.png" alt="image-20240505192257807" style="zoom:50%;" />



## Lesson 5: Moving the camera

之前，我们考虑了相机在(0,0,c)，朝着-z方向看的情况。

对于任意的相机位置，需要三个向量来确定：相机坐标e，相机指向的点c，向上方向向量u,如图所示：

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/b94dd4a591514fd66a91a6e4cc065644.png)

我们假定相机总是朝着-z方向看，而u朝向正y方向，据此就得到了一个新的坐标系x'y'z'，

下面考虑如何将物体坐标[x,y,z]转化为新坐标系下的[x',y',z']。





![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f66a0139058ab1d1025dbfd8cd401389.png)

首先回顾坐标[x,y,z]的定义，它是三个正交的单位向量i,j,k前面的系数

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f00.png)

现在，我们有了新的单位向量i',j',k',那么一定存在矩阵M，使得

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f01.png)

我们将OP写成OO'+O'P,与新的单位坐标建立联系：

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f02.png)

将[i',j',k']用上面的式子表示，提出[i,j,k]:

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f03.png)

左边用[x,y,z]的定义式替换，就得到了[x',y',z']与[x,y,z]的关系

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f04.png)







关于look at的推导，此处写的有些混乱
建议参阅https://www.zhihu.com/question/447781866 

下面是个人理解：

简单来说，设M是(0, 0, 0),[i,j,k]到eyepos, [i',j',k']的变换矩阵
则M=TR,先旋转后平移

其中旋转矩阵R根据单位向量**左乘**该矩阵得到新单位向量，很容易得到(此处r,u,v是i',j',k'在原坐标系下的坐标)

![image-20240505220527743](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505210530096-395261047.png)

而T则为原点平移到eye pos的平移矩阵 （C是eyepos)

![image-20240505221319762](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505211321938-1985120681.png)

此为对坐标轴的变换矩阵，即，我们用M计算了**新的单位向量在原坐标系下的坐标**，而要得到原来单位向量在新坐标系下的坐标，显然应该左乘M的逆矩阵。这样，我们就求得了ModelView矩阵。



据此，编写lookup实现modelview的计算

```cpp
Vec3f light_dir = Vec3f(0, 0, -1).normalize();
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);
//Vec3f camera(0, 0, 3);

//screen_coordinate = viewport * projection * modelview * world_coordinate
Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();
    Matrix M_inv = Matrix::identity(4);
    Matrix T = Matrix::identity(4);
    //thanks https://www.zhihu.com/question/447781866
    for (int i = 0; i < 3; i++) {
        M_inv[0][i] = x[i];
        M_inv[1][i] = y[i];
        M_inv[2][i] = z[i];
        T[i][3] = -eye[i];
    }
    return M_inv * T;
}


Matrix projection(Vec3f eye, Vec3f center) {
    Matrix m = Matrix::identity(4);
    m[3][2] = -1.f / (eye - center).norm();
    //m[3][2] = -1.f / camera.z;
    return m;
}


int main() {
    ...
        
    Matrix modelviewMatrix = lookat(eye, center, up);
    Matrix projectionMatrix = projection(eye, center);
    Matrix viewportMatrix = viewport(width / 8, height / 8, width * 0.75, height * 0.75);

    ...
    screen_coords[j] = hc2v(viewportMatrix * projectionMatrix * modelviewMatrix * v2hc(v));
    ...
    
}
```

效果（目前有点bug)

![image-20240505225818988](https://img2023.cnblogs.com/blog/1928276/202405/1928276-20240505215822020-1676545529.png)






### Bouns：Transformation of normal vectors

为了处理光照，我们将模型进行坐标变换后，如果模型提供了每个面的法向量，还需要将法向量也进行变换。

此处有一个结论：模型上的坐标通过矩阵M进行仿射变换，那么模型的法向量的变换矩阵是M的逆矩阵的转置。

证明：考虑平面方程 Ax+By+Cz=0，它的法向量是（A,B,C) ,写成矩阵形式为：

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f05.png)

在两者之间插入M的逆和M：

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f06.png)

 由于坐标均为列向量，把左边写成转置形式：

![img](https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/05-camera/f07.png)

因此，如果对坐标(x,y,z)做变换M，要满足原来的直线方程，对法向量的变换矩阵为M的逆矩阵的转置（或者转置再求逆，转置和求逆是**可交换的**，证明略）







