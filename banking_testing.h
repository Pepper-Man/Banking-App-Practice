#include "account.h"
#include "bank.h"
#include "constants.h"
#include "database.h"
#include "data_handler.h"
#include "junior_account.h"
#include "savings_account.h"
#include "simple_test.h"
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <string>
#include <vector>
#include <cstdlib>

// Creates an in-memory database for testing purposes
struct TestBankContext {
    SQLite::Database db;
    bank_system::Bank bank;

    TestBankContext(const std::string& db_file = ":memory:")
        : db(db_file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE), bank(db) 
    {
        if (db_file != ":memory:") {
            // Make SQLite hold writes in RAM cache during tests
            db.exec("PRAGMA synchronous = OFF;");
            db.exec("PRAGMA journal_mode = MEMORY;");
        }
    }
};

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
    REQUIRE(acc.withdraw(60.0) == bank_system::TransactionStatus::InsufficientFunds);
    REQUIRE(acc.get_balance() == 50.0);
}

// --- BANK LEVEL TESTS ---

TEST_CASE("Bank creates multiple unique accounts") {
    TestBankContext tbc;
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "user1", "p1", "Name 1", 20) == true);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "user2", "p2", "Name 2", 21) == true);
    REQUIRE(tbc.bank.user_exists("user1") == true);
    REQUIRE(tbc.bank.user_exists("user2") == true);
}

TEST_CASE("Bank blocks duplicate usernames") {
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "admin", "p1", "Admin One", 40);
    bool second_attempt = tbc.bank.create_account(bank_system::AccountType::Standard, "admin", "p2", "Admin Two", 45);
    REQUIRE(second_attempt == false);
}

TEST_CASE("Login fails for non-existent user") {
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "real_user", "pass", "Real", 30);
    REQUIRE(tbc.bank.login("fake_user", "pass") == nullptr);
}

TEST_CASE("Login fails for correct user but wrong password") {
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "bob", "secret", "Bob", 30);
    REQUIRE(tbc.bank.login("bob", "wrong_pass") == nullptr);
}

TEST_CASE("Operations on logged-in account persist in Bank") {
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "saver", "pass123", "The Saver", 25);

    // Login and get pointer
    bank_system::Account* acc = tbc.bank.login("saver", "pass123");
    REQUIRE(acc != nullptr);

    // Deposit money via the pointer
    tbc.bank.deposit_to_account("saver", 500.0);
    REQUIRE(acc->get_balance() == 500.0);

    // Simulate "re-logging in" to ensure data is still there
    bank_system::Account* re_login = tbc.bank.login("saver", "pass123");
    REQUIRE(re_login->get_balance() == 500.0);
}

TEST_CASE("Multiple accounts maintain separate balances") {
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Person A", 20);
    bank_system::Account* accA = tbc.bank.login("userA", "pA");
    tbc.bank.deposit_to_account("userA", 100.69);

    tbc.bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Person B", 30);
    bank_system::Account* accB = tbc.bank.login("userB", "pB");
    tbc.bank.deposit_to_account("userB", 250.42);

    REQUIRE(accA->get_balance() == 100.69);
    REQUIRE(accB->get_balance() == 250.42);
}

TEST_CASE("Saved data file is correctly cleared") {
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Person A", 20);
    bank_system::Account* accA = tbc.bank.login("userA", "pA");
    tbc.bank.deposit_to_account("userA", 1234.0);

    tbc.bank.save();

    // Test
    bank_system::clear_database(tbc.db, tbc.bank);
    REQUIRE(bank_system::read_account_data(tbc.db).empty());
}

