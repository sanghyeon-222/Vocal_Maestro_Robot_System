#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // MariaDB 연결
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("turtlebot_db");
    db.setUserName("turtlebot");
    db.setPassword("1234");

    if (db.open()) {
        qDebug() << "DB 연결 성공!";
    } else {
        qDebug() << "DB 연결 실패:" << db.lastError().text();
    }
}

Widget::~Widget()
{
    db.close();
    delete ui;
}

// voice_command_log INSERT
void Widget::insertVoiceCommand(
    int target_robot_id, QString raw_text,
    QString parsed_action, QString parsed_target,
    bool success)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO voice_command_log "
        "(target_robot_id, raw_text, parsed_action, parsed_target, success) "
        "VALUES (:robot_id, :raw_text, :parsed_action, :parsed_target, :success)"
    );
    query.bindValue(":robot_id",      target_robot_id);
    query.bindValue(":raw_text",      raw_text);
    query.bindValue(":parsed_action", parsed_action);
    query.bindValue(":parsed_target", parsed_target);
    query.bindValue(":success",       success);

    if (query.exec())
        qDebug() << "voice_command_log INSERT 성공!";
    else
        qDebug() << "INSERT 실패:" << query.lastError().text();
}

// picking_event INSERT
void Widget::insertPickingEvent(
    int assigned_robot_id, QString location_code,
    QString event_type, QString note)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO picking_event "
        "(assigned_robot_id, location_code, event_type, note) "
        "VALUES (:robot_id, :location_code, :event_type, :note)"
    );
    query.bindValue(":robot_id",      assigned_robot_id);
    query.bindValue(":location_code", location_code);
    query.bindValue(":event_type",    event_type);
    query.bindValue(":note",          note);

    if (query.exec())
        qDebug() << "picking_event INSERT 성공!";
    else
        qDebug() << "INSERT 실패:" << query.lastError().text();
}
