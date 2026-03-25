#pragma once
#include "account.h"
#include "constants.h"
#include <string>

namespace bank_system {
	class SavingsAccount : public Account {
	public:
		SavingsAccount(std::string username, std::string password, std::string legal_name, int age, double limit, bank_system::AccountType type);
		SavingsAccount(std::string username, std::string password, std::string legal_name, int age, double balance, double limit, bank_system::AccountType type);
		virtual TransactionStatus withdraw(double amount);

		// Getters
		double get_withdraw_limit() const;
	private:
		double _withdraw_limit;
	};
}