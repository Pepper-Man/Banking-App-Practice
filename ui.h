#pragma once

#include "account.h"
#include "bank.h"
#include "constants.h"
#include <SQLiteCpp/Database.h>
#include <string>

namespace ui {
	enum class UserSubView {
		None,
		CreateAccount,
		LogIn,
		CreationSuccess,
		AccountHome
	};

	bool init_ui(const char* window_name, int width, int height);
	void RenderBankTestsTab();
	void RenderAllAccountsTable(const bank_system::Bank& bank);
	void RenderAdminTab(SQLite::Database& db, bank_system::Bank& bank);
	void RenderUserTab(bank_system::Bank& bank);
	void ui_loop(bank_system::Bank& bank, SQLite::Database& db);
}