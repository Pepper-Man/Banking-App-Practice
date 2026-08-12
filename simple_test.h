#pragma once
#include "timer_util.h"
#include <iostream>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    void (*func)();
    bool is_main = false;
    bool is_long = false;
    bool is_database = false;
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
    TestRegistrar(std::string name, void (*func)(), bool isMain, bool isLong, bool isDatabase) {
        get_tests().push_back({ name, func, isMain, isLong, isDatabase });
    }
};

// Helper macros for string concatenation
#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

// Implementation macro: 'id' is expanded once so function & registrar names match
#define INTERNAL_REGISTER_TEST_IMPL(name, isMain, isLong, isDatabase, id) \
    static void TEST_CONCAT(test_func_, id)(); \
    static TestRegistrar TEST_CONCAT(registrar_, id)(name, &TEST_CONCAT(test_func_, id), isMain, isLong, isDatabase); \
    static void TEST_CONCAT(test_func_, id)()

// Evaluates __LINE__ before passing it into the implementation macro
#define INTERNAL_REGISTER_TEST(name, isMain, isLong, isDatabase) \
    INTERNAL_REGISTER_TEST_IMPL(name, isMain, isLong, isDatabase, __LINE__)

// User-facing macros
#define TEST_CASE(name) INTERNAL_REGISTER_TEST(name, true, false, false)
#define TEST_CASE_LONG(name) INTERNAL_REGISTER_TEST(name, false, true, false)
#define TEST_CASE_DATABASE(name) INTERNAL_REGISTER_TEST(name, false, false, true)

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

inline void run_bank_tests(bool runMainTests, bool runLongTests, bool runDbTests) {
    std::cout << "--- STARTING TEST SUITE ---" << std::endl;
    int passed_count = 0;
    const auto& tests = get_tests();
    std::size_t total_test_count = tests.size();
    for (const auto& test : tests) {
        if (test.is_main && !runMainTests) {
            total_test_count--;
            continue;
        }

        if (test.is_long && !runLongTests) {
            total_test_count--;
            continue;
        }

        if (test.is_database && !runDbTests) {
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