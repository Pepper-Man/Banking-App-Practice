#pragma once

#include "constants.h"
#include <chrono>
#include <string>

namespace bank_system {
	class TransactionData {
	public:
		std::string _username;
		bank_system::TransactionType _type;
		double _amount;
		double _balance;
		std::chrono::system_clock::time_point _time;

		TransactionData(std::string username, TransactionType type, double amount, double balance, std::chrono::system_clock::time_point time)
			: _username(username), _type(type), _amount(amount), _balance(balance), _time(time) {}
	};
}
