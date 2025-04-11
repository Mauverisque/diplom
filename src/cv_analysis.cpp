#include "cv_analysis.h"

cv::Mat cvMatFromQPixmap(const QPixmap &pixmap) {
  const QImage image{pixmap.toImage()};

  // Assuming the image has a QImage::Format_RGB32 format
  // it needs to be copied into a cv::Mat with 4 channels
  cv::Mat mat{image.height(), image.width(), CV_8UC4,
              const_cast<uchar *>(image.bits()),
              static_cast<size_t>(image.bytesPerLine())};

  // Drop the alpha channel
  cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR, 0, cv::ALGO_HINT_ACCURATE);

  return mat;
}

QPixmap qPixmapFromCvMat(const cv::Mat &mat) {
  // If the mat is a single-channel image (e.g. a threshold image)
  if (mat.type() == CV_8U) {
    const QImage image{mat.data, mat.cols, mat.rows, static_cast<int>(mat.step),
                       QImage::Format_Grayscale8};

    return QPixmap::fromImage(image);
  }

  const QImage image{mat.data, mat.cols, mat.rows, static_cast<int>(mat.step),
                     QImage::Format_BGR888};

  return QPixmap::fromImage(image);
}

cv::Mat grayThresh(const QPixmap &pixmap) {
  cv::Mat img_gray;
  cv::cvtColor(cvMatFromQPixmap(pixmap), img_gray, cv::COLOR_BGR2GRAY, 0,
               cv::ALGO_HINT_ACCURATE);

  cv::Mat img_blur;
  cv::GaussianBlur(img_gray, img_blur, cv::Size(5, 5), 1);

  cv::Mat img_thresh;
  // Use cv::THRESH_BINARY_INV flag to invert the threshold image
  cv::adaptiveThreshold(img_blur, img_thresh, 255,
                        cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV,
                        11, 2);

  // Connect the thresholding pieces for a continuous contour
  cv::morphologyEx(img_thresh, img_thresh, cv::MORPH_CLOSE,
                   cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)));

  return img_thresh;
}

std::vector<cv::Point> biggestContour(const QPixmap &pixmap,
                                      const bool selected) {
  std::vector<std::vector<cv::Point>> contours;
  // Use cv::RETR_EXTERNAL to prevent nested contours
  selected ? cv::findContours(selectedObjectThresh(pixmap), contours,
                              cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE)
           : cv::findContours(grayThresh(pixmap), contours, cv::RETR_EXTERNAL,
                              cv::CHAIN_APPROX_SIMPLE);

  double max_contour_area{0};
  int max_contour_idx{0};
  for (size_t i = 0; i != contours.size(); ++i) {
    const cv::Moments moments{cv::moments(contours[i], true)};
    if (moments.m00 > max_contour_area) {
      max_contour_area = moments.m00;
      max_contour_idx = i;
    }
  }

  return contours[max_contour_idx];
}

cv::Mat objectThresh(const QPixmap &pixmap) {
  cv::Mat mask = cv::Mat::zeros(pixmap.height(), pixmap.width(), CV_8UC1);
  cv::drawContours(
      mask, std::vector<std::vector<cv::Point>>{biggestContour(pixmap, false)},
      -1, cv::Scalar(255), cv::FILLED);

  return mask;
}

