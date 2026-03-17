#include <iostream>
#include "simple_test.h"
#include "account.h"
#include "bank.h"
#include "data_handler.h"

// Define this to ensure all banking tests run
#define RUN_QUICK_TESTS
#define RUN_LONG_TESTS

#ifdef RUN_QUICK_TESTS

// --- ACCOUNT LEVEL TESTS ---

TEST_CASE("New account has zero balance") {
    bank_system::Account acc("user", "pass", "Name", 25);
    REQUIRE(acc.get_balance() == 0.0);
}

TEST_CASE("Account prevents negative deposit") {
    bank_system::Account acc("user", "pass", "Name", 25);
    acc.deposit(-100.0);
    REQUIRE(acc.get_balance() == 0.0);
}

TEST_CASE("Account prevents over-withdrawal") {
    bank_system::Account acc("user", "pass", "Name", 25);
    acc.deposit(50.0);
    bool success = acc.withdraw(60.0);
    REQUIRE(success == false);
    REQUIRE(acc.get_balance() == 50.0);
}

// --- BANK LEVEL TESTS ---

TEST_CASE("Bank creates multiple unique accounts") {
    bank_system::clear_saved_data();
    bank_system::Bank bank;
    REQUIRE(bank.create_account("user1", "p1", "Name 1", 20) == true);
    REQUIRE(bank.create_account("user2", "p2", "Name 2", 21) == true);
    REQUIRE(bank.user_exists("user1") == true);
    REQUIRE(bank.user_exists("user2") == true);
}

TEST_CASE("Bank blocks duplicate usernames") {
    bank_system::Bank bank;
    bank.create_account("admin", "p1", "Admin One", 40);
    bool second_attempt = bank.create_account("admin", "p2", "Admin Two", 45);
    REQUIRE(second_attempt == false);
}

TEST_CASE("Login fails for non-existent user") {
    bank_system::Bank bank;
    bank.create_account("real_user", "pass", "Real", 30);
    REQUIRE(bank.login("fake_user", "pass") == nullptr);
}

TEST_CASE("Login fails for correct user but wrong password") {
    bank_system::Bank bank;
    bank.create_account("bob", "secret", "Bob", 30);
    REQUIRE(bank.login("bob", "wrong_pass") == nullptr);
}

TEST_CASE("Operations on logged-in account persist in Bank") {
    bank_system::Bank bank;
    bank.create_account("saver", "pass123", "The Saver", 25);

    // Login and get pointer
    bank_system::Account* acc = bank.login("saver", "pass123");
    REQUIRE(acc != nullptr);

    // Deposit money via the pointer
    acc->deposit(500.0);
    REQUIRE(acc->get_balance() == 500.0);

    // Simulate "re-logging in" to ensure data is still there
    bank_system::Account* re_login = bank.login("saver", "pass123");
    REQUIRE(re_login->get_balance() == 500.0);
}

TEST_CASE("Multiple accounts maintain separate balances") {
    bank_system::Bank bank;
    bank.create_account("userA", "pA", "Person A", 20);
    bank_system::Account* accA = bank.login("userA", "pA");
    accA->deposit(100.69);

    bank.create_account("userB", "pB", "Person B", 30);
    bank_system::Account* accB = bank.login("userB", "pB");
    accB->deposit(250.42);

    REQUIRE(accA->get_balance() == 100.69);
    REQUIRE(accB->get_balance() == 250.42);
}

TEST_CASE("Saved data file is correctly cleared") {
    bank_system::Bank bank;
    bank.create_account("userA", "pA", "Person A", 20);
    bank_system::Account* accA = bank.login("userA", "pA");
    accA->deposit(1234.0);

    // Clear
    bank_system::clear_saved_data();

    // Test
    REQUIRE(bank_system::read_account_data().empty());
}

TEST_CASE("Saved user data can be loaded again") {
    // Ensure no stale data
    bank_system::clear_saved_data();

    // Make bank and user in scope
    {
        bank_system::Bank bank;
        bank.create_account("userZ", "password123", "Mr Zed", 25);
        bank_system::Account* accZ = bank.login("userZ", "password123");
        accZ->deposit(420.69);
    } // Bank destructor runs here, should save data to file

    // Open new bank, should load data file
    bank_system::Bank new_bank;
    // Log in to account that should exist
    bank_system::Account* new_accZ = new_bank.login("userZ", "password123");
    REQUIRE(new_accZ != nullptr);
    REQUIRE(new_accZ->get_balance() == 420.69);
}
#endif

#ifdef RUN_LONG_TESTS
TEST_CASE("Create, deposit to and save 1000 accounts") {
    bank_system::Bank bank;
    for (int i = 0; i < 1000; i++) {
        std::string i_str = std::to_string(i);
        bank.create_account("user" + i_str, "p" + i_str, "Person " + i_str, i);
        bank_system::Account* acc = bank.login("user" + i_str, "p" + i_str);
        acc->deposit(i);
        REQUIRE(acc->get_balance() == i);
    }
}

#endif

void get_going() {
#ifdef RUN_QUICK_TESTS
    run_all_tests();
    std::cout << "Press Enter to exit tests..." << std::endl;
    std::cin.get();
#else
    // Actual Banking App Interface
    bank_system::bank_ui();
    // ... your UI logic here ...
#endif
}