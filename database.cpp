#include "account.h"
#include "bank.h"
#include <chrono>
#include "constants.h"
#include "database.h"
#include "junior_account.h"
#include <memory>
#include "savings_account.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <string>
#include "transaction.h"
#include <unordered_map>
#include <utility>
#include <vector>

namespace database {

	// Database schema/admin functions

	void init_tables(SQLite::Database& db) {
		db.exec("PRAGMA foreign_keys = ON;");

		// Create accounts table
		db.exec(R"(
			CREATE TABLE IF NOT EXISTS accounts (
				id				INTEGER PRIMARY KEY AUTOINCREMENT,
				username		TEXT UNIQUE NOT NULL,
				password		TEXT NOT NULL,
				real_name		TEXT NOT NULL,
				age				INTEGER NOT NULL,
				account_type	INTEGER NOT NULL,
				balance			REAL NOT NULL DEFAULT 0.0,
				acc_limit		REAL NULL
			);
		)");

		// Create transactions table
		db.exec(R"(
			CREATE TABLE IF NOT EXISTS transactions (
				id			INTEGER PRIMARY KEY AUTOINCREMENT,
				username	TEXT NOT NULL,
				type		TEXT NOT NULL,
				amount		REAL NOT NULL,
				balance		REAL NOT NULL,
				time		INTEGER NOT NULL,
				FOREIGN KEY (username) REFERENCES accounts(username) ON DELETE CASCADE ON UPDATE CASCADE
			);
		)");
	}

	void clear_database(SQLite::Database& db, bank_system::Bank& bank) {
		// Clear database
		db.exec("DELETE FROM accounts;"); // Delete all accounts (schema tells it to delete all associated transactions too)
		db.exec("DELETE FROM sqlite_sequence;"); // Resets autoincrement counters so new IDs start back at 1

		// Clear accounts and transactions
		bank.clear_accounts_memory();
		bank.clear_transactions_memory();
	}

	void clear_transac_data(SQLite::Database& db, bank_system::Bank& bank) {
		db.exec("DELETE FROM transactions");
		bank.clear_transactions_memory();
	}

	// Single entity operations

	void save_user(SQLite::Database& db, const bank_system::Account& account) {
		SQLite::Statement save_query(db,
			"INSERT INTO accounts (username, password, real_name, age, account_type, balance, acc_limit) "
			"VALUES (?, ?, ?, ?, ?, ?, ?) "
			"ON CONFLICT(username) DO UPDATE SET "
			"password = excluded.password, "
			"real_name = excluded.real_name, "
			"age = excluded.age, "
			"account_type = excluded.account_type, "
			"balance = excluded.balance, "
			"acc_limit = excluded.acc_limit"
		);

		save_query.bind(1, account.get_username());
		save_query.bind(2, account.get_psw_hash());
		save_query.bind(3, account.get_leg_name());
		save_query.bind(4, account.get_age());
		save_query.bind(5, static_cast<int>(account.get_type()));
		save_query.bind(6, account.get_balance());

		// Handle extra account limit data
		bank_system::AccountType type = account.get_type();

		if (const auto* savings_acc = dynamic_cast<const bank_system::SavingsAccount*>(&account)) {
			save_query.bind(7, savings_acc->get_withdraw_limit());
		}
		else if (const auto* junior_acc = dynamic_cast<const bank_system::JuniorAccount*>(&account)) {
			save_query.bind(7, junior_acc->get_balance_limit());
		}
		else {
			save_query.bind(7); // No 2nd param means binds to NULL
		}

		save_query.exec();
	}

	void delete_user(SQLite::Database& db, const bank_system::Account& account) {
		SQLite::Statement delete_query(db,
			"DELETE FROM accounts WHERE username = ?"
		);

		delete_query.bind(1, account.get_username());

		delete_query.exec();
	}

	void save_transaction(SQLite::Database& db, const bank_system::TransactionData& transaction) {
		SQLite::Statement transaction_query(db,
			"INSERT INTO transactions (username, type, amount, balance, time) "
			"VALUES (?, ?, ?, ?, ?) "
		);

		transaction_query.bind(1, transaction._username);
		transaction_query.bind(2, transac_type_to_string(transaction._type));
		transaction_query.bind(3, transaction._amount);
		transaction_query.bind(4, transaction._balance);
		transaction_query.bind(5, std::chrono::system_clock::to_time_t(transaction._time));

		transaction_query.exec();
	}

	// Bulk operations

	std::unordered_map<std::string, std::unique_ptr<bank_system::Account>> read_account_data(const SQLite::Database& db) {
		std::unordered_map<std::string, std::unique_ptr<bank_system::Account>> accounts;

		SQLite::Statement load_query(db,
			"SELECT username, password, real_name, age, account_type, balance, acc_limit "
			"FROM accounts"
		);

		// executeStep() return true for each row
		while (load_query.executeStep()) {
			std::string username = load_query.getColumn("username").getText();
			std::string password = load_query.getColumn("password").getText();
			std::string real_name = load_query.getColumn("real_name").getText();
			int age = load_query.getColumn("age").getInt();
			int type_raw = load_query.getColumn("account_type").getInt();
			double balance = load_query.getColumn("balance").getDouble();

			// Instantiate correct class based on type_raw value
			bank_system::AccountType type = static_cast<bank_system::AccountType>(type_raw);
			std::unique_ptr<bank_system::Account> acc;

			if (type == bank_system::AccountType::Savings) {
				double limit = load_query.getColumn("acc_limit").getDouble();
				acc = std::make_unique<bank_system::SavingsAccount>(username, password, real_name, age, limit, type);
			}
			else if (type == bank_system::AccountType::Junior) {
				double limit = load_query.getColumn("acc_limit").getDouble();
				acc = std::make_unique<bank_system::JuniorAccount>(username, password, real_name, age, limit, type);
			}
			else {
				acc = std::make_unique<bank_system::Account>(username, password, real_name, age);
			}

			// Set the old password hash (the account construcor hashed it a second time, cba to write a specific hydration constructor)
			acc->set_psw_hash(password);

			// Restore balance
			acc->set_balance(balance);

			// Insert acc into memory accounts map
			accounts[username] = std::move(acc);
		}

		return accounts;
	}

	void write_account_data(SQLite::Database& db, std::unordered_map<std::string, std::unique_ptr<bank_system::Account>>& accounts) {
		for (const auto& [username, acc] : accounts) {
			if (acc) {
				database::save_user(db, *acc);
			}
		}
	}

	std::vector<bank_system::TransactionData> read_transac_data(const SQLite::Database& db) {
		std::vector<bank_system::TransactionData> all_transaction_data{};

		SQLite::Statement load_query(db,
			"SELECT username, type, amount, balance, time "
			"FROM transactions"
		);

		while (load_query.executeStep()) {
			std::string username = load_query.getColumn("username").getText();
			bank_system::TransactionType type = bank_system::string_to_transac_type(load_query.getColumn("type").getText());
			double amount = load_query.getColumn("amount").getDouble();
			double balance = load_query.getColumn("balance").getDouble();
			time_t raw_time = static_cast<time_t>(load_query.getColumn("time").getInt64());
			std::chrono::system_clock::time_point time = std::chrono::system_clock::from_time_t(raw_time);

			bank_system::TransactionData transac_data = { username, type, amount, balance, time };
			all_transaction_data.push_back(transac_data);
		}

		return all_transaction_data;
	}

	void write_transac_data(SQLite::Database& db, const std::vector<bank_system::TransactionData>& transac_data) {
		for (const auto& transaction : transac_data) {
			database::save_transaction(db, transaction);
		}
	}
}