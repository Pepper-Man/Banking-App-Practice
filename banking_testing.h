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
#include <cstdlib>

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
    REQUIRE(acc.withdraw(60.0) == bank_system::TransactionStatus::InsufficientFunds);
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
    REQUIRE(first_line.find("userA,Deposit,69.69,69.69") != std::string::npos);
}

TEST_CASE("User cannot withdraw more than balance") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 29);
    bank.deposit_to_account("userA", 200.00);
    REQUIRE(bank.withdraw_from_account("userA", 200.01) == bank_system::TransactionStatus::InsufficientFunds);
    REQUIRE(bank.withdraw_from_account("userA", 200.00) == bank_system::TransactionStatus::Success);
}

TEST_CASE("User cannot deposit zero or negative amount") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 31);
    REQUIRE(bank.deposit_to_account("userA", 0) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(bank.deposit_to_account("userA", 0.00) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(bank.deposit_to_account("userA", 0.01) == bank_system::TransactionStatus::Success);
    REQUIRE(bank.deposit_to_account("userA", -200.00) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(bank.deposit_to_account("userA", -0) == bank_system::TransactionStatus::InvalidAmount);
    REQUIRE(bank.deposit_to_account("userA", 10.00) == bank_system::TransactionStatus::Success);
    REQUIRE(bank.deposit_to_account("userA", -0.00) == bank_system::TransactionStatus::InvalidAmount);
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
    double expected_total = accA_start_amount + accB_start_amount - bank_system::TransferFee;

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

    // Transfer should fail
    bool caught_exception = false;
    REQUIRE(bank.transfer(userA, userB, accA_start_amount * 2.0) == bank_system::TransactionStatus::InsufficientFunds); // Too much!

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
    REQUIRE(bank.withdraw_from_account("userA", 500.0) == bank_system::TransactionStatus::ExceedsAccountLimit);
    bank_system::Account* base_acc = bank.login("userA", "pA");
    bank_system::SavingsAccount* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(base_acc);
    REQUIRE(savings_acc->get_balance() == 100.0);
    REQUIRE(savings_acc->withdraw(101.0) == bank_system::TransactionStatus::ExceedsAccountLimit);
}

TEST_CASE("Junior account cannot exceed balance limit") {
    double balance_limit = 500.00;
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Junior, "userA", "pA", "Mr A", 25, 0.0, balance_limit);
    REQUIRE(bank.deposit_to_account("userA", 499.00) == bank_system::TransactionStatus::Success);
    REQUIRE(bank.deposit_to_account("userA", 1.00) == bank_system::TransactionStatus::Success);
    REQUIRE(bank.deposit_to_account("userA", 10.00) == bank_system::TransactionStatus::ExceedsAccountLimit);
    REQUIRE(bank.deposit_to_account("userA", 0.01) == bank_system::TransactionStatus::ExceedsAccountLimit);
    REQUIRE(bank.withdraw_from_account("userA", 500.00) == bank_system::TransactionStatus::Success);
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

TEST_CASE("Bank audit function to return accounts by type works correctly") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    // Create two standard accounts
    bank.create_account(bank_system::AccountType::Standard, "standardA", "pA", "Mr StandardA", 30);
    bank.create_account(bank_system::AccountType::Standard, "standardB", "pB", "Mr StandardB", 35);

    // Create three savings accounts
    bank.create_account(bank_system::AccountType::Savings, "savingsA", "pA", "Mr SavingsA", 25, 100.0, 0.0);
    bank.create_account(bank_system::AccountType::Savings, "savingsB", "pB", "Mr SavingsB", 26, 110.0, 0.0);
    bank.create_account(bank_system::AccountType::Savings, "savingsC", "pC", "Mr SavingsC", 27, 120.0, 0.0);

    // Create four junior accounts
    bank.create_account(bank_system::AccountType::Junior, "juniorA", "pA", "Master JuniorA", 13);
    bank.create_account(bank_system::AccountType::Junior, "juniorB", "pB", "Master JuniorB", 14);
    bank.create_account(bank_system::AccountType::Junior, "juniorC", "pC", "Master JuniorC", 15);
    bank.create_account(bank_system::AccountType::Junior, "juniorD", "pD", "Master JuniorD", 16);

    // Test vector sizes, should be same as amount of accounts of type
    REQUIRE(bank.get_accounts_by_type(bank_system::AccountType::Standard).size() == 2);
    REQUIRE(bank.get_accounts_by_type(bank_system::AccountType::Savings).size() == 3);
    REQUIRE(bank.get_accounts_by_type(bank_system::AccountType::Junior).size() == 4);
}

TEST_CASE("Bank audit function to check at-risk junior accounts works correctly") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    // Create two not-at-risk juniors
    bank.create_account(bank_system::AccountType::Junior, "juniorA", "pA", "Master JuniorA", 13, 0.0, 100.0);
    bank.deposit_to_account("juniorA", 50.00);
    bank.create_account(bank_system::AccountType::Junior, "juniorB", "pB", "Master JuniorB", 14, 0.0, 150.0);
    bank.deposit_to_account("juniorB", 100.00);

    // Create two at-risk juniors
    bank.create_account(bank_system::AccountType::Junior, "juniorC", "pC", "Master JuniorC", 15, 0.0, 100.0);
    bank.deposit_to_account("juniorC", 95.00);
    bank.create_account(bank_system::AccountType::Junior, "juniorD", "pD", "Master JuniorD", 16, 0.0, 150.0);
    bank.deposit_to_account("juniorD", 140.10);

    REQUIRE(bank.get_at_risk_juniors(10.00).size() == 2);
}

