#pragma once

#include <array>
#include <sensor_msgs/msg/battery_state.hpp>
#include <std_msgs/msg/string.hpp>
#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QFrame>

#include <QPushButton>
#include <QStackedWidget>
#include <QComboBox>

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rviz_common/render_panel.hpp>
#include <rviz_common/visualization_manager.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include "dbclient.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(rclcpp::Node::SharedPtr node, QWidget* parent = nullptr); // 메인 UI/ROS/DB 초기화
  ~MainWindow();                                                                // RViz 매니저 및 리소스 정리

protected:
  void showEvent(QShowEvent* event) override;                                   // 최초 show 시 RViz/Subscriber 초기화 트리거

private slots:
  void spinOnce();                                                              // ROS callback 1회 처리
  void toggleSidePage();                                                        // 상태/제어 페이지 전환
  void refreshDbStatus();                                                       // DB 연결 상태 주기 점검 후 배지 갱신

private:
  void setupUI();                                                               // 전체 Qt UI 레이아웃 구성
  void setupRViz();                                                             // RViz RenderPanel/뷰/툴 초기화
  void setupOdomSubscribers();                                                  // 4대 로봇 odom 구독 설정
  void addMapDisplay();                                                         // RViz Map display 추가
  void addTFDisplay();                                                          // RViz TF display 추가
  void addRobotModelDisplay();                                                  // RViz RobotModel display 추가
  void addLaserScanDisplay();                                                   // RViz LaserScan display 추가
  void setupRobotStatusSubscribers();                                           // 배터리/작업상태 토픽 구독 설정
  void resetView();                                                             // RViz 카메라 시점 초기화
  void zoomInView();                                                            // RViz 시점 확대
  void zoomOutView();                                                           // RViz 시점 축소
  void updateDbIndicator(bool connected);                                       // DB ON/OFF 배지 색상/문구 갱신

  QStackedWidget* side_stack_ = nullptr;                                        // 우측 패널 페이지 스택
  QPushButton*    page_toggle_btn_ = nullptr;                                   // 상태/제어 페이지 전환 버튼
  QPushButton* btn_reset_view_ = nullptr;                                       // 뷰 리셋 버튼
  QWidget* status_page_  = nullptr;                                             // 1페이지 상태 화면
  QWidget* control_page_ = nullptr;                                             // 2페이지 제어 화면
  QPushButton* btn_zoom_in_ = nullptr;                                          // 줌 인 버튼
  QPushButton* btn_zoom_out_ = nullptr;                                         // 줌 아웃 버튼

  QPushButton* btn_fetch_box_     = nullptr;                                    // 상자 가져오기 액션 버튼
  QPushButton* btn_move_goal_     = nullptr;                                    // 목표 위치 이동 버튼
  QPushButton* btn_set_goal_      = nullptr;                                    // 목표 위치 설정 버튼
  QPushButton* btn_select_robot_  = nullptr;                                    // 특정 로봇 선택 버튼

  rclcpp::Node::SharedPtr node_;                                                // 메인 ROS 노드 핸들

  std::shared_ptr<rviz_common::ros_integration::RosNodeAbstraction> ros_node_abs_; // RViz 내부 ROS 노드 래퍼
  rviz_common::RenderPanel*          render_panel_     = nullptr;               // RViz 렌더 패널
  rviz_common::VisualizationManager* manager_          = nullptr;               // RViz display/view 관리자
  bool                               rviz_initialized_ = false;                 // RViz 1회 초기화 여부

  std::array<QLabel*, 4> label_pos_x_;                                          // 로봇별 X 좌표 라벨
  std::array<QLabel*, 4> label_pos_y_;                                          // 로봇별 Y 좌표 라벨
  std::array<QLabel*, 4> label_linear_vel_;                                     // 로봇별 선속도 라벨
  std::array<QLabel*, 4> label_angular_vel_;                                    // 로봇별 각속도 라벨

  std::array<QLabel*, 4> label_battery_;                                        // 로봇별 배터리 라벨
  std::array<QLabel*, 4> label_task_;                                           // 로봇별 작업 상태 라벨

  DbClient* pDb = nullptr;                                                      // DB 접근 객체
  QLabel* label_db_status_ = nullptr;                                           // DB 연결 상태 배지
  QTimer* db_status_timer_ = nullptr;                                           // DB 상태 주기 점검 타이머

  std::array<rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr, 4> odom_subs_;             // 로봇별 odom 구독자
  std::array<rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr, 4> battery_subs_;   // 로봇별 배터리 구독자
  std::array<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr, 4> task_subs_;               // 로봇별 작업상태 구독자

  QTimer* spin_timer_;                                                          // ROS spin 주기 실행 타이머
};
