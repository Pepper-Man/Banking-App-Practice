#pragma once

#include <memory>
#include <SQLiteCpp/Database.h>
#include <string>
#include "transaction.h"
#include <unordered_map>
#include <vector>

// Forward declarations
namespace bank_system {
	class Account;
	class Bank;
}

namespace database {
	// Database schema/admin functions
	void init_tables(SQLite::Database& db);
	void clear_database(SQLite::Database& db, bank_system::Bank& bank);
	void clear_transac_data(SQLite::Database& db, bank_system::Bank& bank);

	// Single entity operations
	void save_user(SQLite::Database& db, const bank_system::Account& account);
	void delete_user(SQLite::Database& db, const bank_system::Account& account);
	void save_transaction(SQLite::Database& db, const bank_system::TransactionData& transaction);

	// Bulk operations
	std::unordered_map<std::string, std::unique_ptr<bank_system::Account>> read_account_data(const SQLite::Database& db);
	void write_account_data(SQLite::Database& db, std::unordered_map<std::string, std::unique_ptr<bank_system::Account>>& accounts);
	std::vector<bank_system::TransactionData> read_transac_data(const SQLite::Database& db);
	void write_transac_data(SQLite::Database& db, const std::vector<bank_system::TransactionData>& transac_data);
}