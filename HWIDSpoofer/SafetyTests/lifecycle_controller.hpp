#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

namespace safety_tests {

enum class LifecycleState {
    Uninitialized,
    Initializing,
    Initialized,
    Stopping,
    Failed
};

// A portable, standalone reference controller for testing lifecycle policy.
// It deliberately has no dependency on Windows, MinHook, or the production DLL.
class LifecycleController {
public:
    using Step = std::function<bool()>;
    using Cleanup = std::function<void()>;

    LifecycleState state() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    bool initialize(const std::vector<Step>& acquire,
                    const std::vector<Cleanup>& rollback) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == LifecycleState::Initialized) {
            return true;
        }
        if (state_ != LifecycleState::Uninitialized &&
            state_ != LifecycleState::Failed) {
            return false;
        }
        if (rollback.size() < acquire.size()) {
            state_ = LifecycleState::Failed;
            return false;
        }

        state_ = LifecycleState::Initializing;
        std::size_t acquired = 0;
        for (std::size_t i = 0; i < acquire.size(); ++i) {
            bool succeeded = false;
            try {
                succeeded = acquire[i] && acquire[i]();
            } catch (...) {
                succeeded = false;
            }
            if (!succeeded) {
                while (acquired > 0) {
                    --acquired;
                    try {
                        if (rollback[acquired]) rollback[acquired]();
                    } catch (...) {
                        // Continue best-effort rollback of prior stages.
                    }
                }
                state_ = LifecycleState::Failed;
                return false;
            }
            ++acquired;
        }
        state_ = LifecycleState::Initialized;
        return true;
    }

    bool shutdown(const Cleanup& cleanup) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == LifecycleState::Uninitialized) {
            return true;
        }
        if (state_ != LifecycleState::Initialized &&
            state_ != LifecycleState::Failed) {
            return false;
        }

        state_ = LifecycleState::Stopping;
        bool succeeded = true;
        try {
            if (cleanup) cleanup();
        } catch (...) {
            succeeded = false;
        }
        state_ = succeeded ? LifecycleState::Uninitialized
                           : LifecycleState::Failed;
        return succeeded;
    }

private:
    mutable std::mutex mutex_;
    LifecycleState state_ = LifecycleState::Uninitialized;
};

} // namespace safety_tests
