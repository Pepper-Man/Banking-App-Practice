#include "account.h"
#include "bank.h"
#include "constants.h"
#include "data_handler.h"
#include "junior_account.h"
#include "savings_account.h"
#include "simple_test.h"
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Comment these out to disable certain tests
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
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "user1", "p1", "Name 1", 20) == true);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "user2", "p2", "Name 2", 21) == true);
    REQUIRE(bank.user_exists("user1") == true);
    REQUIRE(bank.user_exists("user2") == true);
}

TEST_CASE("Bank blocks duplicate usernames") {
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "admin", "p1", "Admin One", 40);
    bool second_attempt = bank.create_account(bank_system::AccountType::Standard, "admin", "p2", "Admin Two", 45);
    REQUIRE(second_attempt == false);
}

TEST_CASE("Login fails for non-existent user") {
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "real_user", "pass", "Real", 30);
    REQUIRE(bank.login("fake_user", "pass") == nullptr);
}

TEST_CASE("Login fails for correct user but wrong password") {
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "bob", "secret", "Bob", 30);
    REQUIRE(bank.login("bob", "wrong_pass") == nullptr);
}

TEST_CASE("Operations on logged-in account persist in Bank") {
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "saver", "pass123", "The Saver", 25);

    // Login and get pointer
    bank_system::Account* acc = bank.login("saver", "pass123");
    REQUIRE(acc != nullptr);

    // Deposit money via the pointer
    bank.deposit_to_account("saver", 500.0);
    REQUIRE(acc->get_balance() == 500.0);

    // Simulate "re-logging in" to ensure data is still there
    bank_system::Account* re_login = bank.login("saver", "pass123");
    REQUIRE(re_login->get_balance() == 500.0);
}

TEST_CASE("Multiple accounts maintain separate balances") {
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Person A", 20);
    bank_system::Account* accA = bank.login("userA", "pA");
    bank.deposit_to_account("userA", 100.69);

    bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Person B", 30);
    bank_system::Account* accB = bank.login("userB", "pB");
    bank.deposit_to_account("userB", 250.42);

    REQUIRE(accA->get_balance() == 100.69);
    REQUIRE(accB->get_balance() == 250.42);
}

TEST_CASE("Saved data file is correctly cleared") {
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Person A", 20);
    bank_system::Account* accA = bank.login("userA", "pA");
    bank.deposit_to_account("userA", 1234.0);

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
        bank.create_account(bank_system::AccountType::Standard, "userZ", "password123", "Mr Zed", 25);
        bank_system::Account* accZ = bank.login("userZ", "password123");
        bank.deposit_to_account("userZ", 420.69);
    } // Bank destructor runs here, should save data to file

    // Open new bank, should load data file
    bank_system::Bank new_bank;
    // Log in to account that should exist
    bank_system::Account* new_accZ = new_bank.login("userZ", "password123");
    REQUIRE(new_accZ != nullptr);
    REQUIRE(new_accZ->get_balance() == 420.69);
}

TEST_CASE("Transactions are cleared correctly") {
    bank_system::clear_transac_data();

    std::ifstream transac_file("transactions.log");
    REQUIRE(transac_file.is_open());
    REQUIRE(transac_file.peek() == EOF);
}

TEST_CASE("Transactions are logged correctly") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();

    {
        bank_system::Bank bank;
        bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
        bank.deposit_to_account("userA", 69.69);
    }
    
    std::ifstream transac_file("transactions.log");
    REQUIRE(transac_file.is_open());
    std::string first_line;
    std::getline(transac_file, first_line);
    REQUIRE(first_line == "Account: userA, Deposit of 69.69 - New balance: 69.69");
}

TEST_CASE("User cannot withdraw more than balance") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 29);
    bank.deposit_to_account("userA", 200.00);
    REQUIRE(bank.withdraw_from_account("userA", 200.01) == false);
    REQUIRE(bank.withdraw_from_account("userA", 200.00) == true);
}

TEST_CASE("User cannot deposit zero or negative amount") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 31);
    REQUIRE(bank.deposit_to_account("userA", 0) == false);
    REQUIRE(bank.deposit_to_account("userA", 0.00) == false);
    REQUIRE(bank.deposit_to_account("userA", 0.01) == true);
    REQUIRE(bank.deposit_to_account("userA", -200.00) == false);
    REQUIRE(bank.deposit_to_account("userA", -0) == false);
    REQUIRE(bank.deposit_to_account("userA", 10.00) == true);
    REQUIRE(bank.deposit_to_account("userA", -0.00) == false);
}

TEST_CASE("User can change password, then log in with new password") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 31);

    // Make sure change is successful
    REQUIRE(bank.request_password_change("userA", "pA", "pZ"));

    // Attempt login with old password, should fail
    bank_system::Account* acc = bank.login("userA", "pA");
    REQUIRE(acc == nullptr);

    // Attempt login with new password, should succeed
    acc = bank.login("userA", "pZ");
    REQUIRE(acc != nullptr);
}

