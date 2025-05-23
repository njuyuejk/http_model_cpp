//
// Created by YJK on 2025/5/22.
//

#ifndef MODEL_POOL_H
#define MODEL_POOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <atomic>
#include <set>
#include <unordered_map>
#include "AIService/rknn/rknnPool.h"
#include "common/Logger.h"

/**
 * @brief 线程安全的模型池
 * 维护多个相同类型的模型实例，支持并发访问
 */
class ModelPool {
public:
    explicit ModelPool(size_t poolSize = 3)
            : maxPoolSize_(poolSize), shutdown_(false), enabled_(false) {}

    ~ModelPool() {
        shutdown();
    }

    /**
     * @brief 初始化模型池
     * @param modelPath 模型文件路径
     * @param modelType 模型类型
     * @param threshold 检测阈值
     * @return 初始化是否成功
     */
    bool initialize(const std::string& modelPath, int modelType, float threshold);

    /**
     * @brief 获取一个可用的模型实例
     * @param timeout 超时时间（毫秒）
     * @return 模型实例的智能指针，如果超时返回nullptr
     */
    std::shared_ptr<rknn_lite> acquireModel(int timeoutMs = 5000);

    /**
     * @brief 归还模型实例到池中
     * @param model 模型实例
     */
    void releaseModel(std::shared_ptr<rknn_lite> model);

    /**
     * @brief 获取池的状态信息
     */
    struct PoolStatus {
        size_t totalModels;
        size_t availableModels;
        size_t busyModels;
        bool isEnabled;
        std::string modelPath;
        int modelType;
        float threshold;
    };

    PoolStatus getStatus() const;

    /**
     * @brief 启用/禁用模型池
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_.load(); }

    /**
     * @brief 获取模型类型
     */
    int getModelType() const { return modelType_; }

    /**
     * @brief 关闭模型池
     */
    void shutdown();

    /*
     * @brief 模型资源清理
     * */
    void clearModelResources(std::shared_ptr<rknn_lite> model);

private:
    mutable std::mutex poolMutex_;
    std::condition_variable condition_;
    std::queue<std::shared_ptr<rknn_lite>> availableModels_;
    std::set<std::shared_ptr<rknn_lite>> allModels_;

    size_t maxPoolSize_;
    std::atomic<bool> enabled_;
    std::atomic<bool> shutdown_;

    // 模型配置信息
    std::string modelPath_;
    int modelType_;
    float threshold_;

    // 统计信息
    mutable std::atomic<size_t> totalAcquires_{0};
    mutable std::atomic<size_t> totalReleases_{0};
    mutable std::atomic<size_t> timeoutCount_{0};
};

/**
 * @brief RAII模型获取器
 * 自动管理模型的获取和释放
 */
class ModelAcquirer {
public:
    ModelAcquirer(ModelPool& pool, int timeoutMs = 5000)
            : pool_(pool), model_(pool.acquireModel(timeoutMs)) {}

    ~ModelAcquirer() {
        if (model_) {
            pool_.clearModelResources(model_);
            pool_.releaseModel(model_);
        }
    }

    // 禁止拷贝
    ModelAcquirer(const ModelAcquirer&) = delete;
    ModelAcquirer& operator=(const ModelAcquirer&) = delete;

    // 支持移动
    ModelAcquirer(ModelAcquirer&& other) noexcept
            : pool_(other.pool_), model_(std::move(other.model_)) {}

    rknn_lite* get() const { return model_.get(); }
    rknn_lite* operator->() const { return model_.get(); }
    rknn_lite& operator*() const { return *model_; }

    bool isValid() const { return model_ != nullptr; }

private:
    ModelPool& pool_;
    std::shared_ptr<rknn_lite> model_;
};

/**
 * @brief 并发监控器
 * 统计并发请求信息
 */
class ConcurrencyMonitor {
public:
    void requestStarted() {
        activeRequests_++;
        totalRequests_++;
    }

    void requestCompleted() {
        activeRequests_--;
    }

    void requestFailed() {
        failedRequests_++;
    }

    struct Stats {
        int active;
        int total;
        int failed;
        double failureRate;
    };

    Stats getStats() const {
        int total = totalRequests_.load();
        int failed = failedRequests_.load();
        return {
                activeRequests_.load(),
                total,
                failed,
                total > 0 ? (double)failed / total : 0.0
        };
    }

    void reset() {
        activeRequests_.store(0);
        totalRequests_.store(0);
        failedRequests_.store(0);
    }

private:
    std::atomic<int> activeRequests_{0};
    std::atomic<int> totalRequests_{0};
    std::atomic<int> failedRequests_{0};
};

#endif // MODEL_POOL_H
