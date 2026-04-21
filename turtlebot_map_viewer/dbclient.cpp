#include "dbclient.h"

DbClient::DbClient(QObject *parent) : QObject(parent)
{
    db_ = QSqlDatabase::addDatabase("QMYSQL");     // MariaDB(MySQL 프로토콜)용 Qt 드라이버 설정
    db_.setHostName("10.10.16.32");                // DB 서버 IP 설정
    db_.setDatabaseName("turtlebot_db");           // 사용할 DB 이름 설정
    db_.setUserName("turtlebot");                  // DB 계정 설정
    db_.setPassword("1234");                       // DB 비밀번호 설정

    if (db_.open()) {
        qDebug() << "DB 연결 성공!";               // 초기 DB 연결 성공 로그
    } else {
        qDebug() << "DB 연결 실패:" << db_.lastError().text(); // 초기 DB 연결 실패 로그
    }
}

DbClient::~DbClient()
{
    db_.close();                                   // 종료 시 DB 연결 닫기
}

bool DbClient::isConnected()
{
    return db_.isOpen();                           // DB open 상태만 확인
}

void DbClient::insertVoiceCommand(
    int target_robot_id, QString raw_text,
    QString parsed_action, QString parsed_target,
    bool success)
{
    QSqlQuery query;                               // 음성 명령 처리 결과 INSERT용 쿼리 객체
    query.prepare(
        "INSERT INTO voice_command_log "
        "(target_robot_id, raw_text, parsed_action, parsed_target, success) "
        "VALUES (:robot_id, :raw_text, :parsed_action, :parsed_target, :success)"
    );
    query.bindValue(":robot_id",      target_robot_id);   // 대상 로봇 ID 바인딩
    query.bindValue(":raw_text",      raw_text);          // 원본 음성 텍스트 바인딩
    query.bindValue(":parsed_action", parsed_action);     // 파싱된 액션 바인딩
    query.bindValue(":parsed_target", parsed_target);     // 파싱된 대상 바인딩
    query.bindValue(":success",       success);           // 명령 성공 여부 바인딩

    if (query.exec())
        qDebug() << "voice_command_log INSERT 성공!";    // 음성명령 로그 저장 성공
    else
        qDebug() << "INSERT 실패:" << query.lastError().text(); // 음성명령 로그 저장 실패
}

void DbClient::insertPickingEvent(
    int assigned_robot_id, QString location_code,
    QString event_type, QString note)
{
    QSqlQuery query;                               // 피킹 이벤트 INSERT용 쿼리 객체
    query.prepare(
        "INSERT INTO picking_event "
        "(assigned_robot_id, location_code, event_type, note) "
        "VALUES (:robot_id, :location_code, :event_type, :note)"
    );
    query.bindValue(":robot_id",      assigned_robot_id); // 작업 할당 로봇 ID 바인딩
    query.bindValue(":location_code", location_code);     // 작업 위치 코드 바인딩
    query.bindValue(":event_type",    event_type);        // 이벤트 타입 바인딩
    query.bindValue(":note",          note);              // 비고/설명 바인딩

    if (query.exec())
        qDebug() << "picking_event INSERT 성공!";        // 피킹 이벤트 저장 성공
    else
        qDebug() << "INSERT 실패:" << query.lastError().text(); // 피킹 이벤트 저장 실패
}

void DbClient::insertRobotAction(
    int robot_id, int cmd_id,
    QString action_type,
    QString location_from, QString location_to,
    QString result)
{
    QSqlQuery query;                               // 로봇 행동 로그 INSERT용 쿼리 객체
    query.prepare(
        "INSERT INTO robot_action_log "
        "(robot_id, cmd_id, action_type, location_from, location_to, result) "
        "VALUES (:robot_id, :cmd_id, :action_type, :location_from, :location_to, :result)"
    );
    query.bindValue(":robot_id",      robot_id);         // 로봇 ID 바인딩
    query.bindValue(":cmd_id",        cmd_id);           // 명령 ID 바인딩
    query.bindValue(":action_type",   action_type);      // 행동 유형 바인딩
    query.bindValue(":location_from", location_from);    // 출발 위치 바인딩
    query.bindValue(":location_to",   location_to);      // 도착 위치 바인딩
    query.bindValue(":result",        result);           // 결과(success/fail 등) 바인딩

    if (query.exec())
        qDebug() << "robot_action_log INSERT 성공!";    // 로봇 행동 이력 저장 성공
    else
        qDebug() << "INSERT 실패:" << query.lastError().text(); // 로봇 행동 이력 저장 실패
}

