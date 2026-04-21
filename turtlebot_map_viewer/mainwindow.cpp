#include "mainwindow.h"
#include <QScrollArea>
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
  setWindowTitle("TurtleBot3 Map Viewer");                    // 메인 윈도우 제목 설정
  // resize(1280, 800);

  setupUI();                                                 // Qt UI 전체 레이아웃 구성

  spin_timer_ = new QTimer(this);                            // ROS spin 주기 처리용 타이머 생성
  connect(spin_timer_, &QTimer::timeout, this, &MainWindow::spinOnce);

  pDb = new DbClient(this);                                  // DB 클라이언트 생성

  db_status_timer_ = new QTimer(this);                       // DB 연결 상태 주기 점검 타이머 생성
  connect(db_status_timer_, &QTimer::timeout, this, &MainWindow::refreshDbStatus);
  db_status_timer_->start(3000);                             // 3초 주기로 DB 상태 확인

  refreshDbStatus();                                         // 시작 직후 DB 상태 배지 1회 갱신
}

void MainWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);                             // 기본 showEvent 처리

  if (!rviz_initialized_) {
    rviz_initialized_ = true;                                // RViz 초기화 1회만 수행하도록 플래그 설정
    QTimer::singleShot(100, this, [this]() {
      setupRViz();                                           // RViz 패널/뷰/디스플레이 초기화
      setupOdomSubscribers();                                // 로봇 odom 구독 설정
      setupRobotStatusSubscribers();                         // 배터리/작업상태 구독 설정
      spin_timer_->start(30);                                // ROS spin 타이머 시작
    });
  }
}

MainWindow::~MainWindow()
{
  if (manager_) {
    manager_->stopUpdate();                                  // RViz 업데이트 중지
    delete manager_;                                         // RViz 매니저 해제
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
        const double x       = msg->pose.pose.position.x;    // 수신한 odom X 좌표 추출
        const double y       = msg->pose.pose.position.y;    // 수신한 odom Y 좌표 추출
        const double linear  = msg->twist.twist.linear.x;    // 수신한 선속도 추출
        const double angular = msg->twist.twist.angular.z;   // 수신한 각속도 추출

        QMetaObject::invokeMethod(this, [this, i, x, y, linear, angular]() {
          label_pos_x_[i]->setText(QString("X : %1 m").arg(x, 0, 'f', 3));              // 로봇별 X 좌표 라벨 갱신
          label_pos_y_[i]->setText(QString("Y : %1 m").arg(y, 0, 'f', 3));              // 로봇별 Y 좌표 라벨 갱신
          label_linear_vel_[i]->setText(QString("선속도 : %1 m/s").arg(linear, 0, 'f', 3));  // 로봇별 선속도 라벨 갱신
          label_angular_vel_[i]->setText(QString("각속도 : %1 rad/s").arg(angular, 0, 'f', 3)); // 로봇별 각속도 라벨 갱신
        }, Qt::QueuedConnection);
      }
    );
  }
}

void MainWindow::setupRobotStatusSubscribers()
{
  const std::array<std::string, 4> robot_names = {"tb1", "tb2", "tb3", "tb4"};

  for (int i = 0; i < 4; ++i) {
    const auto battery_topic = "/" + robot_names[i] + "/battery_state";   // 로봇별 배터리 토픽 생성
    const auto task_topic    = "/" + robot_names[i] + "/task_status";     // 로봇별 작업상태 토픽 생성

    battery_subs_[i] = node_->create_subscription<sensor_msgs::msg::BatteryState>(
      battery_topic, 10,
      [this, i](const sensor_msgs::msg::BatteryState::SharedPtr msg) {
        double percent = msg->percentage;                  // 배터리 비율 수신

        if (percent >= 0.0 && percent <= 1.0) {
          percent *= 100.0;                                // 0~1 비율이면 % 값으로 환산
        }

        QMetaObject::invokeMethod(this, [this, i, percent]() {
          label_battery_[i]->setText(QString("배터리 : %1 %").arg(percent, 0, 'f', 1)); // 로봇별 배터리 라벨 갱신
        }, Qt::QueuedConnection);
      }
    );

    task_subs_[i] = node_->create_subscription<std_msgs::msg::String>(
      task_topic, 10,
      [this, i](const std_msgs::msg::String::SharedPtr msg) {
        const QString task = QString::fromStdString(msg->data);            // 수신한 작업 문자열 변환

        QMetaObject::invokeMethod(this, [this, i, task]() {
          label_task_[i]->setText(QString("작업 : %1").arg(task));         // 로봇별 작업상태 라벨 갱신
        }, Qt::QueuedConnection);
      }
    );
  }
}

