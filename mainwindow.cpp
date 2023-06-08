#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(500,500);
    setWindowIcon(QIcon(":/res/Huawei-logo.png"));
    setWindowTitle("服务开始");
}

void MainWindow::linkThread()
{
    connect(ui->pushButton, &QPushButton::clicked, mainclient, [=](){
        mainclient->sendMessage("11111");
    });

}


MainWindow::~MainWindow()
{
    delete ui;
}

