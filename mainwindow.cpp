#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //setFixedSize(500,500);
    setWindowIcon(QIcon(":/res/Huawei-logo.png"));
    setWindowTitle("服务开始");
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap pix;
    pix.load(":/res/bakeground.jpg");
    painter.drawPixmap(0,0, this->width(), this->height(),pix);
    return;
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

void MainWindow::on_pushButton_Open_clicked()
{
    QString path = QFileDialog::getOpenFileName();
    if(path.isEmpty())
    {
        QMessageBox::warning(this, "打开文件", "路径为空");
        return;
    }
    ui->lineEdit_Path->setText(path);
}

void MainWindow::on_pushButton_send_clicked()
{
    QString path = ui->lineEdit_Path->text();
    connect(this, &MainWindow::sendFiletoSer, mainclient, [=](){
        QString path = ui->lineEdit_Path->text();
        mainclient->sendFile(path);
    });
    emit sendFiletoSer();

    return;
}