TEST_CASE("Saved user data can be loaded again") {
    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str()); // Clear before starting

    // Make bank and user in scope
    {
        TestBankContext tbc(test_db);
        tbc.bank.create_account(bank_system::AccountType::Standard, "userZ", "password123", "Mr Zed", 25);
        bank_system::Account* accZ = tbc.bank.login("userZ", "password123");
        tbc.bank.deposit_to_account("userZ", 420.69);
        tbc.bank.save();
    }

    {
        // Open new bank, should load data file
        TestBankContext new_tbc(test_db);
        REQUIRE(new_tbc.bank.user_exists("userZ") == true);
        // Log in to account that should exist
        bank_system::Account* new_accZ = new_tbc.bank.login("userZ", "password123");
        REQUIRE(new_accZ != nullptr);
        REQUIRE(new_accZ->get_balance() == 420.69);
    }
    
    std::remove(test_db.c_str());
}

TEST_CASE("Transactions are cleared correctly") {
    bank_system::clear_transac_data();

    std::ifstream transac_file("transactions.log");
    REQUIRE(transac_file.is_open());
    REQUIRE(transac_file.peek() == EOF);
}

TEST_CASE("Transactions are logged correctly") {
    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str());

    {
        TestBankContext tbc(test_db);
        tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
        tbc.bank.deposit_to_account("userA", 69.69);
    } // Exiting scope triggers bank destructor, which flushes transaction buffer to database
    
    SQLite::Database db(test_db, SQLite::OPEN_READONLY);
    SQLite::Statement query(db, "SELECT username, type, amount, balance FROM transactions WHERE username = ?");
    query.bind(1, "userA");

    REQUIRE(query.executeStep());
    REQUIRE(std::string(query.getColumn("username").getText()) == "userA");
    REQUIRE(std::string(query.getColumn("type").getText()) == "Deposit");
    REQUIRE(query.getColumn("amount").getDouble() == 69.69);
    REQUIRE(query.getColumn("balance").getDouble() == 69.69);

    std::remove(test_db.c_str());
}

TEST_CASE("User cannot withdraw more than balance") {
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 29);
    tbc.bank.deposit_to_account("userA", 200.00);
    REQUIRE(tbc.bank.withdraw_from_account("userA", 200.01) == bank_system::TransactionStatus::InsufficientFunds);
    REQUIRE(tbc.bank.withdraw_from_account("userA", 200.00) == bank_system::TransactionStatus::Success);
}

TEST_CASE("User cannot deposit zero or negative amount") {
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 31);
    REQUIRE(tbc.bank.deposit_to_account("userA", 0) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(tbc.bank.deposit_to_account("userA", 0.00) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(tbc.bank.deposit_to_account("userA", 0.01) == bank_system::TransactionStatus::Success);
    REQUIRE(tbc.bank.deposit_to_account("userA", -200.00) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(tbc.bank.deposit_to_account("userA", -0) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(tbc.bank.deposit_to_account("userA", 10.00) == bank_system::TransactionStatus::Success);
    REQUIRE(tbc.bank.deposit_to_account("userA", -0.00) == bank_system::TransactionStatus::InvalidAmount);
}

TEST_CASE("User can change password, then log in with new password") {
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 31);

    // Make sure change is successful
    REQUIRE(tbc.bank.request_password_change("userA", "pA", "pZ"));

    // Attempt login with old password, should fail
    bank_system::Account* acc = tbc.bank.login("userA", "pA");
    REQUIRE(acc == nullptr);

    // Attempt login with new password, should succeed
    acc = tbc.bank.login("userA", "pZ");
    REQUIRE(acc != nullptr);
}

TEST_CASE("Bank can transfer amounts between accounts successfully") {
    // Test vars
    std::string userA = "userA";
    std::string userB = "userB";
    double accA_start_amount = 100.0;
    double accB_start_amount = 100.0;
    double expected_total = accA_start_amount + accB_start_amount - bank_system::TransferFee;

    // Set up bank and accounts
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, userA, "pA", "Mr A", 25);
    tbc.bank.create_account(bank_system::AccountType::Standard, userB, "pB", "Mr B", 30);
    tbc.bank.deposit_to_account(userA, accA_start_amount);
    tbc.bank.deposit_to_account(userB, accB_start_amount);

    // Transfer should not throw
    bool caught_exception = false;
    try {
        tbc.bank.transfer(userA, userB, accA_start_amount / 2.0);
    }
    catch (const std::exception& e) {
        (void)e;
        caught_exception = true;
    }
    REQUIRE(caught_exception == false);

    // Check that account values total is still the same
    bank_system::Account* accA = tbc.bank.login(userA, "pA");
    bank_system::Account* accB = tbc.bank.login(userB, "pB");
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
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, userA, "pA", "Mr A", 25);
    tbc.bank.create_account(bank_system::AccountType::Standard, userB, "pB", "Mr B", 30);
    tbc.bank.deposit_to_account(userA, accA_start_amount);
    tbc.bank.deposit_to_account(userB, accB_start_amount);

    // Transfer should fail
    bool caught_exception = false;
    REQUIRE(tbc.bank.transfer(userA, userB, accA_start_amount * 2.0) == bank_system::TransactionStatus::InsufficientFunds); // Too much!

    // Check that account values have not been altered
    bank_system::Account* accA = tbc.bank.login(userA, "pA");
    bank_system::Account* accB = tbc.bank.login(userB, "pB");
    REQUIRE(accA->get_balance() == accA_start_amount && accB->get_balance() == accB_start_amount);
}

