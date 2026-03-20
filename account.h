#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "constants.h"

namespace bank_system {
	class Account {
	public:
		// Constructors
		Account(std::string username, std::string password, std::string legal_name, int age, bank_system::AccountType type = bank_system::AccountType::Standard);
		Account(std::string username, std::string psw_hash, std::string legal_name, int age, double balance, bank_system::AccountType type = bank_system::AccountType::Standard);

		// Public Interface
		// Getters
		double get_balance() const;
		std::string get_username() const;
		std::string get_psw_hash() const;
		std::string get_leg_name() const;
		int get_age() const;
		std::vector<std::string> get_history() const;
		bank_system::AccountType get_type() const;

		// Setters
		void set_balance(double new_balance);
		
		// Other
		bool check_password(std::string password) const;
		bool deposit(double amount);
		virtual bool withdraw(double amount);
		bool change_password(const std::string& old_password, const std::string& new_password);
		void add_to_history(const std::string& transaction);

	private:
		std::string psw_hash(const std::string& password) const;

		// Account variables
		double _balance;
		std::string _username;
		std::string _password_hash;
		std::string _legal_name;
		int _age;
		std::vector<std::string> _history;
		bank_system::AccountType _type;
	};

	void bank_ui();
}