TEST_CASE("Bank audit function to find highest value user") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    // Create some users
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    bank.deposit_to_account("userA", 15000000.00);
    bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 31);
    bank.deposit_to_account("userB", 20000000.00);
    bank.create_account(bank_system::AccountType::Standard, "userC", "pC", "Mr C", 32);
    bank.deposit_to_account("userC", 5000000.00);

    REQUIRE(bank.get_highest_balance_holder().first == "userB");
    REQUIRE(bank.get_highest_balance_holder().second == 20000000.00);
}

TEST_CASE("Bank total balance is correct") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    // Create some users
    bank.create_account(bank_system::AccountType::Standard, "standardA", "pA", "Mr A", 30);
    bank.deposit_to_account("standardA", 15000000.00);
    bank.create_account(bank_system::AccountType::Standard, "standardB", "pB", "Mr B", 31);
    bank.deposit_to_account("standardB", 20000000.00);
    bank.create_account(bank_system::AccountType::Standard, "standardC", "pC", "Mr C", 32);
    bank.deposit_to_account("standardC", 5000000.00);
    bank.create_account(bank_system::AccountType::Savings, "savingsA", "pA", "Mr A", 30, 100.0, 0.0);
    bank.deposit_to_account("savingsA", 15000000.00);
    bank.create_account(bank_system::AccountType::Savings, "savingsB", "pB", "Mr B", 31, 100.0, 0.0);
    bank.deposit_to_account("savingsB", 20000000.00);
    bank.create_account(bank_system::AccountType::Junior, "juniorA", "pA", "Master C", 15, 0.0, 100.0);
    bank.deposit_to_account("juniorA", 50.00);

    REQUIRE(bank.get_total_bank_balance() == 75000050);
}

TEST_CASE("Bank can close accounts of all types") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    bank.create_account(bank_system::AccountType::Standard, "standard", "pA", "Mr A", 30);
    bank.create_account(bank_system::AccountType::Savings, "savings", "pB", "Mr B", 31, 100.0, 0.0);
    bank.create_account(bank_system::AccountType::Junior, "junior", "pC", "Mr C", 32, 0.0, 100.0);

    // close_account returns true if successful
    REQUIRE(bank.close_account("standard"));
    REQUIRE(bank.close_account("savings"));
    REQUIRE(bank.close_account("junior"));

    // Shouldn't be able to log in to deleted accounts
    REQUIRE(bank.login("standard", "pA") == nullptr);
    REQUIRE(bank.login("savings", "pB") == nullptr);
    REQUIRE(bank.login("junior", "pC") == nullptr);
}

TEST_CASE("Bank applies correct interest amount to different account types") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    // Create an account of each type
    bank.create_account(bank_system::AccountType::Standard, "standard", "pA", "Mr A", 30);
    bank.create_account(bank_system::AccountType::Savings, "savings", "pB", "Mr B", 31, 100.0, 0.0);
    bank.create_account(bank_system::AccountType::Junior, "junior", "pC", "Mr C", 32, 0.0, 300.0);
    bank.deposit_to_account("standard", 100.0);
    bank.deposit_to_account("savings", 100.0);
    bank.deposit_to_account("junior", 100.0);

    // Apply interest
    bank.apply_monthly_interest(0.10); // 10% interest

    // Log in to accounts
    bank_system::Account* standard_acc = bank.login("standard", "pA");
    bank_system::SavingsAccount* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(bank.login("savings", "pB"));
    bank_system::JuniorAccount* junior_acc = dynamic_cast<bank_system::JuniorAccount*>(bank.login("junior", "pC"));
    REQUIRE(standard_acc != nullptr);
    REQUIRE(savings_acc != nullptr);
    REQUIRE(junior_acc != nullptr);

    // Check balances
    REQUIRE(standard_acc->get_balance() == 110.0);
    REQUIRE(savings_acc->get_balance() == 120.0);
    REQUIRE(junior_acc->get_balance() == 112.0);
}