void DbClient::insertAlert(
    int robot_id, QString severity,
    QString alert_type, QString message)
{
    QSqlQuery query;                               // 경고 로그 INSERT용 쿼리 객체
    query.prepare(
        "INSERT INTO alert_log "
        "(robot_id, severity, alert_type, message) "
        "VALUES (:robot_id, :severity, :alert_type, :message)"
    );
    query.bindValue(":robot_id",   robot_id);           // 로봇 ID 바인딩
    query.bindValue(":severity",   severity);           // 경고 심각도 바인딩
    query.bindValue(":alert_type", alert_type);         // 경고 유형 바인딩
    query.bindValue(":message",    message);            // 경고 메시지 바인딩

    if (query.exec())
        qDebug() << "alert_log INSERT 성공!";          // 경고 로그 저장 성공
    else
        qDebug() << "INSERT 실패:" << query.lastError().text(); // 경고 로그 저장 실패
}

// 로봇 상태 조회(robot_status_log에서 각 로봇의 최신 상태 가져오기)
QList<QMap<QString, QString>> DbClient::selectRobotStatus()
{
    QList<QMap<QString, QString>> result;         // 로봇 상태 조회 결과 리스트
    QSqlQuery query;

    query.exec(
        "SELECT r.robot_name, r.robot_type, "
        "s.status, s.pos_x, s.pos_y, "
        "s.orientation_z, s.orientation_w, s.battery_pct "
        "FROM robot r "
        "LEFT JOIN robot_status_log s ON r.robot_id = s.robot_id "
        "WHERE s.log_id IN ("
        "  SELECT MAX(log_id) FROM robot_status_log GROUP BY robot_id"
        ")"
    );                                            // 각 로봇의 최신 상태 1건씩 조회

    while (query.next()) {
        QMap<QString, QString> row;               // UI 전달용 key-value 형태로 변환
        row["robot_name"]    = query.value("robot_name").toString();   // 로봇 이름 저장
        row["robot_type"]    = query.value("robot_type").toString();   // 로봇 타입 저장
        row["status"]        = query.value("status").toString();       // 현재 상태 저장
        row["pos_x"]         = query.value("pos_x").toString();        // X 좌표 저장
        row["pos_y"]         = query.value("pos_y").toString();        // Y 좌표 저장
        row["battery_pct"]   = query.value("battery_pct").toString();  // 배터리 퍼센트 저장
        result.append(row);                                            // 결과 리스트에 추가
    }

    return result;                                   // 최신 로봇 상태 목록 반환
}

// 미해결 경고 조회(alert_log에서 resolved=false인 경고 가져오기)
QList<QMap<QString, QString>> DbClient::selectAlerts()
{
    QList<QMap<QString, QString>> result;         // 미해결 경고 조회 결과 리스트
    QSqlQuery query;

    query.exec(
        "SELECT a.alert_id, r.robot_name, "
        "a.severity, a.alert_type, a.message, a.triggered_at "
        "FROM alert_log a "
        "LEFT JOIN robot r ON a.robot_id = r.robot_id "
        "WHERE a.resolved = false "
        "ORDER BY a.triggered_at DESC"
    );                                            // 아직 해제되지 않은 경고를 최신순 조회

    while (query.next()) {
        QMap<QString, QString> row;               // UI 전달용 key-value 형태로 변환
        row["alert_id"]    = query.value("alert_id").toString();       // 경고 ID 저장
        row["robot_name"]  = query.value("robot_name").toString();     // 로봇 이름 저장
        row["severity"]    = query.value("severity").toString();       // 심각도 저장
        row["alert_type"]  = query.value("alert_type").toString();     // 경고 유형 저장
        row["message"]     = query.value("message").toString();        // 경고 메시지 저장
        row["triggered_at"] = query.value("triggered_at").toString();  // 발생 시각 저장
        result.append(row);                                            // 결과 리스트에 추가
    }

    return result;                                   // 미해결 경고 목록 반환
}

bool DbClient::ping()
{
    if (!db_.isOpen()) {
        return false;                                // DB가 닫혀 있으면 즉시 실패 처리
    }

    QSqlQuery query(db_);
    if (!query.exec("SELECT 1")) {
        qDebug() << "DB ping 실패:" << query.lastError().text(); // 실제 쿼리 실패 시 비정상 연결로 판단
        return false;
    }

    return true;                                     // 간단 질의 성공 시 연결 정상으로 판단
}
