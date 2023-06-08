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

    void paintEvent(QPaintEvent *event);
signals:
      void closewindow();
      void sendFiletoSer();
public slots:
      void linkThread();

private slots:
      void on_pushButton_Open_clicked();
      void on_pushButton_send_clicked();


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
