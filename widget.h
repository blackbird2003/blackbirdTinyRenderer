#pragma once

#include <QWidget>
#include <QImage>
#include <QImageReader>
#include "gl.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

extern Matrix ModelView, Projection, Viewport;
extern Model *model;
extern Vec3f light_dir, eye, center, up;
extern int width, height;

void Render(Model *model, IShader &shader);

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void updateImg();


private slots:
    void on_btnRender_clicked();

    void on_cboxShader_currentIndexChanged(int index);

    void on_sboxEyeX_valueChanged(int arg1);

    void on_sboxEyeY_valueChanged(int arg1);

    void on_sboxEyeZ_valueChanged(int arg1);

    void on_cboxModel_currentIndexChanged(int index);

    void on_cboxModels_currentIndexChanged(int index);

private:
    Ui::Widget *ui;
};

