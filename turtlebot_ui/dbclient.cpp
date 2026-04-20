#include "dbclient.h"

DbClient::DbClient(QObject *parent) : QObject(parent)
{
    db_ = QSqlDatabase::addDatabase("QMYSQL");
    db_.setHostName("10.10.16.32");
    db_.setDatabaseName("turtlebot_db");
    db_.setUserName("turtlebot");
    db_.setPassword("1234");

    if (db_.open()) {
        qDebug() << "DB 연결 성공!";
    } else {
        qDebug() << "DB 연결 실패:" << db_.lastError().text();
    }
}

DbClient::~DbClient()
{
    db_.close();
}

bool DbClient::isConnected()
{
    return db_.isOpen();
}

void DbClient::insertVoiceCommand(
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

void DbClient::insertPickingEvent(
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

void DbClient::insertRobotAction(
    int robot_id, int cmd_id,
    QString action_type,
    QString location_from, QString location_to,
    QString result)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO robot_action_log "
        "(robot_id, cmd_id, action_type, location_from, location_to, result) "
        "VALUES (:robot_id, :cmd_id, :action_type, :location_from, :location_to, :result)"
    );
    query.bindValue(":robot_id",      robot_id);
    query.bindValue(":cmd_id",        cmd_id);
    query.bindValue(":action_type",   action_type);
    query.bindValue(":location_from", location_from);
    query.bindValue(":location_to",   location_to);
    query.bindValue(":result",        result);

    if (query.exec())
        qDebug() << "robot_action_log INSERT 성공!";
    else
        qDebug() << "INSERT 실패:" << query.lastError().text();
}

void DbClient::insertAlert(
    int robot_id, QString severity,
    QString alert_type, QString message)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO alert_log "
        "(robot_id, severity, alert_type, message) "
        "VALUES (:robot_id, :severity, :alert_type, :message)"
    );
    query.bindValue(":robot_id",   robot_id);
    query.bindValue(":severity",   severity);
    query.bindValue(":alert_type", alert_type);
    query.bindValue(":message",    message);

    if (query.exec())
        qDebug() << "alert_log INSERT 성공!";
    else
        qDebug() << "INSERT 실패:" << query.lastError().text();
}
