#include "mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QShowEvent>
#include <QFrame>

#include <rviz_common/view_manager.hpp>
#include <rviz_common/view_controller.hpp>
#include <rviz_common/tool_manager.hpp>
#include <rviz_common/tool.hpp>

MainWindow::MainWindow(rclcpp::Node::SharedPtr node, QWidget* parent)
  : QMainWindow(parent), node_(node)
{
  setWindowTitle("TurtleBot3 Map Viewer");

  pDb = new DbClient(this);

  setupUI();

  spin_timer_ = new QTimer(this);
  connect(spin_timer_, &QTimer::timeout, this, &MainWindow::spinOnce);
}

void MainWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);

  if (!rviz_initialized_) {
    rviz_initialized_ = true;
    QTimer::singleShot(100, this, [this]() {
      setupRViz();
      setupOdomSubscribers();
      setupRobotStatusSubscribers();
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

void MainWindow::setupOdomSubscribers()
{
  const std::array<std::string, 4> odom_topics = {
    "/odom",      // Master Robot
    "/tb2/odom",  // Robot 2
    "/tb3/odom",  // Robot 3
    "/tb4/odom"   // Robot 4
  };

  for (int i = 0; i < 4; ++i) {
    odom_subs_[i] = node_->create_subscription<nav_msgs::msg::Odometry>(
      odom_topics[i], 10,
      [this, i](const nav_msgs::msg::Odometry::SharedPtr msg) {
        const double x       = msg->pose.pose.position.x;
        const double y       = msg->pose.pose.position.y;
        const double linear  = msg->twist.twist.linear.x;
        const double angular = msg->twist.twist.angular.z;

        QMetaObject::invokeMethod(this, [this, i, x, y, linear, angular]() {
          label_pos_x_[i]->setText(QString("X : %1 m").arg(x, 0, 'f', 3));
          label_pos_y_[i]->setText(QString("Y : %1 m").arg(y, 0, 'f', 3));
          label_linear_vel_[i]->setText(QString("선속도 : %1 m/s").arg(linear, 0, 'f', 3));
          label_angular_vel_[i]->setText(QString("각속도 : %1 rad/s").arg(angular, 0, 'f', 3));
        }, Qt::QueuedConnection);
      }
    );
  }
}

void MainWindow::setupRobotStatusSubscribers()
{
  const std::array<std::string, 4> robot_names = {"tb1", "tb2", "tb3", "tb4"};

  for (int i = 0; i < 4; ++i) {
    const auto battery_topic = "/" + robot_names[i] + "/battery_state";
    const auto task_topic    = "/" + robot_names[i] + "/task_status";

    battery_subs_[i] = node_->create_subscription<sensor_msgs::msg::BatteryState>(
      battery_topic, 10,
      [this, i](const sensor_msgs::msg::BatteryState::SharedPtr msg) {
        double percent = msg->percentage;

        if (percent >= 0.0 && percent <= 1.0) {
          percent *= 100.0;
        }

        if (percent <= 20.0) {
                    pDb->insertAlert(
                        i + 1,
                        "warning",
                        "low_battery",
                        QString("배터리 %1% 이하").arg(percent, 0, 'f', 1)
                    );
                }

                QMetaObject::invokeMethod(this, [this, i, percent]() {
                    label_battery_[i]->setText(QString("배터리 : %1 %").arg(percent, 0, 'f', 1));
                }, Qt::QueuedConnection);

        QMetaObject::invokeMethod(this, [this, i, percent]() {
          label_battery_[i]->setText(QString("배터리 : %1 %").arg(percent, 0, 'f', 1));
        }, Qt::QueuedConnection);
      }
    );

    task_subs_[i] = node_->create_subscription<std_msgs::msg::String>(
      task_topic, 10,
      [this, i](const std_msgs::msg::String::SharedPtr msg) {
        const QString task = QString::fromStdString(msg->data);

        QMetaObject::invokeMethod(this, [this, i, task]() {
          label_task_[i]->setText(QString("작업 : %1").arg(task));
        }, Qt::QueuedConnection);
      }
    );
  }
}

void MainWindow::setupUI()
{
  auto* central  = new QWidget(this);
  auto* h_layout = new QHBoxLayout(central);
  h_layout->setContentsMargins(0, 0, 0, 0);
  h_layout->setSpacing(0);

  // 오른쪽 정보/제어 패널
  auto* side_panel = new QWidget(central);
  side_panel->setFixedWidth(220);
  side_panel->setStyleSheet("background-color: #1e1e2e;");

  auto* side_main_layout = new QVBoxLayout(side_panel);
  side_main_layout->setContentsMargins(10, 10, 10, 10);
  side_main_layout->setSpacing(6);

  auto makeLine = [&]() {
    auto* line = new QFrame(side_panel);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #313244; max-height: 1px; border: none;");
    return line;
  };

  auto makeSection = [&](const QString& text) {
    auto* lbl = new QLabel(text, side_panel);
    lbl->setStyleSheet("color: #89b4fa; font-size: 10px; font-weight: bold;");
    return lbl;
  };

  auto makeValue = [&](const QString& text) {
    auto* lbl = new QLabel(text, side_panel);
    lbl->setStyleSheet("color: #a6e3a1; font-size: 10px; font-weight: bold; padding-left: 6px;");
    return lbl;
  };

  auto makeActionButton = [&](const QString& text) {
    auto* btn = new QPushButton(text, side_panel);
    btn->setMinimumHeight(34);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
      "QPushButton {"
      "  background-color: #313244;"
      "  color: #cdd6f4;"
      "  border: 1px solid #45475a;"
      "  border-radius: 8px;"
      "  font-size: 11px;"
      "  font-weight: bold;"
      "  padding: 6px;"
      "}"
      "QPushButton:hover {"
      "  background-color: #45475a;"
      "}"
      "QPushButton:pressed {"
      "  background-color: #585b70;"
      "}"
    );
    return btn;
  };

  // -----------------------------
  // 상단 헤더 + 우상단 페이지 전환 버튼
  // -----------------------------
  auto* header_layout = new QHBoxLayout();
  header_layout->setContentsMargins(0, 0, 0, 0);
  header_layout->setSpacing(6);

  auto* title_box = new QVBoxLayout();
  title_box->setContentsMargins(0, 0, 0, 0);
  title_box->setSpacing(1);

  auto* title = new QLabel("🤖  TurtleBot3", side_panel);
  title->setStyleSheet("color: #cdd6f4; font-size: 13px; font-weight: bold;");
  title_box->addWidget(title);

  auto* subtitle = new QLabel("Map Viewer", side_panel);
  subtitle->setStyleSheet("color: #6c7086; font-size: 10px;");
  title_box->addWidget(subtitle);

  page_toggle_btn_ = new QPushButton("제어", side_panel);
  page_toggle_btn_->setFixedSize(54, 26);
  page_toggle_btn_->setCursor(Qt::PointingHandCursor);
  page_toggle_btn_->setStyleSheet(
    "QPushButton {"
    "  background-color: #313244;"
    "  color: #cdd6f4;"
    "  border: 1px solid #45475a;"
    "  border-radius: 8px;"
    "  font-size: 10px;"
    "  font-weight: bold;"
    "}"
    "QPushButton:hover {"
    "  background-color: #45475a;"
    "}"
  );

  connect(page_toggle_btn_, &QPushButton::clicked, this, &MainWindow::toggleSidePage);

  header_layout->addLayout(title_box, 1);
  header_layout->addWidget(page_toggle_btn_, 0, Qt::AlignTop);

  side_main_layout->addLayout(header_layout);
  side_main_layout->addWidget(makeLine());

  // -----------------------------
  // 페이지 스택
  // -----------------------------
  side_stack_ = new QStackedWidget(side_panel);

  // =========================================================
  // 1페이지 : 상태 페이지
  // =========================================================
  status_page_ = new QWidget(side_panel);
  auto* status_layout = new QVBoxLayout(status_page_);
  status_layout->setContentsMargins(0, 6, 0, 0);
  status_layout->setSpacing(6);

  status_layout->addWidget(makeSection("🤖  로봇 상태"));

  for (int i = 0; i < 4; ++i) {
    QString robot_name;
    if (i == 0) {
      robot_name = "Master Robot";
    } else {
      robot_name = QString("Robot %1").arg(i + 1);
    }

    auto* robot_title = new QLabel(robot_name, status_page_);
    robot_title->setStyleSheet("color: #f9e2af; font-size: 10px; font-weight: bold; padding-top: 2px;");
    status_layout->addWidget(robot_title);

    label_pos_x_[i]       = makeValue("X : -- m");
    label_pos_y_[i]       = makeValue("Y : -- m");
    label_linear_vel_[i]  = makeValue("선속도 : -- m/s");
    label_angular_vel_[i] = makeValue("각속도 : -- rad/s");
    label_battery_[i]     = makeValue("배터리 : -- %");
    label_task_[i]        = makeValue("작업 : 대기 중");

    label_task_[i]->setStyleSheet("color: #cdd6f4; font-size: 10px; font-weight: bold; padding-left: 6px;");

    status_layout->addWidget(label_pos_x_[i]);
    status_layout->addWidget(label_pos_y_[i]);
    status_layout->addWidget(label_linear_vel_[i]);
    status_layout->addWidget(label_angular_vel_[i]);
    status_layout->addWidget(label_battery_[i]);
    status_layout->addWidget(label_task_[i]);

    if (i != 3) {
      status_layout->addSpacing(2);
      status_layout->addWidget(makeLine());
      status_layout->addSpacing(2);
    }
  }

  status_layout->addStretch();

  // =========================================================
  // 2페이지 : 제어 페이지
  // =========================================================
  control_page_ = new QWidget(side_panel);
  auto* control_layout = new QVBoxLayout(control_page_);
  control_layout->setContentsMargins(0, 6, 0, 0);
  control_layout->setSpacing(8);

  control_layout->addWidget(makeSection("🕹️  작업 / 제어"));

  btn_fetch_box_ = makeActionButton("상자 가져와");
  btn_move_goal_ = makeActionButton("목표위치 이동");
  btn_set_goal_  = makeActionButton("목표위치 설정");
  btn_select_robot_ = makeActionButton("특정로봇 선택");
  btn_reset_view_ = makeActionButton("뷰 리셋");

  control_layout->addWidget(btn_fetch_box_);
  control_layout->addWidget(btn_move_goal_);
  control_layout->addWidget(btn_set_goal_);
  control_layout->addWidget(btn_select_robot_);
  control_layout->addWidget(btn_reset_view_);

  control_layout->addSpacing(4);
  control_layout->addWidget(makeLine());
  control_layout->addSpacing(4);

  auto* guide = new QLabel("추후 여기에 추가 버튼,\n입력창, 선택 옵션을 확장할 수 있습니다.", control_page_);
  guide->setStyleSheet("color: #7f849c; font-size: 10px; line-height: 1.3;");
  guide->setWordWrap(true);
  control_layout->addWidget(guide);

  control_layout->addStretch();

  side_stack_->addWidget(status_page_);   // index 0
  side_stack_->addWidget(control_page_);  // index 1

  side_main_layout->addWidget(side_stack_, 1);

  auto* version = new QLabel("ROS2 Humble  |  Qt5", side_panel);
  version->setStyleSheet("color: #45475a; font-size: 9px;");
  version->setAlignment(Qt::AlignCenter);
  side_main_layout->addWidget(version);

  // 왼쪽 RViz 렌더링
  render_panel_ = new rviz_common::RenderPanel(central);

  h_layout->addWidget(render_panel_, 1);
  h_layout->addWidget(side_panel);

  setCentralWidget(central);

  // 버튼 클릭 임시 연결
  connect(btn_fetch_box_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 상자 가져오는 중");
    pDb->insertVoiceCommand(1, "상자 가져와", "fetch_box", "slave_1", true);
    pDb->insertPickingEvent(1, "", "pick_start", "상자 가져오기 시작");
  });

  connect(btn_move_goal_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 목표 위치 이동 중");
    pDb->insertVoiceCommand(1, "목표위치 이동", "move", "goal", true);
    pDb->insertRobotAction(1, -1, "move_to", "", "goal", "success");
  });

  connect(btn_set_goal_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 목표 위치 설정 대기");
    pDb->insertVoiceCommand(1, "목표위치 설정", "move", "set_goal", true);
  });

  connect(btn_select_robot_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 특정 로봇 선택 모드");
    pDb->insertVoiceCommand(1, "특정 로봇 선택", "custom", "select", true);
  });

  connect(btn_reset_view_, &QPushButton::clicked, this, &MainWindow::resetView);
}