void MainWindow::setupUI()
{
  auto* central  = new QWidget(this);                         // 중앙 위젯 생성
  auto* h_layout = new QHBoxLayout(central);                 // 메인 수평 레이아웃 생성
  h_layout->setContentsMargins(0, 0, 0, 0);                 // 바깥 여백 제거
  h_layout->setSpacing(0);                                   // 레이아웃 간격 제거

  auto* side_panel = new QWidget(central);                   // 우측 정보/제어 패널 생성
  side_panel->setFixedWidth(220);                            // 우측 패널 폭 고정
  side_panel->setStyleSheet("background-color: #1e1e2e;");  // 우측 패널 배경색 지정

  auto* side_main_layout = new QVBoxLayout(side_panel);      // 우측 패널 메인 수직 레이아웃
  side_main_layout->setContentsMargins(10, 10, 10, 10);     // 우측 패널 안쪽 여백 설정
  side_main_layout->setSpacing(6);                           // 위젯 간 간격 설정

  auto makeLine = [&]() {
    auto* line = new QFrame(side_panel);                     // 구분선 위젯 생성
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #313244; max-height: 1px; border: none;");
    return line;
  };

  auto makeSection = [&](const QString& text) {
    auto* lbl = new QLabel(text, side_panel);                // 섹션 제목 라벨 생성
    lbl->setStyleSheet("color: #89b4fa; font-size: 10px; font-weight: bold;");
    return lbl;
  };

  auto makeValue = [&](const QString& text) {
    auto* lbl = new QLabel(text, side_panel);                // 상태값 라벨 생성
    lbl->setStyleSheet("color: #a6e3a1; font-size: 10px; font-weight: bold; padding-left: 6px;");
    return lbl;
  };

  auto makeActionButton = [&](const QString& text) {
    auto* btn = new QPushButton(text, side_panel);           // 제어용 공통 버튼 생성
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

  auto* header_layout = new QHBoxLayout();                   // 상단 헤더 레이아웃 생성
  header_layout->setContentsMargins(0, 0, 0, 0);
  header_layout->setSpacing(6);

  auto* title_box = new QVBoxLayout();                       // 제목/부제목 묶음 레이아웃 생성
  title_box->setContentsMargins(0, 0, 0, 0);
  title_box->setSpacing(1);

  auto* title = new QLabel("🤖  TurtleBot3", side_panel);   // 메인 제목 라벨 생성
  title->setStyleSheet("color: #cdd6f4; font-size: 13px; font-weight: bold;");
  title_box->addWidget(title);

  auto* subtitle = new QLabel("Map Viewer", side_panel);    // 부제목 라벨 생성
  subtitle->setStyleSheet("color: #6c7086; font-size: 10px;");
  title_box->addWidget(subtitle);

  label_db_status_ = new QLabel("OFF", side_panel);         // DB 상태 배지 라벨 생성
  label_db_status_->setFixedSize(42, 24);
  label_db_status_->setAlignment(Qt::AlignCenter);
  label_db_status_->setStyleSheet(
    "QLabel {"
    "  background-color: #f38ba8;"
    "  color: white;"
    "  border-radius: 8px;"
    "  font-size: 10px;"
    "  font-weight: bold;"
    "  padding: 2px 4px;"
    "}"
  );

  page_toggle_btn_ = new QPushButton("제어", side_panel);   // 상태/제어 페이지 전환 버튼 생성
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

  connect(page_toggle_btn_, &QPushButton::clicked, this, &MainWindow::toggleSidePage); // 페이지 전환 버튼 클릭 연결

  header_layout->addLayout(title_box, 1);                   // 제목 박스를 헤더에 추가
  header_layout->addWidget(label_db_status_, 0, Qt::AlignTop); // DB 상태 배지 추가
  header_layout->addWidget(page_toggle_btn_, 0, Qt::AlignTop); // 페이지 전환 버튼 추가

  side_main_layout->addLayout(header_layout);               // 헤더 레이아웃 추가
  side_main_layout->addWidget(makeLine());                  // 헤더 아래 구분선 추가

  side_stack_ = new QStackedWidget(side_panel);             // 우측 패널 페이지 스택 생성

  status_page_ = new QWidget();                             // 상태 페이지 생성
  auto* status_layout = new QVBoxLayout(status_page_);
  status_layout->setContentsMargins(0, 6, 0, 0);
  status_layout->setSpacing(6);

  status_layout->addWidget(makeSection("🤖  로봇 상태"));   // 상태 페이지 섹션 제목 추가

  for (int i = 0; i < 4; ++i) {
    QString robot_name;
    if (i == 0) {
      robot_name = "Master Robot";                          // 1번 로봇 이름 지정
    } else {
      robot_name = QString("Robot %1").arg(i + 1);         // 나머지 로봇 이름 지정
    }

    auto* robot_title = new QLabel(robot_name, status_page_); // 로봇 이름 라벨 생성
    robot_title->setStyleSheet("color: #f9e2af; font-size: 12px; font-weight: bold; padding-top: 2px;");
    status_layout->addWidget(robot_title);

    label_pos_x_[i]       = makeValue("X : -- m");         // X 좌표 라벨 초기화
    label_pos_y_[i]       = makeValue("Y : -- m");         // Y 좌표 라벨 초기화
    label_linear_vel_[i]  = makeValue("선속도 : -- m/s"); // 선속도 라벨 초기화
    label_angular_vel_[i] = makeValue("각속도 : -- rad/s");// 각속도 라벨 초기화
    label_battery_[i]     = makeValue("배터리 : -- %");    // 배터리 라벨 초기화
    label_task_[i]        = makeValue("작업 : 대기 중");   // 작업 상태 라벨 초기화

    label_task_[i]->setStyleSheet("color: #cdd6f4; font-size: 11px; font-weight: bold; padding-left: 6px;");

    status_layout->addWidget(label_pos_x_[i]);             // X 좌표 라벨 추가
    status_layout->addWidget(label_pos_y_[i]);             // Y 좌표 라벨 추가
    status_layout->addWidget(label_linear_vel_[i]);        // 선속도 라벨 추가
    status_layout->addWidget(label_angular_vel_[i]);       // 각속도 라벨 추가
    status_layout->addWidget(label_battery_[i]);           // 배터리 라벨 추가
    status_layout->addWidget(label_task_[i]);              // 작업 상태 라벨 추가

    if (i != 3) {
      status_layout->addSpacing(2);                        // 로봇 블록 사이 간격 추가
      status_layout->addWidget(makeLine());                // 로봇 블록 사이 구분선 추가
      status_layout->addSpacing(2);
    }
  }

  status_layout->addStretch();                             // 상태 페이지 하단 여백 확보

  auto* status_scroll = new QScrollArea(side_panel);       // 상태 페이지 스크롤 영역 생성
  status_scroll->setWidgetResizable(true);
  status_scroll->setFrameShape(QFrame::NoFrame);
  status_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  status_scroll->setStyleSheet(
    "QScrollArea { background: transparent; border: none; }"
    "QScrollBar:vertical {"
    "  background: #1e1e2e;"
    "  width: 8px;"
    "  margin: 0px;"
    "}"
    "QScrollBar::handle:vertical {"
    "  background: #45475a;"
    "  border-radius: 4px;"
    "  min-height: 20px;"
    "}"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
    "  height: 0px;"
    "}"
  );
  status_scroll->setWidget(status_page_);                  // 상태 페이지를 스크롤 영역에 연결

  control_page_ = new QWidget(side_panel);                 // 제어 페이지 생성
  auto* control_layout = new QVBoxLayout(control_page_);
  control_layout->setContentsMargins(0, 6, 0, 0);
  control_layout->setSpacing(8);

  control_layout->addWidget(makeSection("🕹️  작업 / 제어")); // 제어 페이지 섹션 제목 추가

  btn_fetch_box_ = makeActionButton("상자 가져와");       // 상자 가져오기 버튼 생성
  btn_move_goal_ = makeActionButton("목표위치 이동");     // 목표 위치 이동 버튼 생성
  btn_set_goal_  = makeActionButton("목표위치 설정");     // 목표 위치 설정 버튼 생성
  btn_select_robot_ = makeActionButton("특정로봇 선택"); // 특정 로봇 선택 버튼 생성
  btn_zoom_in_ = makeActionButton("줌 +");               // 줌 인 버튼 생성
  btn_zoom_out_ = makeActionButton("줌 -");              // 줌 아웃 버튼 생성
  btn_reset_view_ = makeActionButton("뷰 리셋");          // 뷰 리셋 버튼 생성

  control_layout->addWidget(btn_fetch_box_);              // 버튼들 제어 페이지에 추가
  control_layout->addWidget(btn_move_goal_);
  control_layout->addWidget(btn_set_goal_);
  control_layout->addWidget(btn_select_robot_);
  control_layout->addWidget(btn_zoom_in_);
  control_layout->addWidget(btn_zoom_out_);
  control_layout->addWidget(btn_reset_view_);

  control_layout->addSpacing(4);
  control_layout->addWidget(makeLine());
  control_layout->addSpacing(4);

  auto* guide = new QLabel("추후 여기에 추가 버튼,\n입력창, 선택 옵션을 확장할 수 있습니다.", control_page_); // 향후 확장 안내 문구
  guide->setStyleSheet("color: #7f849c; font-size: 10px; line-height: 1.3;");
  guide->setWordWrap(true);
  control_layout->addWidget(guide);

  control_layout->addStretch();                            // 제어 페이지 하단 여백 확보

  side_stack_->addWidget(status_scroll);                   // 1페이지 상태 화면 등록
  side_stack_->addWidget(control_page_);                   // 2페이지 제어 화면 등록

  side_main_layout->addWidget(side_stack_, 1);             // 스택 위젯을 우측 패널에 추가

  auto* version = new QLabel("ROS2 Humble  |  Qt5", side_panel); // 버전 정보 라벨 생성
  version->setStyleSheet("color: #45475a; font-size: 9px;");
  version->setAlignment(Qt::AlignCenter);
  side_main_layout->addWidget(version);

  render_panel_ = new rviz_common::RenderPanel(central);   // 좌측 RViz 렌더 패널 생성

  h_layout->addWidget(render_panel_, 1);                   // 좌측 RViz 패널 배치
  h_layout->addWidget(side_panel);                         // 우측 정보/제어 패널 배치

  setCentralWidget(central);                               // 중앙 위젯 설정

  connect(btn_fetch_box_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 상자 가져오는 중");    // 상자 가져오기 버튼 클릭 시 작업 라벨 임시 갱신
  });

  connect(btn_move_goal_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 목표 위치 이동 중");   // 목표 이동 버튼 클릭 시 작업 라벨 임시 갱신
  });

  connect(btn_set_goal_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 목표 위치 설정 대기"); // 목표 설정 버튼 클릭 시 작업 라벨 임시 갱신
  });

  connect(btn_select_robot_, &QPushButton::clicked, this, [this]() {
    label_task_[0]->setText("작업 : 특정 로봇 선택 모드"); // 로봇 선택 버튼 클릭 시 작업 라벨 임시 갱신
  });

  connect(btn_reset_view_, &QPushButton::clicked, this, &MainWindow::resetView);     // 뷰 리셋 버튼 연결
  connect(btn_zoom_in_, &QPushButton::clicked, this, &MainWindow::zoomInView);       // 줌 인 버튼 연결
  connect(btn_zoom_out_, &QPushButton::clicked, this, &MainWindow::zoomOutView);     // 줌 아웃 버튼 연결
}

