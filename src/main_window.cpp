#include "main_window.h"
#include "cv_analysis.h"
#include "sqlite_database.h"

MainWindow::MainWindow(SqliteDatabase &db, QWidget *parent)
    : QMainWindow{parent}, m_db{db} {
  setWindowTitle("Diplom");
  setFixedSize(1000, 600);

  db.openDbConnection();

  initWidgets();
  initTimers();
  initSerialPort();
  initLayouts();
  initMenus();
}

MainWindow::~MainWindow() { m_serial_port->close(); }

void MainWindow::openImage() {
  QFileDialog dialog{this, "Open Image"};
  dialog.setDirectory(QDir("../images_for_classifying"));
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setNameFilter("Images (*.jpg)");

  if (dialog.exec()) {
    const QString path{dialog.selectedFiles().first()};
    getImages(path, path.section('/', -1, -1));
  }
}

void MainWindow::autoTrain() {
  QFileDialog dialog{this, "Auto Train"};
  dialog.setDirectory(QDir("../images_for_training"));
  dialog.setFileMode(QFileDialog::ExistingFile);

  if (dialog.exec()) {
    QDirIterator iter{dialog.directory()};
    while (iter.hasNext()) {
      const QString path{iter.next()};
      if (path.endsWith("jpg")) {
        getImages(path, path.section('/', -1, -1));
        m_line_edit->setText(path.section('/', -1, -1).section('_', 0, 0));
        onButtonPress();
      }
    }
  }
}

void MainWindow::toggleImage() {
  m_check_box->isChecked()
      ? m_img_lbl->setPixmap(m_thresh_pmap.scaled(
            marginlessContentBoxSize(m_img_grbox), Qt::KeepAspectRatio))
      : m_img_lbl->setPixmap(m_img_pmap.scaled(
            marginlessContentBoxSize(m_img_grbox), Qt::KeepAspectRatio));
}

void MainWindow::toggleButtonText() {
  m_line_edit->text().isEmpty() ? m_line_edit_bttn->setText("Get Name")
                                : m_line_edit_bttn->setText("Send Features");
}

void MainWindow::onButtonPress() {
  if (m_line_edit->text().isEmpty()) {
    const std::string name{m_db.objectNameUsingKnn(
        {"", m_hsv[0], m_contour_area, m_ellipse_params[2]})};
    m_line_edit->setText(QString::fromStdString(name));
  } else {
    m_db.execInsertSqlQuery(m_img_name.toStdString(),
                            m_line_edit->text().toStdString(), m_hsv[0],
                            m_contour_area, m_ellipse_params[2]);
    m_line_edit->setText("");
  }
}

void MainWindow::readSerialPort() {
  if (m_serial_port->canReadLine()) {
    const QString data{QString::fromUtf8(m_serial_port->readLine())};
    // Extracts weight value from "Read: X.XXX kg" string
    const QRegularExpression re{"(-?\\d+\\.\\d+)"};
    const QRegularExpressionMatch match{re.match(data)};
    if (match.hasMatch()) {
      m_weight_lbl->setText(QString("%1 kg").arg(match.captured(1)));
    } else {
      qDebug() << "WARNING: Data format does not match expected pattern:"
               << data;
    }
  }
}

void MainWindow::startErrorTimer() {
  if (!m_error_timer->isActive()) {
    m_error_timer->start();
  }
}

void MainWindow::handleSerialPortError() {
  qDebug() << "WARNING: Serial port connection failed";
  m_weight_lbl->setText("N/A kg");
  m_tare_bttn->setDisabled(true);
  m_calibration_spbox->setDisabled(true);
  if (!m_serial_port->isOpen()) {
    m_serial_port->close();
  }
  initSerialPort();
}

void MainWindow::sendCalibrationCommand() {
  if (m_serial_port->isOpen()) {
    m_serial_port->write(
        QString{"CALIBRATE:%1\n"}.arg(m_calibration_spbox->value()).toUtf8());
  }
}

void MainWindow::sendTareCommand() {
  if (m_serial_port->isOpen()) {
    m_serial_port->write("TARE\n");
  }
}