void MainWindow::setupRViz()
{
  ros_node_abs_ = std::make_shared<rviz_common::ros_integration::RosNodeAbstraction>("rviz_node");
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr weak_node = ros_node_abs_;

  manager_ = new rviz_common::VisualizationManager(
    render_panel_, weak_node, nullptr,
    ros_node_abs_->get_raw_node()->get_clock());

  render_panel_->setFocusPolicy(Qt::StrongFocus);
  render_panel_->setMouseTracking(true);

  render_panel_->initialize(manager_);
  manager_->initialize();
  manager_->startUpdate();

  manager_->setFixedFrame("map");

  // 3D Orbit View 세팅
  auto* view_manager = manager_->getViewManager();
  if (view_manager) {
    view_manager->setCurrentViewControllerType("rviz_default_plugins/Orbit");

    auto* view = view_manager->getCurrent();
    if (view) {
      if (auto* target = view->subProp("Target Frame")) {
        target->setValue("map");
      }

      if (auto* focal_shape = view->subProp("Focal Shape Size")) {
        focal_shape->setValue(0.05);
      }

      if (auto* distance = view->subProp("Distance")) {
        distance->setValue(18.0);
      }

      if (auto* pitch = view->subProp("Pitch")) {
        pitch->setValue(0.95);
      }

      if (auto* yaw = view->subProp("Yaw")) {
        yaw->setValue(0.85);
      }

      auto* focal = view->subProp("Focal Point");
      if (focal) {
        if (auto* x = focal->subProp("X")) {
          x->setValue(0.0);
        }
        if (auto* y = focal->subProp("Y")) {
          y->setValue(0.0);
        }
        if (auto* z = focal->subProp("Z")) {
          z->setValue(0.0);
        }
      }
    }
  }

  addMapDisplay();
  addTFDisplay();
  addRobotModelDisplay();
  addLaserScanDisplay();

  // 기본 마우스 도구를 카메라 이동으로 설정
  auto* tool_manager = manager_->getToolManager();
  if (tool_manager) {
    auto* move_tool = tool_manager->addTool("rviz_default_plugins/MoveCamera");
    if (move_tool) {
      tool_manager->setCurrentTool(move_tool);
    }
  }
}

