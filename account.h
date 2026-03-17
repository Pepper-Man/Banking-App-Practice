#pragma once
#include <iostream>
#include <string>

namespace bank_system {
	class Account {
	public:
		// Constructors
		Account(std::string username, std::string password, std::string legal_name, int age);
		Account(std::string username, std::string psw_hash, std::string legal_name, int age, double balance);

		// Public Interface
		// Getters
		double get_balance() const;
		std::string get_username() const;
		std::string get_psw_hash() const;
		std::string get_leg_name() const;
		int get_age() const;

		bool check_password(std::string password) const;
		void deposit(double amount);
		bool withdraw(double amount);

	private:
		std::string psw_hash(const std::string& password) const;

		// Account variables
		double _balance;
		std::string _username;
		std::string _password_hash;
		std::string _legal_name;
		int _age;
	};

	void bank_ui();
}