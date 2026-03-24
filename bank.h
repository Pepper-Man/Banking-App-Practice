#pragma once

#include "account.h"
#include "constants.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

namespace bank_system {
	class JuniorAccount;

	class Bank {
	public:
		// Immediately load saved account data
		Bank() : _accounts(load()) {}

		// Bank-level functions
		// True if acc created, false if username taken
		bool create_account(AccountType type, std::string user, std::string pass, std::string name, int age, double w_limit = 0.0, double b_limit = 0.0);
		// Returns a pointer to the account if found, else nullptr
		Account* login(std::string username, std::string password);
		// Applies interest to all accounts
		void apply_monthly_interest(double rate);
		
		// Admin/audit functions
		std::vector<Account*> get_accounts_by_type(AccountType type) const;
		std::vector<JuniorAccount*> get_at_risk_juniors(double tolerance) const; // "At risk" is defined as being within the tolerance value of the balance limit
		std::pair<std::string, double> get_highest_balance_holder() const;
		double get_total_bank_balance() const;

		// Account-level functions
		bool user_exists(const std::string& username) const;
		bool deposit_to_account(const std::string& username, double amount);
		bool withdraw_from_account(const std::string& username, double amount);
		bool request_password_change(const std::string& user, const std::string& old_p, const std::string& new_p);
		bool transfer(const std::string& from_user, const std::string& to_user, double amount);
		void get_acc_history();

		// Saved account data load+save
		std::unordered_map<std::string, std::unique_ptr<Account>> load();
		void save();

		// Destructor
		~Bank();

	private:
		// Faster to search than a simple vector; acc username is the key
		std::unordered_map<std::string, std::unique_ptr<Account>> _accounts;

		void log_transac(Account* acc, std::string type, double amount);
		std::vector<std::string> _transaction_buffer;
	};
}