TEST_CASE("Bank returns transaction history of account correctly") {
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 25);
    tbc.bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 30);
    tbc.bank.deposit_to_account("userA", 100.0);
    tbc.bank.deposit_to_account("userA", 50.0);
    tbc.bank.deposit_to_account("userB", 120.0);
    tbc.bank.deposit_to_account("userB", 150.0);
    tbc.bank.deposit_to_account("userB", 30.0);
    tbc.bank.deposit_to_account("userA", 50.0);
    tbc.bank.deposit_to_account("userB", 150.0);

    bank_system::Account* accA = tbc.bank.login("userA", "pA");
    bank_system::Account* accB = tbc.bank.login("userB", "pB");

    REQUIRE(accA->get_history().size() == 3);
    REQUIRE(accB->get_history().size() == 4);
}

TEST_CASE("Savings account withdraw limit works correctly") {
    double withdraw_limit = 100.0;
    bank_system::clear_transac_data();
    TestBankContext tbc;

    tbc.bank.create_account(bank_system::AccountType::Savings, "userA", "pA", "Mr A", 25, withdraw_limit);
    tbc.bank.deposit_to_account("userA", 100.0);
    REQUIRE(tbc.bank.withdraw_from_account("userA", 500.0) == bank_system::TransactionStatus::ExceedsAccountLimit);
    bank_system::Account* base_acc = tbc.bank.login("userA", "pA");
    bank_system::SavingsAccount* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(base_acc);
    REQUIRE(savings_acc->get_balance() == 100.0);
    REQUIRE(savings_acc->withdraw(101.0) == bank_system::TransactionStatus::ExceedsAccountLimit);
}

TEST_CASE("Junior account cannot exceed balance limit") {
    double balance_limit = 500.00;
    bank_system::clear_transac_data();
    TestBankContext tbc;

    tbc.bank.create_account(bank_system::AccountType::Junior, "userA", "pA", "Mr A", 25, 0.0, balance_limit);
    REQUIRE(tbc.bank.deposit_to_account("userA", 499.00) == bank_system::TransactionStatus::Success);
    REQUIRE(tbc.bank.deposit_to_account("userA", 1.00) == bank_system::TransactionStatus::Success);
    REQUIRE(tbc.bank.deposit_to_account("userA", 10.00) == bank_system::TransactionStatus::ExceedsAccountLimit);
    REQUIRE(tbc.bank.deposit_to_account("userA", 0.01) == bank_system::TransactionStatus::ExceedsAccountLimit);
    REQUIRE(tbc.bank.withdraw_from_account("userA", 500.00) == bank_system::TransactionStatus::Success);
}

