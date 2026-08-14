#pragma once

#include "constants.h"
#include <chrono>
#include <memory>
#include "SQLiteCpp/Database.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace bank_system {
	class Account;
	class Bank;

	class TransactionData {
	public:
		std::string _username;
		TransactionType _type;
		double _amount;
		double _balance;
		std::chrono::system_clock::time_point _time;

		TransactionData(std::string username, TransactionType type, double amount, double balance, std::chrono::system_clock::time_point time)
			: _username(username), _type(type), _amount(amount), _balance(balance), _time(time) {
		}
	};

	void clear_database(SQLite::Database& db, bank_system::Bank& bank);
	void clear_transac_data();

	std::unordered_map<std::string, std::unique_ptr<Account>> read_account_data(const SQLite::Database& db);

	void write_account_data(SQLite::Database& db, std::unordered_map<std::string, std::unique_ptr<Account>>& accounts);

	void write_transac_data(const std::vector<TransactionData>& transac_data);
}