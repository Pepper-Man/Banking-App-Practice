#pragma once
#include "timer_util.h"
#include <iostream>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    void (*func)();
};

inline bool& get_current_test_failed() {
    static bool failed = false;
    return failed;
}

inline std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

struct TestRegistrar {
    TestRegistrar(std::string name, void (*func)()) {
        get_tests().push_back({ name, func });
    }
};

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)
#define TEST_CASE_UNIQUE(name, count) \
    void TEST_CONCAT(test_func_, count)(); \
    static TestRegistrar TEST_CONCAT(registrar_, count)(name, TEST_CONCAT(test_func_, count)); \
    void TEST_CONCAT(test_func_, count)()

#define TEST_CASE(name) TEST_CASE_UNIQUE(name, __COUNTER__)

#define REQUIRE(cond) \
    if (!(cond)) { \
        std::cout << "  [FAIL] " << #cond << " at line " << __LINE__ << std::endl; \
        get_current_test_failed() = true; \
        return; \
    }

inline void run_all_tests() {
    std::cout << "--- STARTING TEST SUITE ---" << std::endl;
    int passed_count = 0;
    const auto& tests = get_tests();
    for (const auto& test : tests) {
        TestTimer t("Above test");
        get_current_test_failed() = false;
        test.func();
        if (!get_current_test_failed()) {
            passed_count++;
            std::cout << "[PASS] " << test.name << std::endl;
        }
    }
    std::cout << "\nResults: " << passed_count << "/" << tests.size() << " passed." << std::endl;
    std::cout << "---------------------------\n" << std::endl;
}