TEST_CASE("Account type is preserved between bank save and load") {
    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str());
    
    // Create and save accounts of different types
    {
        TestBankContext tbc(test_db);
        tbc.bank.create_account(bank_system::AccountType::Standard, "standardA", "pA", "Mr Standard", 30);
        tbc.bank.create_account(bank_system::AccountType::Savings, "savingsB", "pB", "Mr Savings", 20);
        tbc.bank.create_account(bank_system::AccountType::Junior, "juniorC", "pC", "Mr Junior", 15);
    }

    {
        TestBankContext new_tbc(test_db);
        bank_system::Account* base_standard = new_tbc.bank.login("standardA", "pA");
        bank_system::Account* base_savings = new_tbc.bank.login("savingsB", "pB");
        bank_system::Account* base_junior = new_tbc.bank.login("juniorC", "pC");

        REQUIRE(base_standard != nullptr);
        REQUIRE(base_savings != nullptr);
        REQUIRE(base_junior != nullptr);

        REQUIRE(base_standard->get_type() == bank_system::AccountType::Standard);
        REQUIRE(base_savings->get_type() == bank_system::AccountType::Savings);
        REQUIRE(base_junior->get_type() == bank_system::AccountType::Junior);
    }
    
    std::remove(test_db.c_str());
}

TEST_CASE("Account type-specific functions and values are available after save and load") {
    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str());

    // Create and save accounts of different types
    {
        TestBankContext tbc(test_db);
        tbc.bank.create_account(bank_system::AccountType::Savings, "savingsA", "pA", "Mr Savings", 20, 100.0, 0.0);
        tbc.bank.create_account(bank_system::AccountType::Junior, "juniorB", "pB", "Mr Junior", 15, 0.0, 200.0);
    }

    {
        TestBankContext new_tbc(test_db);
        bank_system::Account* base_savings = new_tbc.bank.login("savingsA", "pA");
        bank_system::Account* base_junior = new_tbc.bank.login("juniorB", "pB");
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

    std::remove(test_db.c_str());
}

TEST_CASE("Bank audit function to return accounts by type works correctly") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    // Create two standard accounts
    tbc.bank.create_account(bank_system::AccountType::Standard, "standardA", "pA", "Mr StandardA", 30);
    tbc.bank.create_account(bank_system::AccountType::Standard, "standardB", "pB", "Mr StandardB", 35);

    // Create three savings accounts
    tbc.bank.create_account(bank_system::AccountType::Savings, "savingsA", "pA", "Mr SavingsA", 25, 100.0, 0.0);
    tbc.bank.create_account(bank_system::AccountType::Savings, "savingsB", "pB", "Mr SavingsB", 26, 110.0, 0.0);
    tbc.bank.create_account(bank_system::AccountType::Savings, "savingsC", "pC", "Mr SavingsC", 27, 120.0, 0.0);

    // Create four junior accounts
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorA", "pA", "Master JuniorA", 13);
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorB", "pB", "Master JuniorB", 14);
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorC", "pC", "Master JuniorC", 15);
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorD", "pD", "Master JuniorD", 16);

    // Test vector sizes, should be same as amount of accounts of type
    REQUIRE(tbc.bank.get_accounts_by_type(bank_system::AccountType::Standard).size() == 2);
    REQUIRE(tbc.bank.get_accounts_by_type(bank_system::AccountType::Savings).size() == 3);
    REQUIRE(tbc.bank.get_accounts_by_type(bank_system::AccountType::Junior).size() == 4);
}

