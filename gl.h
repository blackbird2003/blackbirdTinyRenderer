#pragma once

#include "geometry.h"
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

extern Matrix ModelView, Projection, Viewport;
extern Model *model;
extern Vec3f light_dir, eye, center, up;
extern int width, height;



//Bresenham
void drawLine(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor& color);

//screen_coordinate = viewport * projection * modelview * world_coordinate
void lookat(Vec3f eye, Vec3f center, Vec3f up); //modelview
void projection(Vec3f eye, Vec3f center);
void viewport(int x, int y, int w, int h);


struct IShader {
    virtual ~IShader() {}
    Vec3f varying_intensity; // written by vertex shader, read by fragment shader
    mat<2,3,float> varying_uv;        // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()
    virtual Vec4f  vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};


//void Render(Model *model, IShader &shader);


struct GouraudShader : public IShader {
    //顶点着色
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert));
        glVertex = Viewport * Projection * ModelView * glVertex;
        varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
        return glVertex;
    }
    //片段着色 用于drawTriangle
    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float intensity = varying_intensity * bar;
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

struct SixColorShader : public IShader {
    //顶点着色
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert));
        glVertex = Viewport * Projection * ModelView * glVertex;
        varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
        return glVertex;
    }
    //片段着色 用于drawTriangle
    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float intensity = varying_intensity*bar;
        if (intensity>.85) intensity = 1;
        else if (intensity>.60) intensity = .80;
        else if (intensity>.45) intensity = .60;
        else if (intensity>.30) intensity = .45;
        else if (intensity>.15) intensity = .30;
        else intensity = 0;
        color = TGAColor(255, 155, 0)*intensity;
        return false;
    }
};

struct TextureShader : public IShader {
    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert)*light_dir); // get diffuse lighting intensity
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        return Viewport*Projection*ModelView*gl_Vertex; // transform it to screen coordinates
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float intensity = varying_intensity*bar;   // interpolate intensity for the current pixel
        Vec2f uv = varying_uv*bar;                 // interpolate uv for the current pixel
        color = model->diffuse(uv)*intensity;      // well duh
        return false;                              // no, we do not discard this pixel
    }
};

struct NormalShader : public IShader {
    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        return Viewport*Projection*ModelView*gl_Vertex; // transform it to screen coordinates
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec2f uv = varying_uv*bar;                 // interpolate uv for the current pixel
        Vec3f n = proj<3>(uniform_MIT*embed<4>(model->normal(uv))).normalize();
        Vec3f l = proj<3>(uniform_M  *embed<4>(light_dir        )).normalize();
        float intensity = std::max(0.f, n*l);
        color = model->diffuse(uv)*intensity;      // well duh
        return false;                              // no, we do not discard this pixel
    }
};

struct PhoneShader : public IShader {
    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        return Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec2f uv = varying_uv*bar;
        Vec3f n = proj<3>(uniform_MIT*embed<4>(model->normal(uv))).normalize();
        Vec3f l = proj<3>(uniform_M  *embed<4>(light_dir        )).normalize();
        Vec3f r = (n*(n*l*2.f) - l).normalize();   // reflected light
        float spec = pow(std::max(r.z, 0.0f), model->specular(uv));
        float diff = std::max(0.f, n*l);
        TGAColor c = model->diffuse(uv);
        color = c;
        for (int i=0; i<3; i++) color[i] = std::min<float>(5 + c[i]*(diff + .6*spec), 255);
        return false;
    }
};

struct LighterPhoneShader : public IShader {
    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        Vec4f WorldCoords;
        Vec4f ScreenCoord = Viewport * Projection * ModelView * WorldCoords;
        ScreenCoord;
        return Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec2f uv = varying_uv*bar;
        Vec3f n = proj<3>(uniform_MIT*embed<4>(model->normal(uv))).normalize();
        Vec3f l = proj<3>(uniform_M  *embed<4>(light_dir        )).normalize();
        Vec3f r = (n*(n*l*2.f) - l).normalize();   // reflected light
        float spec = pow(std::max(r.z, 0.0f), model->specular(uv));
        float diff = std::max(0.f, n*l);
        TGAColor c = model->diffuse(uv);
        color = c;
        for (int i=0; i<3; i++) color[i] = std::min<float>(10 + c[i]*(2 * diff + 1.5*spec), 255);
        return false;
    }
};



//Iterate all points in the rectangular bounding box of triangle, draw if the point is inside
// 2024 04 26 2d->3d, texture mapping
// 2024 05 12 Reconstruction
void drawTriangle(Vec4f pts[3], IShader *shader,
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