void MainWindow::setupRViz()
{
  ros_node_abs_ = std::make_shared<rviz_common::ros_integration::RosNodeAbstraction>("rviz_node"); // RViz 내부 ROS 노드 생성
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr weak_node = ros_node_abs_;

  manager_ = new rviz_common::VisualizationManager(
    render_panel_, weak_node, nullptr,
    ros_node_abs_->get_raw_node()->get_clock());           // RViz 시각화 매니저 생성

  render_panel_->setFocusPolicy(Qt::StrongFocus);          // RViz 패널 포커스 허용
  render_panel_->setMouseTracking(true);                   // 마우스 추적 활성화

  render_panel_->initialize(manager_);                     // RViz 패널 초기화
  manager_->initialize();                                  // RViz 매니저 초기화
  manager_->startUpdate();                                 // RViz 업데이트 시작

  manager_->setFixedFrame("map");                          // RViz 고정 프레임을 map으로 설정

  auto* view_manager = manager_->getViewManager();         // RViz ViewManager 획득
  if (view_manager) {
    view_manager->setCurrentViewControllerType("rviz_default_plugins/Orbit"); // Orbit 뷰 사용

    auto* view = view_manager->getCurrent();
    if (view) {
      if (auto* target = view->subProp("Target Frame")) {
        target->setValue("map");                           // 카메라 타겟 프레임을 map으로 설정
      }

      if (auto* focal_shape = view->subProp("Focal Shape Size")) {
        focal_shape->setValue(0.05);                       // 초점 표시 크기 설정
      }

      if (auto* distance = view->subProp("Distance")) {
        distance->setValue(18.0);                          // 초기 카메라 거리 설정
      }

      if (auto* pitch = view->subProp("Pitch")) {
        pitch->setValue(0.95);                             // 초기 카메라 pitch 설정
      }

      if (auto* yaw = view->subProp("Yaw")) {
        yaw->setValue(0.85);                               // 초기 카메라 yaw 설정
      }

      auto* focal = view->subProp("Focal Point");
      if (focal) {
        if (auto* x = focal->subProp("X")) {
          x->setValue(0.0);                                // 초기 초점 X 설정
        }
        if (auto* y = focal->subProp("Y")) {
          y->setValue(0.0);                                // 초기 초점 Y 설정
        }
        if (auto* z = focal->subProp("Z")) {
          z->setValue(0.0);                                // 초기 초점 Z 설정
        }
      }
    }
  }

  addMapDisplay();                                         // 맵 display 추가
  addTFDisplay();                                          // TF display 추가
  addRobotModelDisplay();                                  // 로봇 모델 display 추가
  addLaserScanDisplay();                                   // 라이다 display 추가

  auto* tool_manager = manager_->getToolManager();         // RViz ToolManager 획득
  if (tool_manager) {
    auto* move_tool = tool_manager->addTool("rviz_default_plugins/MoveCamera"); // 기본 마우스 툴을 카메라 이동으로 설정
    if (move_tool) {
      tool_manager->setCurrentTool(move_tool);
    }
  }
}

