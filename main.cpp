#include "bank.h"
#include "banking_testing.h"
#include <cstdlib>
#include "database.h"
#include "GuiManager.h"
#include <iostream>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "ui.h"

int main() {
	// Init tests
	bank_system::init_account_tests();
	
	// Init database
	SQLite::Database db("bank.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
	std::cout << "SQLiteCpp initialised successfully!" << std::endl;
	std::cout << "SQLite C Version: " << SQLite::getLibVersion() << std::endl;
	std::cout << "SQLiteCpp Wrapper Version: " << SQLITECPP_VERSION << std::endl;
	database::init_tables(db);

	// Init bank
	bank_system::Bank bank(db);

	// Init UI
	if (!ui::init_ui("Banking System", 750, 350)) {
		std::cerr << "Failed to initialise UI system.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Running UI..." << std::endl;

	// Run UI loop
	while (GuiManager::IsRunning()) {
		ui::ui_loop(bank, db);
	}

	GuiManager::Shutdown();
	return EXIT_SUCCESS;
}