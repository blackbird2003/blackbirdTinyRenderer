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
Vec3f eye(0, -1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

Matrix ModelView;
Matrix Projection;
Matrix Viewport;

struct GouraudShader : public IShader {
    Vec3f varying_intensity;
    //顶点着色
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert));
        glVertex = Viewport * Projection * ModelView * glVertex;
        varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
        return glVertex;
    }
    //片段着色 用于drawTrianglw
    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float intensity = varying_intensity * bar;
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    lookat(eye, center, up);
    viewport(width/8, height/8, width*3/4, height*3/4);
    projection(eye, center);
    light_dir.normalize();

    GouraudShader shader;

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

