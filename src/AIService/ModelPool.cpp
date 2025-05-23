//
// Created by YJK on 2025/5/22.
//

#include "AIService/ModelPool.h"
#include <fstream>

bool ModelPool::initialize(const std::string& modelPath, int modelType, float threshold) {
    std::unique_lock<std::mutex> lock(poolMutex_);

    if (!availableModels_.empty()) {
        Logger::warning("Model pool already initialized for type: " + std::to_string(modelType));
        return false;
    }

    // 验证模型文件存在
    std::ifstream modelFile(modelPath);
    if (!modelFile.good()) {
        Logger::error("Model file does not exist: " + modelPath);
        return false;
    }
    modelFile.close();

    modelPath_ = modelPath;
    modelType_ = modelType;
    threshold_ = threshold;

    Logger::info("Initializing model pool for type " + std::to_string(modelType) +
                 " with " + std::to_string(maxPoolSize_) + " instances");

    // 创建模型实例池
    for (size_t i = 0; i < maxPoolSize_; ++i) {
        try {
            auto model = std::make_shared<rknn_lite>(
                    const_cast<char*>(modelPath.c_str()),
                    modelType % 3,
                    modelType,
                    threshold
            );

            availableModels_.push(model);
            allModels_.insert(model);

            Logger::debug("Created model instance " + std::to_string(i) + " for type " + std::to_string(modelType));

        } catch (const std::exception& e) {
            Logger::error("Failed to create model instance " + std::to_string(i) +
                          " for type " + std::to_string(modelType) + ": " + e.what());

            // 清理已创建的实例
            while (!availableModels_.empty()) {
                availableModels_.pop();
            }
            allModels_.clear();
            return false;
        }
    }

    enabled_.store(true);
    Logger::info("Model pool initialized successfully for type " + std::to_string(modelType) +
                 " with " + std::to_string(maxPoolSize_) + " instances");
    return true;
}

std::shared_ptr<rknn_lite> ModelPool::acquireModel(int timeoutMs) {
    totalAcquires_++;

    if (!enabled_.load() || shutdown_.load()) {
        Logger::debug("Model pool disabled or shutdown for type: " + std::to_string(modelType_));
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(poolMutex_);

    // 等待可用模型或超时
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    if (!condition_.wait_until(lock, deadline, [this] {
        return !availableModels_.empty() || shutdown_.load();
    })) {
        timeoutCount_++;
        Logger::warning("Model acquisition timeout after " + std::to_string(timeoutMs) +
                        "ms for type: " + std::to_string(modelType_));
        return nullptr;
    }

    if (shutdown_.load() || availableModels_.empty()) {
        return nullptr;
    }

    auto model = availableModels_.front();
    availableModels_.pop();

    Logger::debug("Acquired model for type " + std::to_string(modelType_) +
                  ", remaining available: " + std::to_string(availableModels_.size()));

    return model;
}

void ModelPool::releaseModel(std::shared_ptr<rknn_lite> model) {
    if (!model || shutdown_.load()) {
        return;
    }

    totalReleases_++;

    // 验证这个模型确实属于这个池
    if (allModels_.find(model) == allModels_.end()) {
        Logger::error("Attempt to release model that doesn't belong to pool type: " +
                      std::to_string(modelType_));
        return;
    }

    clearModelResources(model);

    std::unique_lock<std::mutex> lock(poolMutex_);
    availableModels_.push(model);
    condition_.notify_one();

    Logger::debug("Released model for type " + std::to_string(modelType_) +
                  ", available: " + std::to_string(availableModels_.size()));
}

ModelPool::PoolStatus ModelPool::getStatus() const {
    std::unique_lock<std::mutex> lock(poolMutex_);

    PoolStatus status;
    status.totalModels = allModels_.size();
    status.availableModels = availableModels_.size();
    status.busyModels = status.totalModels - status.availableModels;
    status.isEnabled = enabled_.load();
    status.modelPath = modelPath_;
    status.modelType = modelType_;
    status.threshold = threshold_;

    return status;
}

void ModelPool::setEnabled(bool enabled) {
    bool oldValue = enabled_.exchange(enabled);
    if (oldValue != enabled) {
        Logger::info("Model pool for type " + std::to_string(modelType_) +
                     " status changed to: " + (enabled ? "enabled" : "disabled"));

        if (enabled) {
            // 通知等待的线程
            std::unique_lock<std::mutex> lock(poolMutex_);
            condition_.notify_all();
        }
    }
}

void ModelPool::shutdown() {
    if (shutdown_.exchange(true)) {
        return; // 已经关闭
    }

    Logger::info("Shutting down model pool for type: " + std::to_string(modelType_));

    std::unique_lock<std::mutex> lock(poolMutex_);
    condition_.notify_all();

    // 清理资源
    while (!availableModels_.empty()) {
        availableModels_.pop();
    }
    allModels_.clear();

    Logger::info("Model pool shutdown completed for type: " + std::to_string(modelType_) +
                 ", total acquires: " + std::to_string(totalAcquires_.load()) +
                 ", total releases: " + std::to_string(totalReleases_.load()) +
                 ", timeouts: " + std::to_string(timeoutCount_.load()));
}

void ModelPool::clearModelResources(std::shared_ptr<rknn_lite> model) {
    if (model) {
        // 清理模型内部资源
        model->ori_img.release();
        model->results_vector.clear();
        model->results_vector.shrink_to_fit();
        model->plateResults.clear();
        model->plateResults.shrink_to_fit();
    }
}