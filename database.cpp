#include "account.h"
#include "constants.h"
#include "database.h"
#include "junior_account.h"
#include "savings_account.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/SQLiteCpp.h>

namespace database {
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
				account_id	INTEGER NOT NULL,
				type		INTEGER NOT NULL,
				amount		REAL NOT NULL,
				FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
			);
		)");
	}

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
}