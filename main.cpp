#include <QApplication>
#include <cmath>
#include <cstdio>

#include "widget.h"

using namespace std;

int main(int argc, char** argv) {
    QApplication a(argc, argv);
    Widget w;
    w.resize(1000, 600);
    w.setWindowTitle("Blackbird Renderer");
    w.show();
    return a.exec();
    //delete model;
    return 0;
}



