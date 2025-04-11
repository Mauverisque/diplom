#ifndef CV_ANALYSIS_H_
#define CV_ANALYSIS_H_

#include <iostream>

#include <QDebug>
#include <QPixmap>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// Converter functions
cv::Mat cvMatFromQPixmap(const QPixmap &pixmap);
QPixmap qPixmapFromCvMat(const cv::Mat &mat);

// OpenCV image processing functions
cv::Mat grayThresh(const QPixmap &pixmap);
std::vector<cv::Point> biggestContour(const QPixmap &pixmap,
                                      const bool selected);
cv::Mat objectThresh(const QPixmap &pixmap);
cv::Mat selectedObjectThresh(const QPixmap &pixmap);
cv::Scalar meanBgr(const QPixmap &pixmap);

// Functions for convenient use in main_window.cpp
QPixmap threshQPixmap(const QPixmap &pixmap);
QPixmap solidColorQPixmap(const QPixmap &pixmap);
QPixmap contourQPixmap(const QPixmap &pixmap);
QPixmap fitEllipseQPixmap(const QPixmap &pixmap);
cv::Scalar meanHsv(const QPixmap &pixmap);
double contourArea(const QPixmap &pixmap);
cv::Scalar fitEllipseParams(const QPixmap &pixmap);

#endif // CV_ANALYSIS_H_
