#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QFrame>

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rviz_common/render_panel.hpp>
#include <rviz_common/visualization_manager.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>

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

private:
  void setupUI();
  void setupRViz();
  void setupOdomSubscriber();
  void addMapDisplay();
  void addTFDisplay();
  void addRobotModelDisplay();
  void addLaserScanDisplay();

  rclcpp::Node::SharedPtr node_;

  std::shared_ptr<rviz_common::ros_integration::RosNodeAbstraction> ros_node_abs_;
  rviz_common::RenderPanel*          render_panel_     = nullptr;
  rviz_common::VisualizationManager* manager_          = nullptr;
  bool                               rviz_initialized_ = false;

  QLabel* label_pos_x_;
  QLabel* label_pos_y_;
  QLabel* label_linear_vel_;
  QLabel* label_angular_vel_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  QTimer* spin_timer_;
};
