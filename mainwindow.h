#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "common.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    Client *mainclient;
    ~MainWindow();

signals:
      void closewindow();

public slots:
      void linkThread();

private slots:

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
