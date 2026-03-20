#include "account.h"
#include "constants.h"
#include <string>

namespace bank_system {
	class JuniorAccount : public Account {
	public:
		JuniorAccount(std::string username, std::string password, std::string legal_name, int age, double b_limit, bank_system::AccountType type);
		JuniorAccount(std::string username, std::string password, std::string legal_name, int age, double balance, double b_limit, bank_system::AccountType type);
		virtual bool deposit(double amount);
	private:
		double _balance_limit;
	};
}