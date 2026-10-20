#include "lifecycle_controller.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using safety_tests::LifecycleController;
using safety_tests::LifecycleState;

namespace {
int failures = 0;

void expect(bool condition, const char* name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

void test_initial_state_and_idempotent_shutdown() {
    LifecycleController controller;
    expect(controller.state() == LifecycleState::Uninitialized, "initial state");
    expect(controller.shutdown([] {}) , "shutdown before init is safe");
    expect(controller.state() == LifecycleState::Uninitialized, "state after no-op shutdown");
}

void test_success_and_repeat_init() {
    LifecycleController controller;
    int acquired = 0;
    std::vector<LifecycleController::Step> steps{
        [&] { ++acquired; return true; },
        [&] { ++acquired; return true; }
    };
    std::vector<LifecycleController::Cleanup> rollback{[] {}, [] {}};
    expect(controller.initialize(steps, rollback), "initialization succeeds");
    expect(controller.state() == LifecycleState::Initialized, "initialized state");
    expect(controller.initialize(steps, rollback), "repeat initialization is idempotent");
    expect(acquired == 2, "repeat init does not reacquire");
    expect(controller.shutdown([] {}), "shutdown succeeds");
    expect(controller.state() == LifecycleState::Uninitialized, "shutdown state");
}

void test_failure_rolls_back_in_reverse_order() {
    LifecycleController controller;
    std::vector<int> events;
    std::vector<LifecycleController::Step> steps{
        [&] { events.push_back(1); return true; },
        [&] { events.push_back(2); return true; },
        [&] { events.push_back(3); return false; }
    };
    std::vector<LifecycleController::Cleanup> rollback{
        [&] { events.push_back(-1); },
        [&] { events.push_back(-2); },
        [&] { events.push_back(-3); }
    };
    expect(!controller.initialize(steps, rollback), "failed stage reports failure");
    expect(controller.state() == LifecycleState::Failed, "failed state recorded");
    const std::vector<int> expected{1, 2, 3, -2, -1};
    expect(events == expected, "only acquired stages roll back in reverse order");
}

void test_throwing_stage_and_rollback() {
    LifecycleController controller;
    int cleaned = 0;
    std::vector<LifecycleController::Step> steps{
        [] { return true; },
        []() -> bool { throw std::runtime_error("synthetic failure"); }
    };
    std::vector<LifecycleController::Cleanup> rollback{
        [&] { ++cleaned; },
        [] {}
    };
    expect(!controller.initialize(steps, rollback), "thrown stage becomes failure");
    expect(cleaned == 1, "prior stage is cleaned after exception");
    expect(controller.state() == LifecycleState::Failed, "exception leaves failed state");
}

void test_bad_rollback_plan_and_cleanup_failure() {
    LifecycleController controller;
    expect(!controller.initialize({[] { return true; }}, {}),
           "incomplete rollback plan rejected");
    expect(controller.state() == LifecycleState::Failed, "invalid plan recorded");
    expect(!controller.shutdown([] { throw std::runtime_error("cleanup failure"); }),
           "cleanup exception is reported");
    expect(controller.state() == LifecycleState::Failed, "cleanup failure remains visible");
}

void test_failed_stage_can_retry() {
    LifecycleController controller;
    expect(!controller.initialize({[] { return false; }}, {[] {}}),
           "first attempt fails");
    expect(controller.initialize({[] { return true; }}, {[] {}}),
           "retry from failed state succeeds");
    expect(controller.state() == LifecycleState::Initialized, "retry initialized state");
}
} // namespace

int main() {
    test_initial_state_and_idempotent_shutdown();
    test_success_and_repeat_init();
    test_failure_rolls_back_in_reverse_order();
    test_throwing_stage_and_rollback();
    test_bad_rollback_plan_and_cleanup_failure();
    test_failed_stage_can_retry();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All lifecycle controller tests passed\n";
    return EXIT_SUCCESS;
}
