//
// Created by YJK on 2025/4/10.
//
#include "common/utils.h"


#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#include <sys/stat.h>
    #include <unistd.h>
    #include <errno.h>
    #define PATH_SEPARATOR "/"
#endif

bool dirExists(const std::string& dirName_in) {
#ifdef _WIN32
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;
    return false;
#else
    struct stat info;
    if (stat(dirName_in.c_str(), &info) != 0)
        return false;
    return (info.st_mode & S_IFDIR);
#endif
}

bool createDir(const std::string& dirName_in) {
#ifdef _WIN32
    return CreateDirectoryA(dirName_in.c_str(), NULL) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
#else
    mode_t mode = 0755;
    int ret = mkdir(dirName_in.c_str(), mode);
    return (ret == 0 || errno == EEXIST);
#endif
}

// 递归创建目录
bool createDirRecursive(const std::string& dirPath) {
    if (dirExists(dirPath)) {
        return true;
    }

    size_t pos = dirPath.find_last_of(PATH_SEPARATOR);
    if (pos != std::string::npos) {
        std::string parentDir = dirPath.substr(0, pos);
        if (!dirExists(parentDir)) {
            if (!createDirRecursive(parentDir)) {
                return false;
            }
        }
    }

    return createDir(dirPath);
}

double calculateAngle(const cv::Point& center, const cv::Point& point) {
    /**
     * 计算从 center 到 point 的角度，单位为度，顺时针方向为正。
     */
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    double angle = std::atan2(dy, dx) * 180.0 / CV_PI;
    return std::fmod(angle + 360.0, 360.0);
}

double vectorAngle(const cv::Point& base, const cv::Point& v1, const cv::Point& v2) {
    /**
     * 计算从向量 base→v1 到 base→v2 的顺时针夹角（单位：度）
     */
    cv::Point a = v1 - base;
    cv::Point b = v2 - base;

    double dot = a.x * b.x + a.y * b.y;
    double det = a.x * b.y - a.y * b.x;  // 2D叉积
    double angle = std::atan2(det, dot) * 180.0 / CV_PI;
    return std::fmod(angle + 360.0, 360.0);  // 顺时针夹角
}

double getGaugeReading(const std::vector<int>& poseCls, const std::vector<std::vector<cv::Point>>& poseKeypointXY, double startValue, double endValue)
{
    /**
     * 计算仪表盘当前指针的读数。
     *
     * 参数：
     *     poseCls: 每个关键点组的类型（0=指针，1=起始点，2=终止点）
     *     poseKeypointXY: 每个关键点组的两个坐标（[base, tip] 或 [point, (0,0)])
     *     startValue: 仪表盘起始读数
     *     endValue: 仪表盘终止读数
     *
     * 返回：
     *     仪表盘当前的读数
     */
    cv::Point pointerBase, pointerTip, startPoint, endPoint;
    double res;

    // 解析关键点
    for (size_t i = 0; i < poseCls.size() && i < poseKeypointXY.size(); ++i) {
        const auto& cls = poseCls[i];
        const auto& keypoints = poseKeypointXY[i];

        if (cls == 0 && keypoints.size() >= 2 &&
            keypoints[0] != cv::Point(0, 0) && keypoints[1] != cv::Point(0, 0)) {
            pointerBase = keypoints[0];
            pointerTip = keypoints[1];
        } else if (cls == 1 && keypoints.size() >= 1) {
            startPoint = keypoints[0];
        } else if (cls == 2 && keypoints.size() >= 1) {
            endPoint = keypoints[0];
        }
    }

    // 检查是否所有必要的点都找到了
    if (pointerBase == cv::Point(0, 0) || pointerTip == cv::Point(0, 0) ||
        startPoint == cv::Point(0, 0) || endPoint == cv::Point(0, 0)) {
        return res;
    }

    // 计算总扇形角度（从起点指向终点的角度）
    double totalAngle = vectorAngle(pointerBase, startPoint, endPoint);

    // 计算指针当前的角度相对起始角
    double pointerAngle = vectorAngle(pointerBase, startPoint, pointerTip);

    // 限制角度在扇形范围内
    pointerAngle = std::min(pointerAngle, totalAngle);

    // 比例 = 当前指针角度 / 总角度
    double ratio = (totalAngle != 0) ? pointerAngle / totalAngle : 0;
    double reading = startValue + ratio * (endValue - startValue);

    // 结果取四位小数
    res = std::round(reading * 10000.0) / 10000.0;

    return res;
}
