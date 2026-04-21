#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>

class DbClient : public QObject
{
    Q_OBJECT

public:
    explicit DbClient(QObject *parent = nullptr);
    ~DbClient();

    bool isConnected();

    void insertVoiceCommand(int target_robot_id, QString raw_text,
                            QString parsed_action, QString parsed_target,
                            bool success);

    void insertPickingEvent(int assigned_robot_id, QString location_code,
                            QString event_type, QString note);

    void insertRobotAction(int robot_id, int cmd_id,
                           QString action_type,
                           QString location_from, QString location_to,
                           QString result);

    void insertAlert(int robot_id, QString severity,
                     QString alert_type, QString message);

    // 로봇 상태 조회 (robot_status_log SELECT)
    QList<QMap<QString, QString>> selectRobotStatus();

    // 미해결 경고 조회 (alert_log SELECT)
    QList<QMap<QString, QString>> selectAlerts();

private:
    QSqlDatabase db_;
};
