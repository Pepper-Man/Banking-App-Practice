#pragma once
#include "timer_util.h"
#include <iostream>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    void (*func)();
    bool is_long = false;
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
    TestRegistrar(std::string name, void (*func)(), bool isLong) {
        get_tests().push_back({ name, func, isLong });
    }
};

// Helper macros for string concatenation
#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

// Implementation macro: 'id' is expanded once so function & registrar names match
#define INTERNAL_REGISTER_TEST_IMPL(name, isLong, id) \
    static void TEST_CONCAT(test_func_, id)(); \
    static TestRegistrar TEST_CONCAT(registrar_, id)(name, &TEST_CONCAT(test_func_, id), isLong); \
    static void TEST_CONCAT(test_func_, id)()

// Evaluates __LINE__ before passing it into the implementation macro
#define INTERNAL_REGISTER_TEST(name, isLong) \
    INTERNAL_REGISTER_TEST_IMPL(name, isLong, __LINE__)

// User-facing macros
#define TEST_CASE(name) INTERNAL_REGISTER_TEST(name, false)
#define TEST_CASE_LONG(name) INTERNAL_REGISTER_TEST(name, true)

#define REQUIRE(cond) \
    if (!(cond)) { \
        std::cout << "  [FAIL] " << #cond << " at line " << __LINE__ << std::endl; \
        get_current_test_failed() = true; \
        return; \
    }

#define REQUIRE(cond) \
    if (!(cond)) { \
        std::cout << "  [FAIL] " << #cond << " at line " << __LINE__ << std::endl; \
        get_current_test_failed() = true; \
        return; \
    }

inline void run_bank_tests(bool runLongTests) {
    std::cout << "--- STARTING TEST SUITE ---" << std::endl;
    int passed_count = 0;
    const auto& tests = get_tests();
    std::size_t total_test_count = tests.size();
    for (const auto& test : tests) {
        if (test.is_long && !runLongTests) {
            total_test_count--;
            continue;
        }

        TestTimer t("Above test");
        get_current_test_failed() = false;
        test.func();
        if (!get_current_test_failed()) {
            passed_count++;
            std::cout << "[PASS] " << test.name << std::endl;
        }
    }
    std::cout << "\nResults: " << passed_count << "/" << total_test_count << " passed." << std::endl;
    std::cout << "---------------------------\n" << std::endl;
}