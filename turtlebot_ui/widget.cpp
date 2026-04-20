#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // DbClient 생성
    pDb = new DbClient(this);

    if (pDb->isConnected()) {
        qDebug() << "DB 연결 확인!";
    }
}

Widget::~Widget()
{
    delete pDb;
    delete ui;
}