TEST_CASE("Bank audit function to check at-risk junior accounts works correctly") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    // Create two not-at-risk juniors
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorA", "pA", "Master JuniorA", 13, 0.0, 100.0);
    tbc.bank.deposit_to_account("juniorA", 50.00);
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorB", "pB", "Master JuniorB", 14, 0.0, 150.0);
    tbc.bank.deposit_to_account("juniorB", 100.00);

    // Create two at-risk juniors
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorC", "pC", "Master JuniorC", 15, 0.0, 100.0);
    tbc.bank.deposit_to_account("juniorC", 95.00);
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorD", "pD", "Master JuniorD", 16, 0.0, 150.0);
    tbc.bank.deposit_to_account("juniorD", 140.10);

    REQUIRE(tbc.bank.get_at_risk_juniors(10.00).size() == 2);
}

TEST_CASE("Bank audit function to find highest value user") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    // Create some users
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    tbc.bank.deposit_to_account("userA", 15000000.00);
    tbc.bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 31);
    tbc.bank.deposit_to_account("userB", 20000000.00);
    tbc.bank.create_account(bank_system::AccountType::Standard, "userC", "pC", "Mr C", 32);
    tbc.bank.deposit_to_account("userC", 5000000.00);

    REQUIRE(tbc.bank.get_highest_balance_holder().first == "userB");
    REQUIRE(tbc.bank.get_highest_balance_holder().second == 20000000.00);
}

TEST_CASE("Bank total balance is correct") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    // Create some users
    tbc.bank.create_account(bank_system::AccountType::Standard, "standardA", "pA", "Mr A", 30);
    tbc.bank.deposit_to_account("standardA", 15000000.00);
    tbc.bank.create_account(bank_system::AccountType::Standard, "standardB", "pB", "Mr B", 31);
    tbc.bank.deposit_to_account("standardB", 20000000.00);
    tbc.bank.create_account(bank_system::AccountType::Standard, "standardC", "pC", "Mr C", 32);
    tbc.bank.deposit_to_account("standardC", 5000000.00);
    tbc.bank.create_account(bank_system::AccountType::Savings, "savingsA", "pA", "Mr A", 30, 100.0, 0.0);
    tbc.bank.deposit_to_account("savingsA", 15000000.00);
    tbc.bank.create_account(bank_system::AccountType::Savings, "savingsB", "pB", "Mr B", 31, 100.0, 0.0);
    tbc.bank.deposit_to_account("savingsB", 20000000.00);
    tbc.bank.create_account(bank_system::AccountType::Junior, "juniorA", "pA", "Master C", 15, 0.0, 100.0);
    tbc.bank.deposit_to_account("juniorA", 50.00);

    REQUIRE(tbc.bank.get_total_bank_balance() == 75000050);
}

TEST_CASE("Bank can close accounts of all types") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    tbc.bank.create_account(bank_system::AccountType::Standard, "standard", "pA", "Mr A", 30);
    tbc.bank.create_account(bank_system::AccountType::Savings, "savings", "pB", "Mr B", 31, 100.0, 0.0);
    tbc.bank.create_account(bank_system::AccountType::Junior, "junior", "pC", "Mr C", 32, 0.0, 100.0);

    // close_account returns true if successful
    REQUIRE(tbc.bank.close_account("standard"));
    REQUIRE(tbc.bank.close_account("savings"));
    REQUIRE(tbc.bank.close_account("junior"));

    // Shouldn't be able to log in to deleted accounts
    REQUIRE(tbc.bank.login("standard", "pA") == nullptr);
    REQUIRE(tbc.bank.login("savings", "pB") == nullptr);
    REQUIRE(tbc.bank.login("junior", "pC") == nullptr);
}