cv::Mat selectedObjectThresh(const QPixmap &pixmap) {
  const cv::Mat img = cvMatFromQPixmap(pixmap);

  cv::Mat hsv_img;
  cv::cvtColor(img, hsv_img, cv::COLOR_BGR2HSV);

  int hue_bins{6};

  const std::array<float, 2> hue_range{0, 180};
  std::vector<const float *> ranges{hue_range.data()};

  const std::vector<int> channels{0};

  cv::MatND hist;
  const std::vector<int> hist_size{hue_bins};

  const cv::Mat mask{objectThresh(pixmap)};
  cv::calcHist(&hsv_img, 1, channels.data(), mask, hist, 1, hist_size.data(),
               ranges.data());

  hist.convertTo(hist, CV_32F);
  const int K{2};
  cv::Mat labels, centers;
  const cv::TermCriteria criteria(
      cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 10, 1.0);
  cv::kmeans(hist, K, labels, criteria, 3, cv::KMEANS_PP_CENTERS, centers);

  int zero_count{0};
  int one_count{0};
  for (int i = 0; i < labels.size().height; ++i) {
    labels.at<int>(i) == 0 ? zero_count += hist.at<float>(i)
                           : one_count += hist.at<float>(i);
  }

  const int majority_class{(zero_count > one_count) ? 0 : 1};

  cv::Mat majority_class_mask{cv::Mat::zeros(hsv_img.size(), CV_8UC1)};
  for (int y = 0; y < hsv_img.rows; ++y) {
    for (int x = 0; x < hsv_img.cols; ++x) {
      if (mask.at<uchar>(y, x) != 0) {
        int hue_value = hsv_img.at<cv::Vec3b>(y, x)[0];
        int bin_index = (hue_value * hue_bins) / 180;
        if (labels.at<int>(bin_index) == majority_class) {
          majority_class_mask.at<uchar>(y, x) = 255;
        }
      }
    }
  }

  cv::morphologyEx(majority_class_mask, majority_class_mask, cv::MORPH_CLOSE,
                   cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

  return majority_class_mask;
}

cv::Scalar meanBgr(const QPixmap &pixmap) {
  return cv::mean(cvMatFromQPixmap(pixmap), selectedObjectThresh(pixmap));
}

QPixmap threshQPixmap(const QPixmap &pixmap) {
  return qPixmapFromCvMat(selectedObjectThresh(pixmap));
}

QPixmap solidColorQPixmap(const QPixmap &pixmap) {
  return qPixmapFromCvMat(
      {cv::Size(pixmap.width(), pixmap.height()), CV_8UC3, meanBgr(pixmap)});
}

QPixmap contourQPixmap(const QPixmap &pixmap) {
  const cv::Mat mat{cvMatFromQPixmap(pixmap)};

  // Create a black canvas
  cv::Mat img_cont{cv::Size(mat.cols, mat.rows), CV_8UC3};
  img_cont.setTo(cv::Scalar(0, 0, 0));

  const cv::Scalar mean_bgr{meanBgr(pixmap)};

  cv::drawContours(
      img_cont, std::vector<std::vector<cv::Point>>{biggestContour(pixmap, 1)},
      -1, mean_bgr, cv::FILLED);

  return qPixmapFromCvMat(img_cont);
}

QPixmap fitEllipseQPixmap(const QPixmap &pixmap) {
  const cv::Mat mat{cvMatFromQPixmap(pixmap)};

  // Create a black canvas
  cv::Mat img_ellipse{cv::Size(mat.cols, mat.rows), CV_8UC3};
  img_ellipse.setTo(cv::Scalar(0, 0, 0));

  const cv::RotatedRect ellipse{cv::fitEllipse(biggestContour(pixmap, 1))};
  cv::ellipse(img_ellipse, ellipse, meanBgr(pixmap), mat.cols / 40);

  return qPixmapFromCvMat(img_ellipse);
}

cv::Scalar meanHsv(const QPixmap &pixmap) {
  cv::Mat img_hsv;
  cv::cvtColor(cvMatFromQPixmap(pixmap), img_hsv, cv::COLOR_BGR2HSV, 0,
               cv::ALGO_HINT_ACCURATE);

  return cv::mean(img_hsv, selectedObjectThresh(pixmap));
}

double contourArea(const QPixmap &pixmap) {
  return cv::contourArea(biggestContour(pixmap, 1)) /
         (pixmap.height() * pixmap.width());
}

cv::Scalar fitEllipseParams(const QPixmap &pixmap) {
  const cv::RotatedRect ellipse{cv::fitEllipse(biggestContour(pixmap, 1))};

  return cv::Scalar{ellipse.size.height, ellipse.size.width,
                    (ellipse.size.width / ellipse.size.height)};
}
