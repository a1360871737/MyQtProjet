#include "dialog.h"
#include "ui_dialog.h"
#include "common.h"
#include <QDebug>
#include <QMessageBox>
#include <QThread>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->label->setPixmap(QPixmap(":/res/Huawei-logo.png"));
    setWindowIcon(QIcon(":/res/Huawei-logo.png"));

    QMessageBox *box = new QMessageBox(this);
    box->setWindowTitle("Connecting......");
    box->setIcon(QMessageBox::Information); // 设置消息图标为信息图标
    box->setStandardButtons(QMessageBox::Cancel); // 设置按钮类型为取消按钮
    box->setDefaultButton(QMessageBox::Cancel); // 设置默认按钮为取消按钮
    box->setText("连接中....");
    box->hide();
    myclient = new Client();

    QThread *t = new QThread();
    myclient->moveToThread(t);
    conneted = 0;
    mainwindow.mainclient = myclient;
    connect(this, &Dialog::mainshow, &mainwindow, &MainWindow::linkThread);
    connect(myclient, &Client::loginOK, this,[=](){
        this->hide();
        mainwindow.show();
        emit mainshow();
    });

    connect(myclient, &Client::loginFailed, this,[=](){
        box->show();
        box->setText("登录失败");
    });

    connect(myclient, &Client::RegFailed, this,[=](){
        box->show();
        box->setText("账户已存在");
    });

    connect(myclient, &Client::connectOK, myclient,[=](){
        QString usr = ui->lineEdit_usr->text();
        QString pas = ui->lineEdit_pass->text();
        myclient->sendMessage("login:"+usr + ";" + pas);
    });

    connect(myclient, &Client::connectOK, this,[=](){
        box->hide();
        conneted++;
    });
    connect(myclient, &Client::connectERR, this,[=](){
        box->setText("连接失败");
    });

    connect(ui->pushButtonLogin, &QPushButton::clicked, this, [=](){
        if (conneted == 0)
        {
            box->show();
        }
        else
        {
            box->hide();
        }
    });
    connect(ui->pushButtonLogin, &QPushButton::clicked, myclient, [=](){
        if (conneted == 0)
        {
            qDebug() << "conet";
            myclient->connectToServer("101.35.247.40", 8888);
        }
        else
        {
            QString usr = ui->lineEdit_usr->text();
            QString pas = ui->lineEdit_pass->text();
            myclient->sendMessage("login:"+usr + ";" + pas);
        }
    });

    connect(ui->pushButton_exit, &QPushButton::clicked, myclient, [=](){
        myclient->disconnectFromServer();
    });

    connect(ui->pushButton_exit, &QPushButton::clicked, this, [=](){
        this->close();
    });

    connect(ui->pushButton_reg, &QPushButton::clicked, myclient, [=](){
        QString usr = ui->lineEdit_usr->text();
        QString pas = ui->lineEdit_pass->text();
        myclient->sendMessage("regin:"+usr + ";" + pas);
    });

    connect(this, &Dialog::closedaig, myclient, [=](){
        myclient->disconnectFromServer();
    });
    t->start();
}

Dialog::~Dialog()
{
    emit closedaig();
    delete ui;
}

