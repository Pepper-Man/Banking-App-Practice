#include "account.h"
#include "constants.h"
#include "junior_account.h"
#include <string>

namespace bank_system {
	JuniorAccount::JuniorAccount(std::string username, std::string password, std::string legal_name, int age, double b_limit, bank_system::AccountType type)
		: Account(username, password, legal_name, age, type) {
		_balance_limit = b_limit;
	}
	JuniorAccount::JuniorAccount(std::string username, std::string password, std::string legal_name, int age, double balance, double b_limit, bank_system::AccountType type)
		: Account(username, password, legal_name, age, balance, type) {
		_balance_limit = b_limit;
	}

	// Junior accounts limited by maximum balance
	TransactionStatus JuniorAccount::deposit(double amount) {
		if (get_balance() + amount > _balance_limit) return TransactionStatus::ExceedsLimit;

		return Account::deposit(amount);
	}

	double JuniorAccount::get_balance_limit() const {
		return _balance_limit;
	}
}