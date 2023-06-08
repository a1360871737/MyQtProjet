#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "common.h"
#include "mainwindow.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

signals:
      void closedaig();
      void mainshow();

private slots:

private:
    Ui::Dialog *ui;
    int conneted;
    Client *myclient;
    MainWindow mainwindow;
};

#endif // DIALOG_H