void MainWindow::addMapDisplay()
{
  auto* map = manager_->createDisplay("rviz_default_plugins/Map", "Map", true); // OccupancyGrid 맵 display 생성
  map->subProp("Topic")->setValue("/map");                                       // 맵 토픽을 /map으로 설정
  map->subProp("Alpha")->setValue(0.95);                                         // 맵 투명도 설정
}

void MainWindow::addTFDisplay()
{
  auto* tf = manager_->createDisplay("rviz_default_plugins/TF", "TF", true);     // TF display 생성
  tf->subProp("Show Axes")->setValue(true);                                      // 축 표시 활성화
  tf->subProp("Show Names")->setValue(false);                                    // 프레임 이름 표시는 비활성화
}

void MainWindow::addRobotModelDisplay()
{
  auto* robot = manager_->createDisplay("rviz_default_plugins/RobotModel", "Robot Model", true); // RobotModel display 생성
  robot->subProp("Description Topic")->setValue("/robot_description");                              // robot_description 토픽 사용
}

void MainWindow::addLaserScanDisplay()
{
  auto* scan = manager_->createDisplay("rviz_default_plugins/LaserScan", "LaserScan", true); // LaserScan display 생성
  scan->subProp("Topic")->setValue("/scan");                                                   // 라이다 토픽을 /scan으로 설정
  scan->subProp("Size (m)")->setValue(0.03);                                                   // 스캔 포인트 크기 설정
}

