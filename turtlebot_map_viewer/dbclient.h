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
    explicit DbClient(QObject *parent = nullptr);   // DB 연결 생성 및 초기 접속 수행
    ~DbClient();                                    // DB 연결 종료

    bool isConnected();                             // QSqlDatabase open 상태 확인
    bool ping();                                    // 실제 쿼리 가능 여부로 DB 생존 확인

    void insertVoiceCommand(int target_robot_id, QString raw_text,
                            QString parsed_action, QString parsed_target,
                            bool success);          // 음성명령 처리 로그 저장

    void insertPickingEvent(int assigned_robot_id, QString location_code,
                            QString event_type, QString note); // 피킹 작업 이벤트 로그 저장

    void insertRobotAction(int robot_id, int cmd_id,
                           QString action_type,
                           QString location_from, QString location_to,
                           QString result);         // 로봇 행동/이동 이력 저장

    void insertAlert(int robot_id, QString severity,
                     QString alert_type, QString message); // 경고/이상 이벤트 로그 저장

    QList<QMap<QString, QString>> selectRobotStatus(); // 각 로봇 최신 상태 조회
    QList<QMap<QString, QString>> selectAlerts();      // 미해결 경고 목록 조회

private:
    QSqlDatabase db_;                                // DB 커넥션 핸들
};
