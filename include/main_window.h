#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <QCheckBox>
#include <QDirIterator>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSerialPort>
#include <QSpinBox>
#include <QTimer>

#include <opencv2/core/mat.hpp>

class SqliteDatabase; // Forward declaration

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(SqliteDatabase &db, QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void openImage();
  void autoTrain();
  void toggleImage();
  void toggleButtonText();
  void onButtonPress();
  void readSerialPort();
  void startErrorTimer();
  void handleSerialPortError();
  void sendCalibrationCommand();
  void sendTareCommand();

private:
  template <typename... WidgetPointer>
  QGroupBox *createGroupBox(const QString &name, const bool is_hbox,
                            const int height,
                            const WidgetPointer... widget_pointer) const;
  void initWidgets();
  void initTimers();
  void initSerialPort();
  void initLayouts();
  void initMenus();

  void getImages(const QPixmap &pixmap, const QString &pixmap_name);
  void setImages();

  QSize marginlessContentBoxSize(const QWidget *w) const;

  SqliteDatabase &m_db;
  QString m_img_name;
  QSerialPort *m_serial_port;
  QTimer *m_error_timer;

  QGroupBox *m_img_grbox;
  QLabel *m_img_lbl;
  QPixmap m_img_pmap;
  QPixmap m_thresh_pmap;
  QCheckBox *m_check_box;

  QLineEdit *m_line_edit;
  QPushButton *m_line_edit_bttn;

  QGroupBox *m_features_grbox;

  QGroupBox *m_color_grbox;
  QLabel *m_color_lbl;
  QPixmap m_color_pmap;
  cv::Scalar m_hsv;
  QLabel *m_hsv_val_lbl;

  QGroupBox *m_contour_grbox;
  QLabel *m_contour_lbl;
  QPixmap m_contour_pmap;
  double m_contour_area;
  QLabel *m_contour_area_lbl;

  QGroupBox *m_ellipse_grbox;
  QLabel *m_ellipse_lbl;
  QPixmap m_ellipse_pmap;
  cv::Scalar m_ellipse_params;
  QLabel *m_ellipse_params_lbl;

  QGroupBox *m_weight_grbox;
  QLabel *m_weight_lbl;
  QPushButton *m_tare_bttn;
  QSpinBox *m_calibration_spbox;
};

#endif // MAIN_WINDOW_H_
