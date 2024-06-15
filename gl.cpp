#include "gl.h"
#include "widget.h"



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


//screen_coordinate = viewport * projection * modelview * world_coordinate
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
//    Matrix M_inv, T; M_inv.identity(); T.identity();
//    //thanks https://www.zhihu.com/question/447781866 for giving explanation
//    for (int i = 0; i < 3; i++) {
//        M_inv[0][i] = x[i];
//        M_inv[1][i] = y[i];
//        M_inv[2][i] = z[i];
//        T[i][3] = -center[i];
//    }
//    ModelView = M_inv * T;
    ModelView = Matrix::identity();
    for (int i=0; i<3; i++) {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
}


void projection(Vec3f eye, Vec3f center) {
    Projection = Matrix::identity();
    Projection[3][2] = -1.f / (eye - center).norm();
}


void viewport(int x, int y, int w, int h) {
    Viewport = Matrix ::identity();
    //Translation
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 255 / 2.f;
    //scale to [0, 1]
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 255 / 2.f;
}
//{
//    Matrix m = Matrix::identity(4);
//    //Translation
//    m[0][3] = x + w / 2.f;
//    m[1][3] = y + h / 2.f;
//    m[2][3] = 255 / 2.f;
//    //scale to [0, 1]
//    m[0][0] = w / 2.f;
//    m[1][1] = h / 2.f;
//    m[2][2] = 255 / 2.f;
//    return m;
//}

const TGAColor black = TGAColor(0, 0, 0, 0);
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);


Vec3f baryCentric(std::vector<Vec2f> tri, Vec2f P) {
    auto A = tri[0], B = tri[1], C = tri[2];
    Vec3f s[2];
    for (int i=2; i--; ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}



/*
template<size_t LEN,size_t DIM,typename T> vec<LEN,T> embed(const vec<DIM,T> &v, T fill=1) {
    vec<LEN,T> ret;
    for (size_t i=LEN; i--; ret[i]=(i<DIM?v[i]:fill));
    return ret;
}

template<size_t LEN,size_t DIM, typename T> vec<LEN,T> proj(const vec<DIM,T> &v) {
    vec<LEN,T> ret;
    for (size_t i=LEN; i--; ret[i]=v[i]);
    return ret;
}

 embed: low to high dimension
 proj: high to low dimension
*/


void drawTriangle(Vec4f tri[3], IShader *shader,
                  TGAImage &image, TGAImage &zbuffer) {
    //here pts is screen coords
//    Vec2f bboxmin(image.get_width()-1,  image.get_height()-1);
//    Vec2f bboxmax(0, 0);
//    Vec2f clamp(image.get_width()-1, image.get_height()-1);
//    for (int i=0; i<3; i++) {
//        bboxmin.x = std::max((float)0, std::min(bboxmin.x, tri[i][1] / tri[i][3]));
//        bboxmin.y = std::max((float)0, std::min(bboxmin.y, tri[i][2] / tri[i][3]));
//        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, tri[i][1] / tri[i][3]));
//        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, tri[i][2] / tri[i][3]));
//    }


    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::min(bboxmin[j], tri[i][j]/tri[i][3]);
            bboxmax[j] = std::max(bboxmax[j], tri[i][j]/tri[i][3]);
        }
    }

    Vec2i P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            std::vector<Vec2f> tri2D = {proj<2>(tri[0]/tri[0][3]), proj<2>(tri[1]/tri[1][3]), proj<2>(tri[2]/tri[2][3])};
            Vec3f bc  = baryCentric(tri2D, P);
            if (bc.x<0 || bc.y<0 || bc.z<0) continue;

            //关于这里为什么要对z和w分别插值，以z/w为深度，而不是直接用归一化的z来求
            //疑惑了很久，后来发现，GAMES101课程中也有人提出相关问题
            //关于透视矫正插值，下面链接的回答给出了完整的推导过程
            //https://www.zhihu.com/question/332096916
            float z = tri[0][2] * bc.x + tri[1][2] * bc.y + tri[2][2] * bc.z;
            float w = tri[0][3] * bc.x + tri[1][3] * bc.y + tri[2][3] * bc.z;
            int frag_depth = std::max(0, std::min(255, int(z/w+.5)));
            //zbuffer.get虽然是TGA Color类型，但作为zbuffer时不表示color，而是深度etc
            if (bc.x < 0 || bc.y < 0 || bc.z < 0 || frag_depth < zbuffer.get(P.x, P.y)[0])
                continue;
            TGAColor color;
            bool discard = shader->fragment(bc, color);
            if (!discard) {
                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                image.set(P.x, P.y, color);
            }
        }
    }
}
//
//void drawTriangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
//    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
//    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
//    for (int i=0; i<3; i++) {
//        for (int j=0; j<2; j++) {
//            bboxmin[j] = std::min(bboxmin[j], pts[i][j]/pts[i][3]);
//            bboxmax[j] = std::max(bboxmax[j], pts[i][j]/pts[i][3]);
//        }
//    }
//    Vec2i P;
//    TGAColor color;
//    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
//        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
//            Vec3f c = baryCentric({(Vec2f)proj<2>(pts[0]/pts[0][3]), (Vec2f)proj<2>(pts[1]/pts[1][3]), (Vec2f)proj<2>(pts[2]/pts[2][3])}, proj<2>(P));
//            float z = pts[0][2]*c.x + pts[1][2]*c.y + pts[2][2]*c.z;
//            float w = pts[0][3]*c.x + pts[1][3]*c.y + pts[2][3]*c.z;
//            int frag_depth = std::max(0, std::min(255, int(z/w+.5)));
//            if (c.x<0 || c.y<0 || c.z<0 || zbuffer.get(P.x, P.y)[0]>frag_depth) continue;
//            bool discard = shader.fragment(c, color);
//            if (!discard) {
//                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
//                image.set(P.x, P.y, color);
//            }
//        }
//    }
//}
//





