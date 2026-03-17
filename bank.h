#pragma once

#include <unordered_map>
#include <memory>
#include "account.h"

namespace bank_system {
	class Bank {
	public:
		// Immediately load saved account data
		Bank() : _accounts(load()) {}
		// True if acc created, false if username taken
		bool create_account(std::string user, std::string pass, std::string name, int age);

		// Returns a pointer to the account if found, else nullptr
		Account* login(std::string username, std::string password);

		bool user_exists(const std::string& username) const;

		std::unordered_map<std::string, std::unique_ptr<Account>> load();
		void save();

		~Bank();

	private:
		// Faster to search than a simple vector; acc username is the key
		std::unordered_map<std::string, std::unique_ptr<Account>> _accounts;
	};
}