TEST_CASE("Bank can transfer amounts between accounts successfully") {
    // Test vars
    std::string userA = "userA";
    std::string userB = "userB";
    double accA_start_amount = 100.0;
    double accB_start_amount = 100.0;
    double expected_total = accA_start_amount + accB_start_amount;

    // Set up bank and accounts
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, userA, "pA", "Mr A", 25);
    bank.create_account(bank_system::AccountType::Standard, userB, "pB", "Mr B", 30);
    bank.deposit_to_account(userA, accA_start_amount);
    bank.deposit_to_account(userB, accB_start_amount);

    // Transfer should not throw
    bool caught_exception = false;
    try {
        bank.transfer(userA, userB, accA_start_amount / 2.0);
    }
    catch (const std::exception& e) {
        caught_exception = true;
    }
    REQUIRE(caught_exception == false);

    // Check that account values total is still the same
    bank_system::Account* accA = bank.login(userA, "pA");
    bank_system::Account* accB = bank.login(userB, "pB");
    REQUIRE(accA->get_balance() + accB->get_balance() == expected_total);
}

TEST_CASE("Invalid transfer should fail") {
    // Test vars
    std::string userA = "userA";
    std::string userB = "userB";
    double accA_start_amount = 100.0;
    double accB_start_amount = 100.0;
    double expected_total = accA_start_amount + accB_start_amount;

    // Set up bank and accounts
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, userA, "pA", "Mr A", 25);
    bank.create_account(bank_system::AccountType::Standard, userB, "pB", "Mr B", 30);
    bank.deposit_to_account(userA, accA_start_amount);
    bank.deposit_to_account(userB, accB_start_amount);

    // Transfer should throw
    bool caught_exception = false;
    try {
        bank.transfer(userA, userB, accA_start_amount * 2.0); // Too much!
    }
    catch (const std::exception& e) {
        caught_exception = true;
    }
    REQUIRE(caught_exception == true);

    // Check that account values have not been altered
    bank_system::Account* accA = bank.login(userA, "pA");
    bank_system::Account* accB = bank.login(userB, "pB");
    REQUIRE(accA->get_balance() == accA_start_amount && accB->get_balance() == accB_start_amount);
}

TEST_CASE("Bank returns transaction history of account correctly") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 25);
    bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 30);
    bank.deposit_to_account("userA", 100.0);
    bank.deposit_to_account("userA", 50.0);
    bank.deposit_to_account("userB", 120.0);
    bank.deposit_to_account("userB", 150.0);
    bank.deposit_to_account("userB", 30.0);
    bank.deposit_to_account("userA", 50.0);
    bank.deposit_to_account("userB", 150.0);

    bank_system::Account* accA = bank.login("userA", "pA");
    bank_system::Account* accB = bank.login("userB", "pB");

    REQUIRE(accA->get_history().size() == 3);
    REQUIRE(accB->get_history().size() == 4);
}

TEST_CASE("Savings account withdraw limit works correctly") {
    double withdraw_limit = 100.0;

    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Savings, "userA", "pA", "Mr A", 25, withdraw_limit);
    bank.deposit_to_account("userA", 100.0);
    REQUIRE(bank.withdraw_from_account("userA", 500.0) == false);
    bank_system::Account* base_acc = bank.login("userA", "pA");
    bank_system::SavingsAccount* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(base_acc);
    REQUIRE(savings_acc->get_balance() == 100.0);
    REQUIRE(savings_acc->withdraw(101.0) == false);
}

TEST_CASE("Junior account cannot exceed balance limit") {
    double balance_limit = 500.00;
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Junior, "userA", "pA", "Mr A", 25, 0.0, balance_limit);
    REQUIRE(bank.deposit_to_account("userA", 499.00) == true);
    REQUIRE(bank.deposit_to_account("userA", 1.00) == true);
    REQUIRE(bank.deposit_to_account("userA", 10.00) == false);
    REQUIRE(bank.deposit_to_account("userA", 0.01) == false);
    REQUIRE(bank.withdraw_from_account("userA", 500.00) == true);
}

TEST_CASE("Account type is preserved between bank save and load") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    
    // Create and save accounts of different types
    {
        bank_system::Bank bank;
        bank.create_account(bank_system::AccountType::Standard, "standardA", "pA", "Mr Standard", 30);
        bank.create_account(bank_system::AccountType::Savings, "savingsB", "pB", "Mr Savings", 20);
        bank.create_account(bank_system::AccountType::Junior, "juniorC", "pC", "Mr Junior", 15);
    }

    bank_system::Bank new_bank;
    bank_system::Account* base_standard = new_bank.login("standardA", "pA");
    bank_system::Account* base_savings = new_bank.login("savingsB", "pB");
    bank_system::Account* base_junior = new_bank.login("juniorC", "pC");

    REQUIRE(base_standard != nullptr);
    REQUIRE(base_savings != nullptr);
    REQUIRE(base_junior != nullptr);

    REQUIRE(base_standard->get_type() == bank_system::AccountType::Standard);
    REQUIRE(base_savings->get_type() == bank_system::AccountType::Savings);
    REQUIRE(base_junior->get_type() == bank_system::AccountType::Junior);
}