TEST_CASE("Bank applies correct interest amount to different account types") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    // Create an account of each type
    tbc.bank.create_account(bank_system::AccountType::Standard, "standard", "pA", "Mr A", 30);
    tbc.bank.create_account(bank_system::AccountType::Savings, "savings", "pB", "Mr B", 31, 100.0, 0.0);
    tbc.bank.create_account(bank_system::AccountType::Junior, "junior", "pC", "Mr C", 32, 0.0, 300.0);
    tbc.bank.deposit_to_account("standard", 100.0);
    tbc.bank.deposit_to_account("savings", 100.0);
    tbc.bank.deposit_to_account("junior", 100.0);

    // Apply interest
    tbc.bank.apply_monthly_interest(0.10); // 10% interest

    // Log in to accounts
    bank_system::Account* standard_acc = tbc.bank.login("standard", "pA");
    bank_system::SavingsAccount* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(tbc.bank.login("savings", "pB"));
    bank_system::JuniorAccount* junior_acc = dynamic_cast<bank_system::JuniorAccount*>(tbc.bank.login("junior", "pC"));
    REQUIRE(standard_acc != nullptr);
    REQUIRE(savings_acc != nullptr);
    REQUIRE(junior_acc != nullptr);

    // Check balances
    REQUIRE(standard_acc->get_balance() == 110.0);
    REQUIRE(savings_acc->get_balance() == 120.0);
    REQUIRE(junior_acc->get_balance() == 112.0);
}

TEST_CASE("Account usernames are correctly validated") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "username123TEST", "pA", "Mr Test", 30) == true);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "user,name", "pA", "Mr Test", 30) == false);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "user name", "pA", "Mr Test", 30) == false);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, " username", "pA", "Mr Test", 30) == false);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "username ", "pA", "Mr Test", 30) == false);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "us3rn4m3", "pA", "Mr Test", 30) == true);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "username!", "pA", "Mr Test", 30) == false);
    REQUIRE(tbc.bank.create_account(bank_system::AccountType::Standard, "USERNAME", "pA", "Mr Test", 30) == true);
}

TEST_CASE("Ensure that flagged accounts are limited until they are un-flagged") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    // Create two accounts
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    tbc.bank.deposit_to_account("userA", 1000.0);
    tbc.bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 31);
    tbc.bank.deposit_to_account("userB", 250.0);
    bank_system::Account* accA = tbc.bank.login("userA", "pA");

    // Flag account A
    tbc.bank.flag_account("userA");
    
    REQUIRE(accA->deposit(250.0) == bank_system::TransactionStatus::AccountLocked);
    REQUIRE(accA->withdraw(500.0) == bank_system::TransactionStatus::AccountLocked);
    REQUIRE(tbc.bank.transfer("userA", "userB", 500.0) == bank_system::TransactionStatus::AccountLocked);

    // Unflag account A
    tbc.bank.unflag_account("userA");
    REQUIRE(accA->deposit(250.0) == bank_system::TransactionStatus::Success);
    REQUIRE(accA->withdraw(500.0) == bank_system::TransactionStatus::Success);
    REQUIRE(tbc.bank.transfer("userA", "userB", 500.0) == bank_system::TransactionStatus::Success);
}

TEST_CASE("Account can return balance in other currencies") {
    bank_system::clear_transac_data();
    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    tbc.bank.deposit_to_account("userA", 100.00); // In GBP
    bank_system::Account* acc = tbc.bank.login("userA", "pA");

    // 1 penny margin
    double tolerance = 0.01;

    // GBP
    double result = acc->get_balance_in_currency(bank_system::Currency::GBP);
    double expected = 100.00;
    REQUIRE(std::abs(result - expected) < tolerance);

    //USD
    result = acc->get_balance_in_currency(bank_system::Currency::USD);
    expected = 134.00;
    REQUIRE(std::abs(result - expected) < tolerance);

    //EUR
    result = acc->get_balance_in_currency(bank_system::Currency::EUR);
    expected = 116.00;
    REQUIRE(std::abs(result - expected) < tolerance);

    //JPY
    result = acc->get_balance_in_currency(bank_system::Currency::JPY);
    expected = 21273.00;
    REQUIRE(std::abs(result - expected) < tolerance);

    //AUD
    result = acc->get_balance_in_currency(bank_system::Currency::AUD);
    expected = 192.00;
    REQUIRE(std::abs(result - expected) < tolerance);

    //CAD
    result = acc->get_balance_in_currency(bank_system::Currency::CAD);
    expected = 185.00;
    REQUIRE(std::abs(result - expected) < tolerance);
}

