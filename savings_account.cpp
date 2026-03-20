#include "savings_account.h"
#include <string>
#include "account.h"

namespace bank_system {
	SavingsAccount::SavingsAccount(std::string username, std::string password, std::string legal_name, int age, double limit)
		: Account(username, password, legal_name, age) {
		_withdraw_limit = limit;
	}

	SavingsAccount::SavingsAccount(std::string username, std::string password, std::string legal_name, int age, double balance, double limit)
		: Account(username, password, legal_name, age, balance) {
		_withdraw_limit = limit;
	}

	// Savings accounts have a limit on the maximum withdrawal
	bool SavingsAccount::withdraw(double amount) {
		if (amount > _withdraw_limit) return false;
		
		return Account::withdraw(amount);
	}
}