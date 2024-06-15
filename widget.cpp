#include "widget.h"
#include "./ui_widget.h"
#include <bits/stdc++.h>
#include "gl.h"

int width  = 800;
int height = 800; //const int depth = 255; //as default

Vec3f light_dir = Vec3f(1, 1, 1).normalize();
Vec3f eye(3, 3, 5);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

Matrix ModelView;
Matrix Projection;
Matrix Viewport;

GouraudShader gouraudShader;
SixColorShader sixColorShader;
TextureShader textureShader;
NormalShader normalShader;
PhoneShader phoneShader;
LighterPhoneShader lighterPhoneShader;

Model model0 = Model("obj/african_head/african_head.obj");
Model model1 = Model("obj/boggie/body.obj");
Model model2 = Model("obj/diablo3_pose/diablo3_pose.obj");
Model *model = &model0;
IShader *shader = &phoneShader;


void Render(Model *model, IShader *shader) {
    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
    lookat(eye, center, up);
    viewport(width/8, height/8, width*3/4, height*3/4);
    projection(eye, center);
    light_dir.normalize();

    shader->uniform_M =  Projection*ModelView;
    shader->uniform_MIT = (Projection*ModelView).invert_transpose();
    int cnt = 0;
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f world_coords[3];
        Vec4f screen_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            world_coords[j] = v;
            screen_coords[j] = shader->vertex(i, j);
        }
        drawTriangle(screen_coords, shader, image, zbuffer);
        //printf("%d ok\n", ++cnt);
    }

    // image.flip_vertically();
    // zbuffer.flip_vertically();
    bool okwrite = image.write_tga_file("output.tga");
    qDebug() << "okwrite" << okwrite;
    zbuffer.write_tga_file("zbuffer.tga");
}

QImage loadTga(const char* filePath, bool &success)
{
    QImage img;
    if (!img.load(filePath))
    {

        // open the file
        std::fstream fsPicture(filePath, std::ios::in | std::ios::binary);

        if (!fsPicture.is_open())
        {
            img = QImage(1, 1, QImage::Format_RGB32);
            img.fill(Qt::red);
            success = false;
            return img;
        }

        // some variables
        std::vector<std::uint8_t>* vui8Pixels;
        std::uint32_t ui32BpP;
        std::uint32_t ui32Width;
        std::uint32_t ui32Height;

        // read in the header
        std::uint8_t ui8x18Header[19] = { 0 };
        fsPicture.read(reinterpret_cast<char*>(&ui8x18Header), sizeof(ui8x18Header) - 1);

        //get variables
        vui8Pixels = new std::vector<std::uint8_t>;
        bool bCompressed;
        std::uint32_t ui32IDLength;
        std::uint32_t ui32PicType;
        std::uint32_t ui32PaletteLength;
        std::uint32_t ui32Size;

        // extract all information from header
        ui32IDLength = ui8x18Header[0];
        ui32PicType = ui8x18Header[2];
        ui32PaletteLength = ui8x18Header[6] * 0x100 + ui8x18Header[5];
        ui32Width = ui8x18Header[13] * 0x100 + ui8x18Header[12];
        ui32Height = ui8x18Header[15] * 0x100 + ui8x18Header[14];
        ui32BpP = ui8x18Header[16];

        // calculate some more information
        ui32Size = ui32Width * ui32Height * ui32BpP / 8;
        bCompressed = ui32PicType == 9 || ui32PicType == 10;
        vui8Pixels->resize(ui32Size);

        // jump to the data block
        fsPicture.seekg(ui32IDLength + ui32PaletteLength, std::ios_base::cur);

        if (ui32PicType == 2 && (ui32BpP == 24 || ui32BpP == 32))
        {
            fsPicture.read(reinterpret_cast<char*>(vui8Pixels->data()), ui32Size);
        }
        // else if compressed 24 or 32 bit
        else if (ui32PicType == 10 && (ui32BpP == 24 || ui32BpP == 32))	// compressed
        {
            std::uint8_t tempChunkHeader;
            std::uint8_t tempData[5];
            unsigned int tempByteIndex = 0;

            do {
                fsPicture.read(reinterpret_cast<char*>(&tempChunkHeader), sizeof(tempChunkHeader));

                if (tempChunkHeader >> 7)	// repeat count
                {
                    // just use the first 7 bits
                    tempChunkHeader = (uint8_t(tempChunkHeader << 1) >> 1);

                    fsPicture.read(reinterpret_cast<char*>(&tempData), ui32BpP / 8);

                    for (int i = 0; i <= tempChunkHeader; i++)
                    {
                        vui8Pixels->at(tempByteIndex++) = tempData[0];
                        vui8Pixels->at(tempByteIndex++) = tempData[1];
                        vui8Pixels->at(tempByteIndex++) = tempData[2];
                        if (ui32BpP == 32) vui8Pixels->at(tempByteIndex++) = tempData[3];
                    }
                }
                else						// data count
                {
                    // just use the first 7 bits
                    tempChunkHeader = (uint8_t(tempChunkHeader << 1) >> 1);

                    for (int i = 0; i <= tempChunkHeader; i++)
                    {
                        fsPicture.read(reinterpret_cast<char*>(&tempData), ui32BpP / 8);

                        vui8Pixels->at(tempByteIndex++) = tempData[0];
                        vui8Pixels->at(tempByteIndex++) = tempData[1];
                        vui8Pixels->at(tempByteIndex++) = tempData[2];
                        if (ui32BpP == 32) vui8Pixels->at(tempByteIndex++) = tempData[3];
                    }
                }
            } while (tempByteIndex < ui32Size);
        }
        // not useable format
        else
        {
            fsPicture.close();
            img = QImage(1, 1, QImage::Format_RGB32);
            img.fill(Qt::red);
            success = false;
            return img;
        }

        fsPicture.close();

        img = QImage(ui32Width, ui32Height, QImage::Format_RGB888);

        int pixelSize = ui32BpP == 32 ? 4 : 3;
        //TODO: write direct into img
        for (unsigned int x = 0; x < ui32Width; x++)
        {
            for (unsigned int y = 0; y < ui32Height; y++)
            {
                int valr = vui8Pixels->at(y * ui32Width * pixelSize + x * pixelSize + 2);
                int valg = vui8Pixels->at(y * ui32Width * pixelSize + x * pixelSize + 1);
                int valb = vui8Pixels->at(y * ui32Width * pixelSize + x * pixelSize);

                QColor value(valr, valg, valb);
                img.setPixelColor(x, y, value);
            }
        }

        img = img.mirrored();

    }
    success = true;
    return img;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setStyleSheet("background-color: white;");
    QImage *img = new QImage;
    QString imgPath = "./output.tga";
    ui->label->setGeometry(200,0,800,600);//前两个参数表示label左上角位置后面分别是宽和高
    ;
    bool ok = 1;
    *img = loadTga(imgPath.toStdString().c_str(), ok);
    //img->scaled(400,300,Qt::KeepAspectRatio);
    ui->label->setPixmap(QPixmap::fromImage(*img));
    free(img);
}