template <typename... WidgetPointer>
QGroupBox *
MainWindow::createGroupBox(const QString &name, const bool is_hbox,
                           const int height,
                           const WidgetPointer... widget_pointer) const {
  QGroupBox *group_box = new QGroupBox(name);

  QBoxLayout *box_layout;

  if (is_hbox) {
    box_layout = new QHBoxLayout;
    (box_layout->addWidget(widget_pointer, 0, Qt::AlignCenter), ...);
  } else {
    box_layout = new QVBoxLayout;
    (box_layout->addWidget(widget_pointer), ...);
  }

  group_box->setLayout(box_layout);
  if (height > 0) {
    group_box->setFixedHeight(height);
  }

  return group_box;
}

void MainWindow::initWidgets() {
  m_img_lbl = new QLabel;
  m_img_grbox = createGroupBox("Original Image", true, 0, m_img_lbl);

  m_check_box = new QCheckBox(m_img_lbl);
  m_check_box->setFocusPolicy(Qt::NoFocus);
  m_check_box->hide();
  connect(m_check_box, &QCheckBox::toggled, this, &MainWindow::toggleImage);

  m_line_edit = new QLineEdit;
  m_line_edit->setAlignment(Qt::AlignCenter);
  m_line_edit->setPlaceholderText("Object Name");
  m_line_edit->setDisabled(true);
  connect(m_line_edit, &QLineEdit::textChanged, this,
          &MainWindow::toggleButtonText);

  m_line_edit_bttn = new QPushButton("Get Name");
  m_line_edit_bttn->setFocusPolicy(Qt::NoFocus);
  m_line_edit_bttn->setDisabled(true);
  connect(m_line_edit_bttn, &QPushButton::clicked, this,
          &MainWindow::onButtonPress);
  connect(m_line_edit, &QLineEdit::returnPressed, m_line_edit_bttn,
          &QPushButton::click);

  m_color_lbl = new QLabel;
  m_hsv_val_lbl = new QLabel;
  m_color_grbox = createGroupBox("Color", true, 0, m_color_lbl, m_hsv_val_lbl);

  m_contour_lbl = new QLabel;
  m_contour_area_lbl = new QLabel;
  m_contour_grbox =
      createGroupBox("Contour", true, 0, m_contour_lbl, m_contour_area_lbl);

  m_ellipse_lbl = new QLabel;
  m_ellipse_params_lbl = new QLabel;
  m_ellipse_grbox =
      createGroupBox("Ellipse", true, 0, m_ellipse_lbl, m_ellipse_params_lbl);

  m_weight_lbl = new QLabel("N/A kg");
  m_tare_bttn = new QPushButton("Tare");
  m_tare_bttn->setDisabled(true);
  connect(m_tare_bttn, &QPushButton::pressed, this,
          &MainWindow::sendTareCommand);
  m_calibration_spbox = new QSpinBox();
  m_calibration_spbox->setRange(-100000, 100000);
  m_calibration_spbox->setDisabled(true);
  connect(m_calibration_spbox, &QSpinBox::valueChanged, this,
          &MainWindow::sendCalibrationCommand);
  m_weight_grbox = createGroupBox("Weight", true, 60, m_weight_lbl, m_tare_bttn,
                                  m_calibration_spbox);

  m_features_grbox =
      createGroupBox("Object Features", false, 0, m_color_grbox,
                     m_contour_grbox, m_ellipse_grbox, m_weight_grbox);
}

void MainWindow::initTimers() {
  m_error_timer = new QTimer();
  m_error_timer->setSingleShot(true);
  m_error_timer->setInterval(1000);
  connect(m_error_timer, &QTimer::timeout, this,
          &MainWindow::handleSerialPortError);
}

void MainWindow::initSerialPort() {
  m_serial_port = new QSerialPort();
  connect(m_serial_port, &QSerialPort::readyRead, this,
          &MainWindow::readSerialPort);
  connect(m_serial_port, &QSerialPort::errorOccurred, this,
          &MainWindow::startErrorTimer);

  m_serial_port->setPortName("COM3");
  m_serial_port->setBaudRate(QSerialPort::Baud9600);
  m_serial_port->setDataBits(QSerialPort::Data8);
  m_serial_port->setParity(QSerialPort::NoParity);
  m_serial_port->setStopBits(QSerialPort::OneStop);
  m_serial_port->setFlowControl(QSerialPort::NoFlowControl);

  if (m_serial_port->open(QIODevice::ReadWrite)) {
    if (m_error_timer->isActive()) {
      m_error_timer->stop();
    }
    m_serial_port->setDataTerminalReady(false);
    m_tare_bttn->setDisabled(false);
    m_calibration_spbox->setDisabled(false);
    m_calibration_spbox->setValue(100000);
    qDebug() << "Serial port opened successfully";
  }
}

