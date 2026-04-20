#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QShowEvent>
#include <QFrame>

MainWindow::MainWindow(rclcpp::Node::SharedPtr node, QWidget* parent)
  : QMainWindow(parent), node_(node)
{
  setWindowTitle("TurtleBot3 Map Viewer");
  resize(1280, 800);

  setupUI();

  spin_timer_ = new QTimer(this);
  connect(spin_timer_, &QTimer::timeout, this, &MainWindow::spinOnce);
}

void MainWindow::setupUI()
{
  auto* central  = new QWidget(this);
  auto* h_layout = new QHBoxLayout(central);
  h_layout->setContentsMargins(0, 0, 0, 0);
  h_layout->setSpacing(0);

  // 왼쪽 정보 패널
  auto* side_panel = new QWidget(central);
  side_panel->setFixedWidth(220);
  side_panel->setStyleSheet("background-color: #1e1e2e;");

  auto* v_layout = new QVBoxLayout(side_panel);
  v_layout->setContentsMargins(16, 20, 16, 20);
  v_layout->setSpacing(8);

  auto* title = new QLabel("🤖  TurtleBot3", side_panel);
  title->setStyleSheet("color: #cdd6f4; font-size: 15px; font-weight: bold;");
  v_layout->addWidget(title);

  auto* subtitle = new QLabel("Map Viewer", side_panel);
  subtitle->setStyleSheet("color: #6c7086; font-size: 11px;");
  v_layout->addWidget(subtitle);

  auto makeLine = [&]() {
    auto* line = new QFrame(side_panel);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #313244; max-height: 1px; border: none;");
    return line;
  };

  v_layout->addSpacing(8);
  v_layout->addWidget(makeLine());
  v_layout->addSpacing(8);

  auto makeSection = [&](const QString& text) {
    auto* lbl = new QLabel(text, side_panel);
    lbl->setStyleSheet("color: #89b4fa; font-size: 11px; font-weight: bold;");
    return lbl;
  };

  auto makeValue = [&](const QString& text) {
    auto* lbl = new QLabel(text, side_panel);
    lbl->setStyleSheet("color: #a6e3a1; font-size: 13px; font-weight: bold; padding-left: 8px;");
    return lbl;
  };

  v_layout->addWidget(makeSection("📍  위치"));
  label_pos_x_ = makeValue("X :  -- m");
  label_pos_y_ = makeValue("Y :  -- m");
  v_layout->addWidget(label_pos_x_);
  v_layout->addWidget(label_pos_y_);

  v_layout->addSpacing(8);
  v_layout->addWidget(makeLine());
  v_layout->addSpacing(8);

  v_layout->addWidget(makeSection("🚀  속도"));
  label_linear_vel_  = makeValue("선속도 :  -- m/s");
  label_angular_vel_ = makeValue("각속도 :  -- rad/s");
  v_layout->addWidget(label_linear_vel_);
  v_layout->addWidget(label_angular_vel_);

  v_layout->addStretch();

  auto* version = new QLabel("ROS2 Humble  |  Qt5", side_panel);
  version->setStyleSheet("color: #45475a; font-size: 10px;");
  version->setAlignment(Qt::AlignCenter);
  v_layout->addWidget(version);

  // 오른쪽 RViz 렌더링
  render_panel_ = new rviz_common::RenderPanel(central);

  h_layout->addWidget(side_panel);
  h_layout->addWidget(render_panel_, 1);

  setCentralWidget(central);
}

void MainWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);

  if (!rviz_initialized_) {
    rviz_initialized_ = true;
    QTimer::singleShot(100, this, [this]() {
      setupRViz();
      setupOdomSubscriber();
      spin_timer_->start(30);
    });
  }
}

MainWindow::~MainWindow()
{
  if (manager_) {
    manager_->stopUpdate();
    delete manager_;
  }
}

void MainWindow::setupOdomSubscriber()
{
  odom_sub_ = node_->create_subscription<nav_msgs::msg::Odometry>(
    "/odom", 10,
    [this](const nav_msgs::msg::Odometry::SharedPtr msg) {
      double x       = msg->pose.pose.position.x;
      double y       = msg->pose.pose.position.y;
      double linear  = msg->twist.twist.linear.x;
      double angular = msg->twist.twist.angular.z;

      QMetaObject::invokeMethod(this, [this, x, y, linear, angular]() {
        label_pos_x_->setText(QString("X :  %1 m").arg(x, 0, 'f', 3));
        label_pos_y_->setText(QString("Y :  %1 m").arg(y, 0, 'f', 3));
        label_linear_vel_->setText(QString("선속도 :  %1 m/s").arg(linear, 0, 'f', 3));
        label_angular_vel_->setText(QString("각속도 :  %1 rad/s").arg(angular, 0, 'f', 3));
      }, Qt::QueuedConnection);
    }
  );
}

void MainWindow::setupRViz()
{
  ros_node_abs_ = std::make_shared<rviz_common::ros_integration::RosNodeAbstraction>("rviz_node");
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr weak_node = ros_node_abs_;

  manager_ = new rviz_common::VisualizationManager(
    render_panel_, weak_node, nullptr,
    ros_node_abs_->get_raw_node()->get_clock());

  render_panel_->initialize(manager_);
  manager_->initialize();
  manager_->startUpdate();

  manager_->setFixedFrame("map");

  addMapDisplay();
  addTFDisplay();
  addRobotModelDisplay();
  addLaserScanDisplay();
}

void MainWindow::addMapDisplay()
{
  auto* map = manager_->createDisplay("rviz_default_plugins/Map", "Map", true);
  map->subProp("Topic")->setValue("/map");
}

void MainWindow::addTFDisplay()
{
  auto* tf = manager_->createDisplay("rviz_default_plugins/TF", "TF", true);
  tf->subProp("Show Axes")->setValue(true);
  tf->subProp("Show Names")->setValue(false);
}

void MainWindow::addRobotModelDisplay()
{
  auto* robot = manager_->createDisplay("rviz_default_plugins/RobotModel", "Robot Model", true);
  robot->subProp("Description Topic")->setValue("/robot_description");
}

void MainWindow::addLaserScanDisplay()
{
  auto* scan = manager_->createDisplay("rviz_default_plugins/LaserScan", "LaserScan", true);
  scan->subProp("Topic")->setValue("/scan");
  scan->subProp("Size (m)")->setValue(0.05);
}

void MainWindow::spinOnce()
{
  rclcpp::spin_some(node_);
}
