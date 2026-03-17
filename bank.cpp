#include "bank.h"
#include "data_handler.h"

namespace bank_system {
	bool Bank::create_account(std::string user, std::string pass, std::string name, int age) {
		if (user_exists(user)) return false;

		_accounts[user] = std::make_unique<Account>(user, pass, name, age);
		return true;
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

	void Bank::save() {
		write_account_data(_accounts);
	}
}