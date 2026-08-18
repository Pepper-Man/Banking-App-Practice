#pragma once
#include "constants.h"
#include <string>
#include "transaction.h"
#include <vector>

namespace bank_system {
	class Account {
	public:
		// Constructors
		Account(std::string username, std::string password, std::string legal_name, int age, bank_system::AccountType type = bank_system::AccountType::Standard);
		Account(std::string username, std::string psw_hash, std::string legal_name, int age, double balance, bank_system::AccountType type = bank_system::AccountType::Standard);

		// Public Interface
		// Getters
		double get_balance() const;
		double get_balance_in_currency(bank_system::Currency currency) const;
		std::string get_username() const;
		std::string get_psw_hash() const;
		std::string get_leg_name() const;
		int get_age() const;
		std::vector<bank_system::TransactionData> get_history() const;
		bank_system::AccountType get_type() const;
		bool get_flagged() const;
		std::vector<TransactionData> get_history_by_type(TransactionType type) const;

		// Setters
		void set_balance(double new_balance);
		void set_flagged(bool val);
		void set_psw_hash(const std::string& hash);
		
		// Other
		bool check_password(std::string password) const;
		virtual TransactionStatus deposit(double amount);
		virtual TransactionStatus withdraw(double amount);
		bool change_password(const std::string& old_password, const std::string& new_password);
		void add_to_history(const TransactionData& transaction);
		void clear_history();

	private:
		std::string psw_hash(const std::string& password) const;

		// Account variables
		double _balance;
		std::string _username;
		std::string _password_hash;
		std::string _legal_name;
		int _age;
		std::vector<TransactionData> _history;
		bank_system::AccountType _type;
		bool _flagged = false;
	};

	void bank_ui();
}