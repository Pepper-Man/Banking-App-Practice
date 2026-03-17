#pragma once
#include <unordered_map>
#include <memory>
#include "account.h"

namespace bank_system {
	void clear_saved_data();

	std::unordered_map<std::string, std::unique_ptr<Account>> read_account_data();

	void write_account_data(std::unordered_map<std::string, std::unique_ptr<Account>>& accounts);

	void write_transac_data(Account* acc, std::string type, double amount);
}