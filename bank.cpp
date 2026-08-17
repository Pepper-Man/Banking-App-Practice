#include "account.h"
#include "bank.h"
#include "constants.h"
#include "database.h"
#include "data_handler.h"
#include "junior_account.h"
#include "savings_account.h"
#include "SQLiteCpp/Statement.h"
#include "SQLiteCpp/Transaction.h"
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <cctype>
#include <vector>
#include "utility.h"

namespace bank_system {
	bool Bank::create_account(AccountType type, std::string user, std::string pass, std::string name, int age, double w_limit, double b_limit) {
		if (!is_valid_acc_name(user)) return false; // Check that username is allowed

		if (_accounts.find(user) != _accounts.end()) return false; // Account already exists!

		std::unique_ptr<Account> new_acc;

		switch (type) {
		case AccountType::Savings:
			new_acc = std::make_unique<SavingsAccount>(user, pass, name, age, w_limit, type);
			break;
		case AccountType::Junior:
			new_acc = std::make_unique<JuniorAccount>(user, pass, name, age, b_limit, type);
			break;
		case AccountType::Standard:
		default:
			new_acc = std::make_unique<Account>(user, pass, name, age);
			break;
		}

		// Insert new user account into the database
		database::save_user(_db, *new_acc);

		_accounts[user] = std::move(new_acc);
		return true;
	}

	bank_system::Account* Bank::login(std::string username, std::string password) {
		auto it = _accounts.find(username);

		if (it != _accounts.end() && it->second->check_password(password)) {
			return it->second.get();
		}

		// Default to nullptr if acc not found
		return nullptr;
	}

	void Bank::apply_monthly_interest(double rate) {
		// This will hold the "snapshot" of the current (aka old) data
		// so if anything goes wrong, we just undo back to it
		std::unordered_map<std::string, double> rollback_map;
		rollback_map.reserve(_accounts.size()); // Avoid keep resizing the map as the loop goes on - RAM is cheaper than speed here tbh

		std::size_t transactions_added = 0;

		try {
			if (rate <= 0 || rate > 1.0) throw std::invalid_argument("Rate must be > 0.0 and < 1.0"); 

			for (auto& [username, acc] : _accounts) {
				// Record old state before changing it
				rollback_map[username] = acc->get_balance();

				// Interest rate is modified per account type
				double current_rate = rate;
				if (acc->get_type() == AccountType::Savings) current_rate *= SavingsInterestMultiplier;
				if (acc->get_type() == AccountType::Junior) current_rate *= JuniorInterestMultiplier;

				// Apply interest and log
				double interest = acc->get_balance() * current_rate;
				acc->deposit(interest);
				log_transac(acc.get(), TransactionType::Interest, interest);
				transactions_added++;
			}
		}
		// Catch ANY exception
		catch (const std::exception& e) {
			// Since something, anything, went wrong, roll back the data
			// Restore everyones old balance
			for (auto& [username, old_balance] : rollback_map) {
				_accounts[username]->set_balance(old_balance);
			}

			// Remove bad logs
			for (std::size_t i = 0; i < transactions_added; i++) {
				_transaction_buffer.pop_back();
			}

			throw std::runtime_error("Interest sweep failed! Data rolled back. Error: " + std::string(e.what()));
		}
	}

	bool Bank::close_account(const std::string& username) {
		auto it = _accounts.find(username);

		if (it != _accounts.end()) {
			_accounts.erase(it);
			return true;
		}
		return false;
	}

	bool Bank::is_valid_acc_name(const std::string& name) {
		// No empty usernames!
		if (name.empty()) return false;

		// Only allow alphanumerical characters in usernames
		for (const char& c : name) {
			if (!std::isalnum(static_cast<unsigned char>(c))) return false;
		}
		return true;
	}

	void Bank::clear_accounts_memory() {
		_accounts.clear();
	}

	void Bank::clear_transactions_memory() {
		_transaction_buffer.clear();

		for (auto& [username, acc] : _accounts) {
			acc->clear_history();
		}
	}

	std::vector<Account*> Bank::get_accounts_by_type(AccountType type) const {
		std::vector<Account*> found;

		for (const auto& [username, acc] : _accounts) {
			if (acc->get_type() == type) {
				// acc.get() gets the raw ptr from the unique ptr
				found.push_back(acc.get());
			}
		}

		return found;
	}

	std::vector<JuniorAccount*> Bank::get_at_risk_juniors(double tolerance) const {
		std::vector<JuniorAccount*> found;
		std::vector<Account*> all_j_accounts = get_accounts_by_type(AccountType::Junior);

		for (const auto& acc : all_j_accounts) {
			JuniorAccount* j_acc = dynamic_cast<JuniorAccount*>(acc);

			if (j_acc) { // Make sure dynamic cast worked
				double balance = j_acc->get_balance();
				double balance_limit = j_acc->get_balance_limit();

				if (balance_limit - balance <= tolerance) {
					found.push_back(j_acc);
				}
			}
		}

		return found;
	}

	std::pair<std::string, double> Bank::get_highest_balance_holder() const {
		std::string top_user = "None";
		double max_balance = -1.0;

		for (const auto& [username, acc] : _accounts) {
			double current_balance = acc->get_balance();
			if (current_balance > max_balance) {
				max_balance = current_balance;
				top_user = username;
			}
		}

		return { top_user, max_balance };
	}