TEST_CASE("Account usernames are correctly validated") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "username123TEST", "pA", "Mr Test", 30) == true);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "user,name", "pA", "Mr Test", 30) == false);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "user name", "pA", "Mr Test", 30) == false);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, " username", "pA", "Mr Test", 30) == false);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "username ", "pA", "Mr Test", 30) == false);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "us3rn4m3", "pA", "Mr Test", 30) == true);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "username!", "pA", "Mr Test", 30) == false);
    REQUIRE(bank.create_account(bank_system::AccountType::Standard, "USERNAME", "pA", "Mr Test", 30) == true);
}

TEST_CASE("Ensure that flagged accounts are limited until they are un-flagged") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    // Create two accounts
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    bank.deposit_to_account("userA", 1000.0);
    bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 31);
    bank.deposit_to_account("userB", 250.0);
    bank_system::Account* accA = bank.login("userA", "pA");

    // Flag account A
    bank.flag_account("userA");
    
    REQUIRE(accA->deposit(250.0) == bank_system::TransactionStatus::AccountLocked);
    REQUIRE(accA->withdraw(500.0) == bank_system::TransactionStatus::AccountLocked);
    REQUIRE(bank.transfer("userA", "userB", 500.0) == bank_system::TransactionStatus::AccountLocked);

    // Unflag account A
    bank.unflag_account("userA");
    REQUIRE(accA->deposit(250.0) == bank_system::TransactionStatus::Success);
    REQUIRE(accA->withdraw(500.0) == bank_system::TransactionStatus::Success);
    REQUIRE(bank.transfer("userA", "userB", 500.0) == bank_system::TransactionStatus::Success);
}

TEST_CASE("Account can return balance in other currencies") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;
    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    bank.deposit_to_account("userA", 100.00); // In GBP
    bank_system::Account* acc = bank.login("userA", "pA");

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
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();
    bank_system::Bank bank;

    double userAStartingCash = 10000.00;
    double userBStartingCash = 1000.00;

    bank.create_account(bank_system::AccountType::Standard, "userA", "pA", "Mr A", 30);
    bank.deposit_to_account("userA", userAStartingCash);
    bank.create_account(bank_system::AccountType::Standard, "userB", "pB", "Mr B", 40);
    bank.deposit_to_account("userB", userBStartingCash);

    // Should fail as its over the transfer limit
    REQUIRE(bank.transfer("userA", "userB", bank_system::TransferLimit + 0.01) == bank_system::TransactionStatus::ExceedsBankLimit);

    // Should succeed
    REQUIRE(bank.transfer("userA", "userB", bank_system::TransferLimit) == bank_system::TransactionStatus::Success);

    bank_system::Account* accA = bank.login("userA", "pA");
    bank_system::Account* accB = bank.login("userB", "pB");
    REQUIRE(accA->get_balance() == userAStartingCash - bank_system::TransferLimit - 0.50);
    REQUIRE(accB->get_balance() == userBStartingCash + bank_system::TransferLimit);
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

TEST_CASE("Bank total is correct with thousands of accounts, between saves and loads") {
    bank_system::clear_saved_data();
    bank_system::clear_transac_data();

    int num_accounts = 2000;

    {
        bank_system::Bank bank;
        for (int i = 0; i < num_accounts; i++) {
            std::string user = "user" + std::to_string(i);
            bank.create_account(bank_system::AccountType::Standard, user, "pass123", "Full Name", 30);
            bank.deposit_to_account(user, 100.00);
            bank.withdraw_from_account(user, 10.00);
        }
    }

    bank_system::Bank new_bank;
    REQUIRE(new_bank.get_total_bank_balance() == 90 * num_accounts);
}

#endif

void get_going() {
#ifdef RUN_QUICK_TESTS
    run_all_tests();
    std::cout << "Press Enter to exit tests..." << std::endl;
    std::cin.get();
#else
    bank_system::bank_ui();
#endif
}