#include "account.h"
#include "data_handler.h"
#include <functional>
#include <sstream>
#include <iomanip>

namespace bank_system {
	Account::Account(std::string username, std::string password, std::string legal_name, int age) :
		_balance(0), _username(username), _password_hash(psw_hash(password)), _legal_name(legal_name), _age(age) {
	}

	Account::Account(std::string username, std::string psw_hash, std::string legal_name, int age, double balance) :
		_username(username), _password_hash(psw_hash), _legal_name(legal_name), _age(age), _balance(balance) {
	}

	void Account::set_balance(double new_balance) {
		_balance = new_balance;
	}

	double Account::get_balance() const {
		return _balance;
	}

	std::string Account::get_username() const {
		return _username;
	}

	std::string Account::get_psw_hash() const {
		return _password_hash;
	}

	std::string Account::get_leg_name() const {
		return _legal_name;
	}

	int Account::get_age() const {
		return _age;
	}

	bool Account::check_password(std::string password) const {
		return psw_hash(password) == _password_hash;
	}

	bool Account::deposit(double amount) {
		if (amount > 0) {
			_balance += amount;
			return true;
		}
		return false;
	}

	bool Account::withdraw(double amount) {
		if (amount > 0 && amount <= _balance) {
			_balance -= amount;
			
			return true;
		}
		return false;
	}

	bool Account::change_password(const std::string& old_password, const std::string& new_password) {
		if (new_password.empty() || old_password == new_password) return false;

		if (psw_hash(old_password) == _password_hash) {
			_password_hash = psw_hash(new_password);
			return true;
		}
		return false;
	}

	std::string Account::psw_hash(const std::string& password) const {
		std::hash<std::string> hasher;
		size_t hashed_val = hasher(password);

		// Convert the number to a hex string to "look" like a real hash
		std::stringstream ss;
		ss << std::hex << std::setw(16) << std::setfill('0') << hashed_val;
		return ss.str();
	}

	void bank_ui() {
		std::string input;

		std::cout << "Welcome to C++ Console Bank" << std::endl;
		std::cout << "\nPlease log in.\nUsername: " << std::endl;
		std::cin >> input;
	}
}