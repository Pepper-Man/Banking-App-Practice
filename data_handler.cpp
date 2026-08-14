#include "account.h"
#include "bank.h"
#include "constants.h"
#include "database.h"
#include "data_handler.h"
#include "junior_account.h"
#include "savings_account.h"
#include "SQLiteCpp/Database.h"
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

	void clear_transac_data() {
		std::ofstream file("transactions.log");
		file.close();
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

		// On bank load, we read all transactions and set the history for each account
		// TODO: Fix getting acc history once we have the transactions table working
		//get_all_acc_history();
		return accounts;
	}

	void write_account_data(SQLite::Database& db, std::unordered_map<std::string, std::unique_ptr<Account>>& accounts) {
		for (const auto& [username, acc] : accounts) {
			if (acc) {
				database::save_user(db, *acc);
			}
		}
	}

	void write_transac_data(const std::vector<TransactionData>& transac_data) {
		std::ofstream audit_file("transactions.log", std::ios_base::app);

		if (!audit_file.is_open()) {
			throw std::runtime_error("Failed to open transaction log file!");
		}

		for (const TransactionData& transaction : transac_data) {
			audit_file << transaction._username << ",";
			audit_file << transac_type_to_string(transaction._type) << ",";
			audit_file << transaction._amount << ",";
			audit_file << transaction._balance << ",";
			audit_file << std::chrono::system_clock::to_time_t(transaction._time);
			audit_file << "\n";
		}
		audit_file.close();
	}
}