TEST_CASE("Transfer fees and limits are applied correctly") {
    bank_system::clear_transac_data();
    TestBankContext tbc;

    double userAStartingCash = 10000.00;
    double userBStartingCash = 1000.00;

    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    tbc.bank.deposit_to_account("userA", userAStartingCash);
    tbc.bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 40);
    tbc.bank.deposit_to_account("userB", userBStartingCash);

    // Should fail as its over the transfer limit
    REQUIRE(tbc.bank.transfer("userA", "userB", bank_system::TransferLimit + 0.01) == bank_system::TransactionStatus::ExceedsBankLimit);

    // Should succeed
    REQUIRE(tbc.bank.transfer("userA", "userB", bank_system::TransferLimit) == bank_system::TransactionStatus::Success);

    bank_system::Account* accA = tbc.bank.login("userA", "pA");
    bank_system::Account* accB = tbc.bank.login("userB", "pB");
    REQUIRE(accA->get_balance() == userAStartingCash - bank_system::TransferLimit - 0.50);
    REQUIRE(accB->get_balance() == userBStartingCash + bank_system::TransferLimit);
}

TEST_CASE("Users can filter account history by type") {
    bank_system::clear_transac_data();

    TestBankContext tbc;
    tbc.bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "John A", 40);

    tbc.bank.deposit_to_account("userA", 50.0);
    tbc.bank.deposit_to_account("userA", 75.0);
    tbc.bank.deposit_to_account("userA", 25.0);

    tbc.bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "James B", 45);
    tbc.bank.transfer("userA", "userB", 50.0);
    tbc.bank.transfer("userB", "userA", 25.0);

    tbc.bank.withdraw_from_account("userA", 10.0);
    tbc.bank.withdraw_from_account("userA", 5.0);
    
    bank_system::Account* accA = tbc.bank.login("userA", "pA");

    REQUIRE(accA->get_history().size() == 7);
    REQUIRE(accA->get_history_by_type(bank_system::TransactionType::Deposit).size() == 3);
    REQUIRE(accA->get_history_by_type(bank_system::TransactionType::Withdrawal).size() == 2);
    REQUIRE(accA->get_history_by_type(bank_system::TransactionType::TransferOut).size() == 1);
    REQUIRE(accA->get_history_by_type(bank_system::TransactionType::TransferIn).size() == 1);
    REQUIRE(accA->get_history_by_type(bank_system::TransactionType::Deposit)[1]._balance == 125);
}


///////////////////////////////////
// More complex tests (~>10ms each)
///////////////////////////////////
TEST_CASE_LONG("Create, deposit to and save 1000 accounts") {
    TestBankContext tbc;
    for (int i = 0; i < 1000; i++) {
        std::string i_str = std::to_string(i);
        tbc.bank.create_account(bank_system::AccountType::Standard, "user" + i_str, "p" + i_str, "Person " + i_str, i);
        bank_system::Account* acc = tbc.bank.login("user" + i_str, "p" + i_str);
        tbc.bank.deposit_to_account("user" + i_str, i);
        REQUIRE(acc->get_balance() == i);
    }
}

TEST_CASE_LONG("Bank applies interest to all accounts correctly") {
    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str());

    // Test vars
    const int num_accounts = 1000;
    const double initial_deposit = 100.00;
    const double interest_rate = 0.05; // 5% interest
    const double expected_balance = 105.00;

    {
        TestBankContext tbc(test_db);

        // Create 1000 accounts with money
        for (int i = 0; i < num_accounts; i++) {
            std::string user = "user" + std::to_string(i);
            tbc.bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
            tbc.bank.deposit_to_account(user, initial_deposit);
        }

        // Apply interest to all
        tbc.bank.apply_monthly_interest(interest_rate);

        // Check a few sample users
        std::vector<std::string> test_users = { "user0", "user500", "user999" };
        for (const std::string& username : test_users) {
            bank_system::Account* acc = tbc.bank.login(username, "pass123");
            REQUIRE(acc != nullptr);
            REQUIRE(acc->get_balance() == expected_balance);
        }
    }

    // Check logging
    SQLite::Database db(test_db, SQLite::OPEN_READONLY);
    SQLite::Statement query(db, "SELECT COUNT(*) FROM transactions");
    REQUIRE(query.executeStep());
    REQUIRE(query.getColumn(0).getInt() == 2000); // 1000 deposit logs and 1000 interest logs
}