	double Bank::get_total_bank_balance() const {
		double total = 0.0;

		for (const auto& [username, acc] : _accounts) {
			total += acc->get_balance();
		}

		return total;
	}

	bool Bank::flag_account(const std::string& username) {
		auto it = _accounts.find(username);
		if (it != _accounts.end()) {
			it->second->set_flagged(true);
		}
		return false;
	}

	bool Bank::unflag_account(const std::string& username) {
		auto it = _accounts.find(username);
		if (it != _accounts.end()) {
			it->second->set_flagged(false);
		}
		return false;
	}

	bool Bank::user_exists(const std::string& username) const {
		return _accounts.contains(username);
	}

	TransactionStatus Bank::deposit_to_account(const std::string& username, double amount) {
		auto it = _accounts.find(username);
		if (it != _accounts.end()) {
			// Only log transaction if it is successful/allowed
			TransactionStatus status = it->second->deposit(amount);
			if (status == TransactionStatus::Success) {
				log_transac(it->second.get(), TransactionType::Deposit, amount); // Record the event
				return TransactionStatus::Success;
			}
			return status;
		}
		return TransactionStatus::UnknownAccount;
	}

	TransactionStatus Bank::withdraw_from_account(const std::string& username, double amount) {
		auto it = _accounts.find(username);
		if (it != _accounts.end()) {
			// Only log transaction if it is successful/allowed
			TransactionStatus status = it->second->withdraw(amount);
			if (status == TransactionStatus::Success) {
				log_transac(it->second.get(), TransactionType::Withdrawal, amount);
				return TransactionStatus::Success;
			}
			return status;
		}
		return TransactionStatus::UnknownAccount;
	}

	bool Bank::request_password_change(const std::string& user, const std::string& old_p, const std::string& new_p) {
		auto it = _accounts.find(user);
		if (it != _accounts.end()) {
			bool success = it->second->change_password(old_p, new_p);

			if (success) {
				log_transac(it->second.get(), TransactionType::PasswordChange, 0.0);
			}
			return true;
		}
		return false;
	}

	TransactionStatus Bank::transfer(const std::string& from_user, const std::string& to_user, double amount) {
		// Can't transfer more than bank's current transfer limit
		if (amount > bank_system::TransferLimit) return TransactionStatus::ExceedsBankLimit;

		// Can't transfer between same account
		if (from_user == to_user) return TransactionStatus::SameAccount;

		// Verify both accounts exist, exit early if not
		auto from_it = _accounts.find(from_user);
		auto to_it = _accounts.find(to_user);
		if (from_it == _accounts.end() || to_it == _accounts.end()) return TransactionStatus::UnknownAccount;

		// If either account is flagged, do not allow transfer
		if (from_it->second->get_flagged() || to_it->second->get_flagged()) return TransactionStatus::AccountLocked;

		// Create backups
		double from_balance_backup = from_it->second->get_balance();
		double to_balance_backup = to_it->second->get_balance();

		// Transaction fee is added to withdrawal amount
		double withdraw_amount = amount + bank_system::TransferFee;

		// Ensure from acc has enough funds
		if (withdraw_amount > from_balance_backup) return TransactionStatus::InsufficientFunds;

		// Try to withdraw
		TransactionStatus withdraw_result = from_it->second->withdraw(withdraw_amount);
		if (withdraw_result != TransactionStatus::Success) return withdraw_result;

		// Try to deposit
		TransactionStatus deposit_result = to_it->second->deposit(amount);
		
		// Check for failure, revert to backup if necessary
		if (deposit_result != TransactionStatus::Success) {
			from_it->second->set_balance(from_balance_backup);
			to_it->second->set_balance(to_balance_backup);
			return deposit_result;
		}

		// Both succeeded, now log
		log_transac(from_it->second.get(), TransactionType::TransferOut, withdraw_amount);
		log_transac(to_it->second.get(), TransactionType::TransferIn, amount);

		return TransactionStatus::Success;
	}

	void Bank::get_all_acc_history() {
		// Read past from database
		std::vector<bank_system::TransactionData> all_transaction_data = bank_system::read_transac_data(_db);

		for (const bank_system::TransactionData& transac_data : all_transaction_data) {
			auto it = _accounts.find(transac_data._username);
			if (it != _accounts.end()) {
				it->second->add_to_history(transac_data);
			}
		}
	}

	std::unordered_map<std::string, std::unique_ptr<Account>> Bank::load() const {
		return bank_system::read_account_data(_db);
	}

	void Bank::save() {
		SQLite::Transaction transaction(_db);
		write_account_data(_db, _accounts);
		transaction.commit();
	}

	void Bank::log_transac(Account* acc, TransactionType type, double amount) {
		if (!acc) return;
		TransactionData transac_data = { acc->get_username(), type, amount, acc->get_balance(), std::chrono::system_clock::now() };

		_transaction_buffer.push_back(transac_data);
		acc->add_to_history(transac_data);
	}

	Bank::~Bank() {
		try {
			// Save accounts
			save();

			// Save transactions
			write_transac_data(_db, _transaction_buffer);
		}
		catch (const std::exception& e) {
			std::cerr << "Error during Bank destruction: " << e.what() << std::endl;
		}
	}
}