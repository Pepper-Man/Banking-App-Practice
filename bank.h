#pragma once

#include "account.h"
#include "constants.h"
#include "database.h"
#include <memory>
#include "SQLiteCpp/Database.h"
#include <string>
#include "transaction.h"
#include <unordered_map>
#include <utility>
#include <vector>

namespace bank_system {
	class JuniorAccount;

	class Bank {
	public:
		// Immediately load saved account data and store database ref
		Bank(SQLite::Database& db) : _db(db) {
			database::init_tables(_db);
			_accounts = load();
			get_all_acc_history();
		}

		// Bank-level functions
		// True if acc created, false if username taken
		bool create_account(AccountType type, std::string user, std::string pass, std::string name, int age, double w_limit = 0.0, double b_limit = 0.0);
		// Returns a pointer to the account if found, else nullptr
		Account* login(std::string username, std::string password);
		// Applies interest to all accounts
		void apply_monthly_interest(double rate);
		bool close_account(const std::string& username);
		void get_all_acc_history();
		
		// Admin/audit functions
		void clear_accounts_memory();
		void clear_database_transactions_from_memory();
		std::vector<Account*> get_accounts_by_type(AccountType type) const;
		std::vector<JuniorAccount*> get_at_risk_juniors(double tolerance) const; // "At risk" is defined as being within the tolerance value of the balance limit
		std::pair<std::string, double> get_highest_balance_holder() const;
		double get_total_bank_balance() const;
		bool flag_account(const std::string& username); // Returns success, not value
		bool unflag_account(const std::string& username); // Returns success, not value

		// Account-level functions
		bool user_exists(const std::string& username) const;
		TransactionStatus deposit_to_account(const std::string& username, double amount);
		TransactionStatus withdraw_from_account(const std::string& username, double amount);
		bool request_password_change(const std::string& user, const std::string& old_p, const std::string& new_p);
		TransactionStatus transfer(const std::string& from_user, const std::string& to_user, double amount);

		// Saved account data load+save
		std::unordered_map<std::string, std::unique_ptr<Account>> load() const;
		void save();

		// Destructor
		~Bank();

	private:
		// Store reference to database
		SQLite::Database& _db;

		// Faster to search than a simple vector; acc username is the key
		std::unordered_map<std::string, std::unique_ptr<Account>> _accounts;

		void log_transac(Account* acc, TransactionType type, double amount);
		std::vector<TransactionData> _transaction_buffer;
		bool is_valid_acc_name(const std::string& name);
	};
}