TEST_CASE("Account type-specific functions and values are available after save and load") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();

    // Create and save accounts of different types
    {
        bank_system::Bank bank;
        bank.create_account(bank_system::AccountType::Savings, "savingsA", "pA", "Mr Savings", 20, 100.0, 0.0);
        bank.create_account(bank_system::AccountType::Junior, "juniorB", "pB", "Mr Junior", 15, 0.0, 200.0);
    }

    bank_system::Bank new_bank;
    bank_system::Account* base_savings = new_bank.login("savingsA", "pA");
    bank_system::Account* base_junior = new_bank.login("juniorB", "pB");
    REQUIRE(base_savings != nullptr);
    REQUIRE(base_junior != nullptr);
    bank_system::SavingsAccount* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(base_savings);
    bank_system::JuniorAccount* junior_acc = dynamic_cast<bank_system::JuniorAccount*>(base_junior);

    REQUIRE(savings_acc != nullptr);
    REQUIRE(junior_acc != nullptr);

    // These getter functions are unique to their respective account child type
    REQUIRE(savings_acc->get_withdraw_limit() == 100.0);
    REQUIRE(junior_acc->get_balance_limit() == 200.0);
}
#endif

// More complex tests (~>10ms each)
#ifdef RUN_LONG_TESTS
TEST_CASE("Create, deposit to and save 1000 accounts") {
    bank_system::Bank bank;
    for (int i = 0; i < 1000; i++) {
        std::string i_str = std::to_string(i);
        bank.create_account(bank_system::AccountType::Standard, "user" + i_str, "p" + i_str, "Person " + i_str, i);
        bank_system::Account* acc = bank.login("user" + i_str, "p" + i_str);
        bank.deposit_to_account("user" + i_str, i);
        REQUIRE(acc->get_balance() == i);
    }
}

TEST_CASE("Bank applies interest to all accounts correctly") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();

    // Test vars
    const int num_accounts = 1000;
    const double initial_deposit = 100.00;
    const double interest_rate = 0.05; // 5% interest
    const double expected_balance = 105.00;

    {
        bank_system::Bank bank;

        // Create 1000 accounts with money
        for (int i = 0; i < num_accounts; i++) {
            std::string user = "user" + std::to_string(i);
            bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
            bank.deposit_to_account(user, initial_deposit);
        }

        // Apply interest to all
        bank.apply_monthly_interest(interest_rate);

        // Check a few sample users
        std::vector<std::string> test_users = { "user0", "user500", "user999" };
        for (const std::string& username : test_users) {
            bank_system::Account* acc = bank.login(username, "pass123");
            REQUIRE(acc != nullptr);
            REQUIRE(acc->get_balance() == expected_balance);
        }
    }

    // Check logging
    std::ifstream log_file("transactions.log");
    REQUIRE(log_file.is_open());

    int line_count = 0;
    std::string dummy;
    while (std::getline(log_file, dummy)) {
        line_count++;
    }

    // Should be 1000 deposit log lines and 1000 interest log lines
    REQUIRE(line_count == 2000);
}

TEST_CASE("Interest sweep mainatins data integrity on failure") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();

    // Test vars
    const int num_accounts = 1000;
    const double initial_deposit = 100.00;
    const double interest_rate = 500.0; // Intentionally bad interest value

    bank_system::Bank bank;
    // Create 1000 accounts with money
    for (int i = 0; i < num_accounts; i++) {
        std::string user = "user" + std::to_string(i);
        bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
        bank.deposit_to_account(user, initial_deposit);
    }

    bool caught_exception = false;
    try {
        bank.apply_monthly_interest(interest_rate);
    }
    catch (const std::exception& e) {
        caught_exception = true;
    }

    REQUIRE(caught_exception == true);
    // Now check that all balances are still 100.0
    for (int i = 0; i < num_accounts; i++) {
        std::string user = "user" + std::to_string(i);
        bank_system::Account* acc = bank.login(user, "pass123");
        REQUIRE(acc != nullptr);
        REQUIRE(acc->get_balance() == initial_deposit);
    }
}

TEST_CASE("Bank returns account history correctly from saved file and large amount of transactions") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();

    int num_accounts = 1000;

    {
        bank_system::Bank bank;
        // Create 1000 accounts with money
        for (int i = 0; i < num_accounts; i++) {
            std::string user = "user" + std::to_string(i);
            bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
            bank.deposit_to_account(user, 100.00);
            bank.withdraw_from_account(user, 10.00);
        }

        // Bank closes, saves 2000 transactions to file
    }

    // Open new bank, read acc history
    bank_system::Bank bank;
    bank_system::Account* acc500 = bank.login("user500", "pass123");
    REQUIRE(acc500 != nullptr);
    REQUIRE(acc500->get_history().size() == 2);
    bank.deposit_to_account("user500", 12.00);
    REQUIRE(acc500->get_history().size() == 3);
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