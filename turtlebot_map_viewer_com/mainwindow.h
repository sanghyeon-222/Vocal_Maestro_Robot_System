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
  explicit MainWindow(rclcpp::Node::SharedPtr node, QWidget* parent = nullptr);
  ~MainWindow();

protected:
  void showEvent(QShowEvent* event) override;

private slots:
  void spinOnce();
  void toggleSidePage();

private:
  void setupUI();
  void setupRViz();
  void setupOdomSubscribers();
  void addMapDisplay();
  void addTFDisplay();
  void addRobotModelDisplay();
  void addLaserScanDisplay();
  void setupRobotStatusSubscribers();
  void resetView();
  QStackedWidget* side_stack_ = nullptr;
  QPushButton*    page_toggle_btn_ = nullptr;
  QPushButton* btn_reset_view_ = nullptr;
  QWidget* status_page_  = nullptr;
  QWidget* control_page_ = nullptr;

  QPushButton* btn_fetch_box_     = nullptr;
  QPushButton* btn_move_goal_     = nullptr;
  QPushButton* btn_set_goal_      = nullptr;
  QPushButton* btn_select_robot_  = nullptr;

  DbClient *pDb;


  rclcpp::Node::SharedPtr node_;

  std::shared_ptr<rviz_common::ros_integration::RosNodeAbstraction> ros_node_abs_;
  rviz_common::RenderPanel*          render_panel_     = nullptr;
  rviz_common::VisualizationManager* manager_          = nullptr;
  bool                               rviz_initialized_ = false;

  std::array<QLabel*, 4> label_pos_x_;
  std::array<QLabel*, 4> label_pos_y_;
  std::array<QLabel*, 4> label_linear_vel_;
  std::array<QLabel*, 4> label_angular_vel_;

  std::array<QLabel*, 4> label_battery_;
  std::array<QLabel*, 4> label_task_;

  //rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  std::array<rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr, 4> odom_subs_;
  std::array<rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr, 4> battery_subs_;
  std::array<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr, 4> task_subs_;

  QTimer* spin_timer_;
};
