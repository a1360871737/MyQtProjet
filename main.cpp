#include "dialog.h"
#include "mainwindow.h"
#include <QApplication>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog d;
    d.show();
    return a.exec();
}