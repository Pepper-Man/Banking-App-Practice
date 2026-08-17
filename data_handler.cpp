#include "account.h"
#include "bank.h"
#include "constants.h"
#include "database.h"
#include "data_handler.h"
#include "junior_account.h"
#include "savings_account.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include <chrono>
#include <fstream>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bank_system {
	void clear_database(SQLite::Database& db, bank_system::Bank& bank) {
		// Clear database
		db.exec("DELETE FROM accounts;"); // Delete all accounts (schema tells it to delete all associated transactions too)
		db.exec("DELETE FROM sqlite_sequence;"); // Resets autoincrement counters so new IDs start back at 1

		// Clear memory
		bank.clear_memory();
	}

	void clear_transac_data(SQLite::Database& db, bank_system::Bank& bank) {
		db.exec("DELETE FROM transactions");
		bank.clear_memory();
	}

	std::unordered_map<std::string, std::unique_ptr<Account>> read_account_data(const SQLite::Database& db) {
		std::unordered_map<std::string, std::unique_ptr<Account>> accounts;

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

	void write_account_data(SQLite::Database& db, std::unordered_map<std::string, std::unique_ptr<Account>>& accounts) {
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
			bank_system::TransactionType type = string_to_transac_type(load_query.getColumn("type").getText());
			double amount = load_query.getColumn("amount").getDouble();
			double balance = load_query.getColumn("balance").getDouble();
			time_t raw_time = static_cast<time_t>(load_query.getColumn("time").getInt64());
			std::chrono::system_clock::time_point time = std::chrono::system_clock::from_time_t(raw_time);

			bank_system::TransactionData transac_data = { username, type, amount, balance, time };
			all_transaction_data.push_back(transac_data);
		}

		return all_transaction_data;
	}

	void write_transac_data(SQLite::Database& db, const std::vector<TransactionData>& transac_data) {
		for (const auto& transaction : transac_data) {
			database::save_transaction(db, transaction);
		}
	}
}