void Widget::updateImg() {
    QImage *img = new QImage;
    QString imgPath = "./output.tga";
    ui->label->setGeometry(200,0,800,600);//前两个参数表示label左上角位置后面分别是宽和高
    ;
    bool ok = 1;
    *img = loadTga(imgPath.toStdString().c_str(), ok);
    //img->scaled(400,300,Qt::KeepAspectRatio);
    ui->label->setPixmap(QPixmap::fromImage(*img));
    free(img);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_btnRender_clicked()
{
    Render(model, shader);
    updateImg();
}



void Widget::on_cboxShader_currentIndexChanged(int index)
{
    qDebug() << "cboxShader" << index;
    if (index == 0) shader = &gouraudShader;
    if (index == 1) shader = &sixColorShader;
    if (index == 2) shader = &textureShader;
    if (index == 3) shader = &normalShader;
    if (index == 4) shader = &phoneShader;
    if (index == 5) shader = &lighterPhoneShader;
    Render(model, shader);
    updateImg();
}


void Widget::on_sboxEyeX_valueChanged(int arg1)
{
    eye.x = arg1;
    Render(model, shader);
    updateImg();
}


void Widget::on_sboxEyeY_valueChanged(int arg1)
{
    eye.y = arg1;
    Render(model, shader);
    updateImg();
}


void Widget::on_sboxEyeZ_valueChanged(int arg1)
{
    eye.z = arg1;
    Render(model, shader);
    updateImg();
}





void Widget::on_cboxModels_currentIndexChanged(int index)
{
    qDebug() << "cboxModel" << index;
    if (index == 0) model = &model0;
    if (index == 1) model = &model1;
    if (index == 2) model = &model2;
    Render(model, shader);
    updateImg();
}


void Widget::on_cboxModel_currentIndexChanged(int index)
{
    qDebug() << "cboxModel" << index;
    if (index == 0) model = &model0;
    if (index == 1) model = &model1;
    if (index == 2) model = &model2;
    Render(model, shader);
    updateImg();
}

