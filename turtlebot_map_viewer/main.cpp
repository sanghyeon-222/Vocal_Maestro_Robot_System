#include <QApplication>
#include <rclcpp/rclcpp.hpp>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
  rclcpp::init(argc, argv);
  QApplication app(argc, argv);

  auto node = rclcpp::Node::make_shared("turtlebot_map_viewer_node");

  MainWindow window(node);
  window.show();

  int result = app.exec();
  rclcpp::shutdown();
  return result;
}