void MainWindow::addMapDisplay()
{
  auto* map = manager_->createDisplay("rviz_default_plugins/Map", "Map", true);
  map->subProp("Topic")->setValue("/map");
  map->subProp("Alpha")->setValue(0.95);
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
  scan->subProp("Size (m)")->setValue(0.03);
}

void MainWindow::spinOnce()
{
  rclcpp::spin_some(node_);
}

void MainWindow::toggleSidePage()
{
  if (!side_stack_) {
    return;
  }

  if (side_stack_->currentIndex() == 0) {
    side_stack_->setCurrentIndex(1);   // 제어 페이지
    page_toggle_btn_->setText("상태");
  } else {
    side_stack_->setCurrentIndex(0);   // 상태 페이지
    page_toggle_btn_->setText("제어");
  }
}

void MainWindow::resetView()
{
  if (!manager_) {
    return;
  }

  auto* view_manager = manager_->getViewManager();
  if (!view_manager) {
    return;
  }

  view_manager->setCurrentViewControllerType("rviz_default_plugins/Orbit");

  auto* view = view_manager->getCurrent();
  if (!view) {
    return;
  }

  if (auto* target = view->subProp("Target Frame")) {
    target->setValue("map");
  }

  if (auto* focal_shape = view->subProp("Focal Shape Size")) {
    focal_shape->setValue(0.05);
  }

  if (auto* distance = view->subProp("Distance")) {
    distance->setValue(18.0);
  }

  if (auto* pitch = view->subProp("Pitch")) {
    pitch->setValue(0.95);
  }

  if (auto* yaw = view->subProp("Yaw")) {
    yaw->setValue(0.85);
  }

  auto* focal = view->subProp("Focal Point");
  if (focal) {
    if (auto* x = focal->subProp("X")) {
      x->setValue(0.0);
    }
    if (auto* y = focal->subProp("Y")) {
      y->setValue(0.0);
    }
    if (auto* z = focal->subProp("Z")) {
      z->setValue(0.0);
    }
  }
}
