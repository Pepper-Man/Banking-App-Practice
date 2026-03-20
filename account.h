#pragma once
#include <iostream>
#include <string>
#include <vector>

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
		std::vector<std::string> get_history() const;

		// Setters
		void set_balance(double new_balance);
		
		// Other
		bool check_password(std::string password) const;
		bool deposit(double amount);
		bool withdraw(double amount);
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
	};

	void bank_ui();
}