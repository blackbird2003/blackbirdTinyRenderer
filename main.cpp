#include <cmath>
#include <cstdio>

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

//Bresenham
//Reference: https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C++
void drawLine(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor color)
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

Model *model = NULL;
const int width  = 800;
const int height = 800;
const int depth = 255;
const TGAColor black = TGAColor(0, 0, 0, 0);
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

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
            Vec3f bc  = tri.baryCentric(Vec2f(P.x, P.y));//toTriangle2D().baryCentric(P);
            if (bc.x<0 || bc.y<0 || bc.z<0) continue;
            int idx = P.x + P.y * width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                Vec2i P_uv = uv[0] * bc.x + uv[1] * bc.y + uv[2] * bc.z;
                TGAColor color = model->diffuse(P_uv);
                image.set(P.x, P.y, color);
            }
        }
    }
}

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

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    //Initialize Z-Buffer
    float *zbuffer = new float[width * height];
    for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());


    TGAImage image(width, height, TGAImage::RGB);

    int cnt = 0;

    Matrix projectionMatrix = projection(camera);
    Matrix viewportMatrix = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            //screen_coords[j] = Vec2i((v.x+1.)*width/2., (v.y+1.)*height/2.);

            //world -> screen:
            //3d coordinate -> homogeneous coordinates
            //-> projection trans(camera at (0,0,c), project to plane z = 0)
            //-> viewport trans(to make central at (w/2,h/2,d/2) and into a cubic(1,1,1))

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
            //triangle((Vec3i)screen_coords[0], (Vec3i)screen_coords[1], (Vec3i)screen_coords[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
        }
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;
    delete[] zbuffer;
    return 0;
}

