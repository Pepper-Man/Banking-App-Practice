#pragma once

#include "account.h"
#include "data_handler.h"
#include <SQLiteCpp/Database.h>

namespace database {
	void init_tables(SQLite::Database& db);
	void save_user(SQLite::Database& db, const bank_system::Account& account);
	void delete_user(SQLite::Database& db, const bank_system::Account& account);
	void save_transaction(SQLite::Database& db, const bank_system::TransactionData& transaction);
}