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
