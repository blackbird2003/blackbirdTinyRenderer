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
const TGAColor black = TGAColor(0, 0, 0, 0);
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

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