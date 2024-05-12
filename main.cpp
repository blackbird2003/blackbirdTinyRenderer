#include <cmath>
#include <cstdio>

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "gl.h"

Model *model = NULL;
const int width  = 800;
const int height = 800;//const int depth = 255; //set as default

Vec3f light_dir = Vec3f(1, 1, 1).normalize();
Vec3f eye(3, 3, 5);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

Matrix ModelView;
Matrix Projection;
Matrix Viewport;

struct GouraudShader : public IShader {
    Vec3f varying_intensity;
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()
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
    Vec3f varying_intensity;
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()
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
    Vec3f          varying_intensity; // written by vertex shader, read by fragment shader
    mat<2,3,float> varying_uv;        // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()

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
    mat<2,3,float> varying_uv;  // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()

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
    mat<2,3,float> varying_uv;  // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()

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
    mat<2,3,float> varying_uv;  // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()

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
        for (int i=0; i<3; i++) color[i] = std::min<float>(10 + c[i]*(2 * diff + 1.5*spec), 255);
        return false;
    }
};

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/diablo3_pose/diablo3_pose.obj");
        //model = new Model("obj/african_head/african_head.obj");
        //model = new Model("obj/boggie/body.obj");
        //model = new Model("obj/boggie/head.obj");
        //model = new Model("obj/boggie/eyes.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    lookat(eye, center, up);
    viewport(width/8, height/8, width*3/4, height*3/4);
    projection(eye, center);
    light_dir.normalize();

    //GouraudShader shader;
    //SixColorShader shader;
    //TextureShader shader;
    //NormalShader shader;
    PhoneShader shader;
    //LighterPhoneShader shader;
    shader.uniform_M   =  Projection*ModelView;
    shader.uniform_MIT = (Projection*ModelView).invert_transpose();
    int cnt = 0;
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f world_coords[3];
        Vec4f screen_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            world_coords[j] = v;
            screen_coords[j] = shader.vertex(i, j);
        }
        drawTriangle(screen_coords, shader, image, zbuffer);
        printf("%d ok\n", ++cnt);
    }

    image.flip_vertically();
    zbuffer.flip_vertically();
    image.write_tga_file("output.tga");
    zbuffer.write_tga_file("zbuffer.tga");
    delete model;
    return 0;
}