void MainWindow::initLayouts() {
  QWidget *cent_widg = new QWidget;
  setCentralWidget(cent_widg);

  QHBoxLayout *main_hbox = new QHBoxLayout;
  {
    QVBoxLayout *img_vbox = new QVBoxLayout;
    {
      img_vbox->addWidget(m_img_grbox);
      QHBoxLayout *line_edit_hbox = new QHBoxLayout;
      {
        line_edit_hbox->addWidget(m_line_edit, 6);
        line_edit_hbox->addWidget(m_line_edit_bttn, 1);
      }
      img_vbox->addLayout(line_edit_hbox, 1);
    }
    main_hbox->addLayout(img_vbox, 7);
    main_hbox->addWidget(m_features_grbox, 3);
  }

  cent_widg->setLayout(main_hbox);
}

void MainWindow::initMenus() {
  QAction *open_img_act = new QAction(
      QIcon::fromTheme(QIcon::ThemeIcon::FolderOpen), "Open Image", this);
  connect(open_img_act, &QAction::triggered, this, &MainWindow::openImage);

  QAction *auto_train_act = new QAction(
      QIcon::fromTheme(QIcon::ThemeIcon::Computer), "Auto Train", this);
  connect(auto_train_act, &QAction::triggered, this, &MainWindow::autoTrain);

  QMenu *file_menu{menuBar()->addMenu("File")};
  file_menu->addAction(open_img_act);
  file_menu->addAction(auto_train_act);
}

void MainWindow::getImages(const QPixmap &pixmap, const QString &pixmap_name) {
  m_img_name = pixmap_name.section('/', -1, -1);

  m_img_pmap = pixmap;

  m_thresh_pmap = threshQPixmap(pixmap);

  m_color_pmap = solidColorQPixmap(pixmap);
  m_hsv = meanHsv(pixmap);
  // Normalize values to a 0 to 1 scale
  m_hsv[0] *= (1.0 / 180);
  m_hsv[1] *= (1.0 / 255);
  m_hsv[2] *= (1.0 / 255);

  m_contour_pmap = contourQPixmap(pixmap);
  m_contour_area = contourArea(pixmap);

  m_ellipse_pmap = fitEllipseQPixmap(pixmap);
  m_ellipse_params = fitEllipseParams(pixmap);

  setImages();
}

void MainWindow::setImages() {
  m_check_box->show();
  m_line_edit->setDisabled(false);
  m_line_edit->setText("");
  m_line_edit_bttn->setDisabled(false);

  toggleImage();

  m_color_lbl->setPixmap(m_color_pmap.scaled(
      marginlessContentBoxSize(m_color_grbox), Qt::KeepAspectRatio));

  m_hsv_val_lbl->setText(QString("Hue:\t%1%\nSat:\t%2%\nVal:\t%3%")
                             .arg(static_cast<int>(m_hsv[0] * 100))
                             .arg(static_cast<int>(m_hsv[1] * 100))
                             .arg(static_cast<int>(m_hsv[2] * 100)));

  m_contour_lbl->setPixmap(m_contour_pmap.scaled(
      marginlessContentBoxSize(m_contour_grbox), Qt::KeepAspectRatio));

  m_contour_area_lbl->setText(
      QString("Area:\t%1%").arg(static_cast<int>(m_contour_area * 100)));

  m_ellipse_lbl->setPixmap(m_ellipse_pmap.scaled(
      marginlessContentBoxSize(m_ellipse_grbox), Qt::KeepAspectRatio));

  m_ellipse_params_lbl->setText(
      QString("Height:\t%1 px\nWidth:\t%2 px\nRound:\t%3%")
          .arg(static_cast<int>(m_ellipse_params[0]))
          .arg(static_cast<int>(m_ellipse_params[1]))
          .arg(static_cast<int>(m_ellipse_params[2] * 100)));
}

QSize MainWindow::marginlessContentBoxSize(const QWidget *w) const {
  return w->contentsRect().size() -
         (QSize(w->contentsMargins().left() + w->contentsMargins().right(),
                w->contentsMargins().top() + w->contentsMargins().bottom()));
}
