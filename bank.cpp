#include "bank.h"
#include "data_handler.h"

namespace bank_system {
	bool Bank::create_account(std::string user, std::string pass, std::string name, int age) {
		// try_emplace only emplaces if key doesnt exist - returns a pair, second is a bool, true if placed, false if already exists
		auto [it, inserted] = _accounts.try_emplace(user, std::make_unique<Account>(user, pass, name, age));
		return inserted;
	}

	bank_system::Account* Bank::login(std::string username, std::string password) {
		auto it = _accounts.find(username);

		if (it != _accounts.end() && it->second->check_password(password)) {
			return it->second.get();
		}

		// Default to nullptr if acc not found
		return nullptr;
	}

	bool Bank::user_exists(const std::string& username) const {
		return _accounts.contains(username);
	}

	std::unordered_map<std::string, std::unique_ptr<Account>> Bank::load() {
		return read_account_data();
	}

	void Bank::save() {
		write_account_data(_accounts);
	}

	Bank::~Bank() {
		save();
	}
}