void MainWindow::spinOnce()
{
  rclcpp::spin_some(node_);                                 // ROS 콜백 큐를 1회 처리
}

void MainWindow::toggleSidePage()
{
  if (!side_stack_) {
    return;                                                 // 페이지 스택 없으면 종료
  }

  if (side_stack_->currentIndex() == 0) {
    side_stack_->setCurrentIndex(1);                        // 상태 페이지 -> 제어 페이지 전환
    page_toggle_btn_->setText("상태");
  } else {
    side_stack_->setCurrentIndex(0);                        // 제어 페이지 -> 상태 페이지 전환
    page_toggle_btn_->setText("제어");
  }
}

void MainWindow::resetView()
{
  if (!manager_) {
    return;                                                 // RViz 매니저 없으면 종료
  }

  auto* view_manager = manager_->getViewManager();
  if (!view_manager) {
    return;                                                 // ViewManager 없으면 종료
  }

  view_manager->setCurrentViewControllerType("rviz_default_plugins/Orbit"); // Orbit 뷰로 재설정

  auto* view = view_manager->getCurrent();
  if (!view) {
    return;                                                 // 현재 뷰 없으면 종료
  }

  if (auto* target = view->subProp("Target Frame")) {
    target->setValue("map");                                // 타겟 프레임을 map으로 초기화
  }

  if (auto* focal_shape = view->subProp("Focal Shape Size")) {
    focal_shape->setValue(0.05);                            // 초점 표시 크기 초기화
  }

  if (auto* distance = view->subProp("Distance")) {
    distance->setValue(18.0);                               // 카메라 거리 초기화
  }

  if (auto* pitch = view->subProp("Pitch")) {
    pitch->setValue(0.95);                                  // 카메라 pitch 초기화
  }

  if (auto* yaw = view->subProp("Yaw")) {
    yaw->setValue(0.85);                                    // 카메라 yaw 초기화
  }

  auto* focal = view->subProp("Focal Point");
  if (focal) {
    if (auto* x = focal->subProp("X")) {
      x->setValue(0.0);                                     // 초점 X 초기화
    }
    if (auto* y = focal->subProp("Y")) {
      y->setValue(0.0);                                     // 초점 Y 초기화
    }
    if (auto* z = focal->subProp("Z")) {
      z->setValue(0.0);                                     // 초점 Z 초기화
    }
  }
}

