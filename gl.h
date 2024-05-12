#pragma once

#include "geometry.h"
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

//Bresenham
void drawLine(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor& color);


extern Matrix ModelView;
extern Matrix Projection;
extern Matrix Viewport;



//screen_coordinate = viewport * projection * modelview * world_coordinate
void lookat(Vec3f eye, Vec3f center, Vec3f up); //modelview
void projection(Vec3f eye, Vec3f center);
void viewport(int x, int y, int w, int h);


struct IShader {
    virtual ~IShader() {}
    virtual Vec4f  vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

//Iterate all points in the rectangular bounding box of triangle, draw if the point is inside
// 2024 04 26 2d->3d, texture mapping
// 2024 05 12 Reconstruction
void drawTriangle(Vec4f pts[3], IShader &shader,
                  TGAImage &image, TGAImage &zbuffer);
//    Vec2f bboxmin(image.get_width()-1,  image.get_height()-1);
//    Vec2f bboxmax(0, 0);
//    Vec2f clamp(image.get_width()-1, image.get_height()-1);
//    for (int i=0; i<3; i++) {
//        bboxmin.x = std::max((float)0, std::min(bboxmin.x, tri.pt[i].x));
//        bboxmin.y = std::max((float)0, std::min(bboxmin.y, tri.pt[i].y));
//        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, tri.pt[i].x));
//        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, tri.pt[i].y));
//    }
//
//    Vec3i P;
//    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
//        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
//            Vec3f bc  = tri.baryCentric(Vec2f(P.x, P.y));//toTriangle2D().baryCentric(P);
//            if (bc.x<0 || bc.y<0 || bc.z<0) continue;
//
//            P.z = tri.depth[0] * bc.x + tri.depth[1] * bc.y + tri.depth[2] * bc.z;
//
//            int idx = P.x + P.y * width;
//            if (P.z > zbuffer[idx]) {
//                zbuffer[idx] = P.z;
//                Vec2i P_uv = uv[0] * bc.x + uv[1] * bc.y + uv[2] * bc.z;
//                TGAColor color = model->diffuse(P_uv);
//                image.set(P.x, P.y, color);
//            }
//        }
//    }
//}
