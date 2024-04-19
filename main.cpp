#include <cmath>

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

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

int main(int argc, char** argv)
{
    TGAImage image(200, 200, TGAImage::RGB);

    Triangle2D<int> tri((std::vector<Vec2<int>>) {Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160)});
    drawSolidTriangle(tri, image, red);

    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}
