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

	void Bank::apply_monthly_interest(double rate) {
		// This will hold the "snapshot" of the current (aka old) data
		// so if anything goes wrong, we just undo back to it
		std::unordered_map<std::string, double> rollback_map;
		rollback_map.reserve(_accounts.size()); // Avoid keep resizing the map as the loop goes on - RAM is cheaper than speed here tbh

		std::size_t transactions_added = 0;

		try {
			if (rate <= 0 || rate > 1.0) throw std::invalid_argument("Rate must be > 0.0 and < 1.0"); 

			for (auto& [username, acc] : _accounts) {
				// Record old state before changing it
				rollback_map[username] = acc->get_balance();

				double interest = acc->get_balance() * rate;
				acc->deposit(interest);
				log_transac(acc.get(), "Interest", interest);
				transactions_added++;
			}
		}
		// Catch ANY exception
		catch (const std::exception& e) {
			// Since something, anything, went wrong, roll back the data
			// Restore everyones old balance
			for (auto& [username, old_balance] : rollback_map) {
				_accounts[username]->set_balance(old_balance);
			}

			// Remove bad logs
			for (std::size_t i = 0; i < transactions_added; i++) {
				_transaction_buffer.pop_back();
			}

			throw std::runtime_error("Interest sweep failed! Data rolled back. Error: " + std::string(e.what()));
		}
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