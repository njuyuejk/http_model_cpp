//
// Created by YJK on 2025/4/10.
//

#ifndef FFMPEG_PULL_PUSH_UTILS_H
#define FFMPEG_PULL_PUSH_UTILS_H

#include <iostream>
#include <string>
#include "opencv2/opencv.hpp"
#include <cmath>
#include <vector>
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

bool dirExists(const std::string& dirName_in);

bool createDir(const std::string& dirName_in);

bool createDirRecursive(const std::string& dirPath);

double calculateAngle(const cv::Point& center, const cv::Point& point);

double vectorAngle(const cv::Point& base, const cv::Point& v1, const cv::Point& v2);

double getGaugeReading(const std::vector<int>& poseCls, const std::vector<std::vector<cv::Point>>& poseKeypointXY, double startValue, double endValue);

json any_to_json(const std::any& value);

#endif //FFMPEG_PULL_PUSH_UTILS_H