TEST_CASE_LONG("Interest sweep mainatins data integrity on failure") {
    bank_system::clear_transac_data();

    // Test vars
    const int num_accounts = 1000;
    const double initial_deposit = 100.00;
    const double interest_rate = 500.0; // Intentionally bad interest value

    TestBankContext tbc;
    // Create 1000 accounts with money
    for (int i = 0; i < num_accounts; i++) {
        std::string user = "user" + std::to_string(i);
        tbc.bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
        tbc.bank.deposit_to_account(user, initial_deposit);
    }

    bool caught_exception = false;
    try {
        tbc.bank.apply_monthly_interest(interest_rate);
    }
    catch (const std::exception& e) {
        (void)e;
        caught_exception = true;
    }

    REQUIRE(caught_exception == true);
    // Now check that all balances are still 100.0
    for (int i = 0; i < num_accounts; i++) {
        std::string user = "user" + std::to_string(i);
        bank_system::Account* acc = tbc.bank.login(user, "pass123");
        REQUIRE(acc != nullptr);
        REQUIRE(acc->get_balance() == initial_deposit);
    }
}

TEST_CASE_LONG("Bank returns account history correctly from saved file and large amount of transactions") {
    // TODO: Check that this works after implementing the transaction table stuff in the db

    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str());

    int num_accounts = 1000;

    {
        TestBankContext tbc(test_db);

        SQLite::Transaction txn(tbc.db);

        // Create 1000 accounts with money
        for (int i = 0; i < num_accounts; i++) {
            std::string user = "user" + std::to_string(i);
            tbc.bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
            tbc.bank.deposit_to_account(user, 100.00);
            tbc.bank.withdraw_from_account(user, 10.00);
        }

        txn.commit();
    }

    // Open new bank, read acc history
    {
        TestBankContext tbc(test_db);
        bank_system::Account* acc500 = tbc.bank.login("user500", "pass123");
        REQUIRE(acc500 != nullptr);
        REQUIRE(acc500->get_history().size() == 2);
        tbc.bank.deposit_to_account("user500", 12.00);
        REQUIRE(acc500->get_history().size() == 3);

    }
    
    std::remove(test_db.c_str());
}

TEST_CASE_LONG("Bank total is correct with thousands of accounts, between saves and loads") {
    const std::string test_db = "test_persistence.db";
    std::remove(test_db.c_str());

    int num_accounts = 2000;

    {
        TestBankContext tbc(test_db);

        SQLite::Transaction txn(tbc.db);

        for (int i = 0; i < num_accounts; i++) {
            std::string user = "user" + std::to_string(i);
            tbc.bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
            tbc.bank.deposit_to_account(user, 100.00);
            tbc.bank.withdraw_from_account(user, 10.00);
        }

        txn.commit();
    }
    
    {
        TestBankContext new_tbc(test_db);
        REQUIRE(new_tbc.bank.get_total_bank_balance() == 90 * num_accounts);
    }
    
    std::remove(test_db.c_str());
}

TEST_CASE_DATABASE("Creating/Opening test database file doesn't fail") {
    bool opened_successfully = false;

    try {
        SQLite::Database db("test.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        opened_successfully = true;
    }
    catch (const std::exception& e) {
        (void)e;
        opened_successfully = false;
    }
    
    REQUIRE(opened_successfully == true);
}