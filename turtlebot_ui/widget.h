#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    // DB INSERT 함수들
    void insertVoiceCommand(int target_robot_id, QString raw_text,
                            QString parsed_action, QString parsed_target,
                            bool success);
    void insertPickingEvent(int assigned_robot_id, QString location_code,
                            QString event_type, QString note);

private:
    Ui::Widget *ui;
    QSqlDatabase db;
};

#endif
