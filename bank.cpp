#include "bank.h"
#include "data_handler.h"
#include <sstream>

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

	bool Bank::deposit_to_account(const std::string& username, double amount) {
		auto it = _accounts.find(username);
		if (it != _accounts.end()) {
			// Only log transaction if it is successful/allowed
			if (it->second->deposit(amount)) {
				log_transac(it->second.get(), "Deposit", amount); // Record the event
				return true;
			}
		}
		return false;
	}

	bool Bank::withdraw_from_account(const std::string& username, double amount) {
		auto it = _accounts.find(username);
		if (it != _accounts.end()) {
			// Only log transaction if it is successful/allowed
			if (it->second->withdraw(amount)) {
				log_transac(it->second.get(), "Withdrawal", amount);
				return true;
			}
		}
		return false;
	}

	std::unordered_map<std::string, std::unique_ptr<Account>> Bank::load() {
		return read_account_data();
	}

	void Bank::save() {
		write_account_data(_accounts);
	}

	void Bank::log_transac(Account* acc, std::string type, double amount) {
		std::stringstream ss;
		ss << "Account: " << acc->get_username() << ", ";
		ss << type << " of ";
		ss << amount << " - New balance: ";
		ss << acc->get_balance();

		_transaction_buffer.push_back(ss.str());
	}

	Bank::~Bank() {
		// Save accounts
		save();

		// Save transactions
		write_transac_data(_transaction_buffer);
	}
}