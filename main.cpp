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
	if (!bank_system::init_account_tests()) {
		std::cerr << "Failed to initialise bank tests.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Tests successfully initialised..." << std::endl;
	
	// Init database
	SQLite::Database db = database::open_database("bank.db");
	std::cout << "Database successfully initialised..." << std::endl;
	std::cout << "SQLite C Version: " << SQLite::getLibVersion() << std::endl;
	std::cout << "SQLiteCpp Wrapper Version: " << SQLITECPP_VERSION << std::endl;
	if (!database::init_tables(db)) {
		std::cerr << "Failed to initialize database schema.\n";
		return EXIT_FAILURE;
	}

	// Init bank
	bank_system::Bank bank(db);
	std::cout << "Bank system successfully initialised..." << std::endl;

	// Init UI
	if (!ui::init_ui("Banking System", 750, 350)) {
		std::cerr << "Failed to initialise UI system.\n";
		return EXIT_FAILURE;
	}
	std::cout << "UI successfully initialised..." << std::endl;

	std::cout << "Running UI..." << std::endl;

	// Run UI loop
	while (GuiManager::IsRunning()) {
		ui::ui_loop(bank, db);
	}

	GuiManager::Shutdown();
	return EXIT_SUCCESS;
}