void MainWindow::zoomInView()
{
  if (!manager_) {
    return;                                                 // RViz 매니저 없으면 종료
  }

  auto* view_manager = manager_->getViewManager();
  if (!view_manager) {
    return;                                                 // ViewManager 없으면 종료
  }

  auto* view = view_manager->getCurrent();
  if (!view) {
    return;                                                 // 현재 뷰 없으면 종료
  }

  auto* distance = view->subProp("Distance");
  if (!distance) {
    return;                                                 // 거리 속성 없으면 종료
  }

  double current = distance->getValue().toDouble();         // 현재 카메라 거리 읽기
  current *= 0.85;                                          // 15% 확대

  if (current < 2.0) {
    current = 2.0;                                          // 최소 확대 한계 설정
  }

  distance->setValue(current);                              // 새 거리값 적용
}

void MainWindow::zoomOutView()
{
  if (!manager_) {
    return;                                                 // RViz 매니저 없으면 종료
  }

  auto* view_manager = manager_->getViewManager();
  if (!view_manager) {
    return;                                                 // ViewManager 없으면 종료
  }

  auto* view = view_manager->getCurrent();
  if (!view) {
    return;                                                 // 현재 뷰 없으면 종료
  }

  auto* distance = view->subProp("Distance");
  if (!distance) {
    return;                                                 // 거리 속성 없으면 종료
  }

  double current = distance->getValue().toDouble();         // 현재 카메라 거리 읽기
  current *= 1.15;                                          // 15% 축소

  if (current > 80.0) {
    current = 80.0;                                         // 최대 축소 한계 설정
  }

  distance->setValue(current);                              // 새 거리값 적용
}

void MainWindow::updateDbIndicator(bool connected)
{
  if (!label_db_status_) {
    return;                                                 // DB 상태 라벨 없으면 종료
  }

  if (connected) {
    label_db_status_->setText("ON");                        // 연결 성공 시 ON 표시
    label_db_status_->setStyleSheet(
      "QLabel {"
      "  background-color: #a6e3a1;"
      "  color: #1e1e2e;"
      "  border-radius: 8px;"
      "  font-size: 10px;"
      "  font-weight: bold;"
      "  padding: 2px 4px;"
      "}"
    );
  } else {
    label_db_status_->setText("OFF");                       // 연결 실패 시 OFF 표시
    label_db_status_->setStyleSheet(
      "QLabel {"
      "  background-color: #f38ba8;"
      "  color: white;"
      "  border-radius: 8px;"
      "  font-size: 10px;"
      "  font-weight: bold;"
      "  padding: 2px 4px;"
      "}"
    );
  }
}

void MainWindow::refreshDbStatus()
{
  if (!pDb) {
    updateDbIndicator(false);                               // DB 객체 없으면 OFF 표시
    return;
  }

  const bool ok = pDb->ping();                              // DB ping으로 실제 연결 상태 확인
  updateDbIndicator(ok);                                    // 배지 ON/OFF 갱신
}
