#include "account.h"
#include <string>

namespace bank_system {
	class SavingsAccount : public Account {
	public:
		SavingsAccount(std::string username, std::string password, std::string legal_name, int age, double limit);
		SavingsAccount(std::string username, std::string password, std::string legal_name, int age, double balance, double limit);
		virtual bool withdraw(double amount);
	private:
		double _withdraw_limit;
	};
}