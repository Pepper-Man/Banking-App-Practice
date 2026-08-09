#include "account.h"
#include "bank.h"
#include "constants.h"
#include "data_handler.h"
#include "junior_account.h"
#include "savings_account.h"
#include <chrono>
#include <fstream>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bank_system {
	void clear_saved_data() {
		std::ofstream file("data.csv");
		file.close();
	}

	void clear_transac_data() {
		std::ofstream file("transactions.log");
		file.close();
	}

	std::unordered_map<std::string, std::unique_ptr<Account>> read_account_data() {
		std::unordered_map<std::string, std::unique_ptr<Account>> accounts;
		std::string acc_str;
		std::ifstream data_file("data.csv");

		while (std::getline(data_file, acc_str)) {
			if (acc_str.empty()) continue; // Skip empty lines

			// Split csv line into parts
			std::vector<std::string> acc_data;
			std::istringstream ss(acc_str);
			std::string part;
			while (std::getline(ss, part, ',')) {
				acc_data.push_back(part);
			}

			// Now read parts and create acc
			std::string username = acc_data[0];
			std::string pass_hash = acc_data[1];
			std::string legal_name = acc_data[2];
			int age = std::stoi(acc_data[3]);
			double balance = std::stod(acc_data[4]);
			bank_system::AccountType type = static_cast<bank_system::AccountType>(std::stoi(acc_data[5])); // account type, converted to int then converted to enum
			std::unique_ptr<Account> acc;

			switch (type) {
			// Extra braces to calm compiler about the var declaration
			case bank_system::Savings:
				{
				double w_limit = std::stod(acc_data[6]);
				acc = std::make_unique<SavingsAccount>(username, pass_hash, legal_name, age, balance, w_limit, bank_system::AccountType::Savings);
				}
				
				break;
			case bank_system::Junior:
				{
				double b_limit = std::stod(acc_data[6]);
				acc = std::make_unique<JuniorAccount>(username, pass_hash, legal_name, age, balance, b_limit, bank_system::AccountType::Junior);
				}
				
				break;

			default:
				acc = std::make_unique<Account>(username, pass_hash, legal_name, age, balance, bank_system::AccountType::Standard);
			}

			accounts.try_emplace(username, std::move(acc));
		}

		data_file.close();
		return accounts;
	}

	void write_account_data(std::unordered_map<std::string, std::unique_ptr<Account>>& accounts) {
		std::ofstream data_file("data.csv");

		if (!data_file.is_open()) {
			throw std::runtime_error("Failed to open user data file!");
		}

		for (const auto& [username, acc] : accounts) {
			data_file << acc->get_username() << ",";
			data_file << acc->get_psw_hash() << ",";
			data_file << acc->get_leg_name() << ",";
			data_file << acc->get_age() << ",";
			data_file << acc->get_balance() << ",";

			// Handle extra account data
			bank_system::AccountType type = acc->get_type();
			data_file << type;

			switch (type) {
			// In these cases we cast to specific acc type so we can access its extra data
			// Also it is wrapped in braces so the compiler doesnt get iffy about the variable declaration
			case bank_system::AccountType::Savings:
				{
					auto* savings_acc = dynamic_cast<bank_system::SavingsAccount*>(acc.get());
					if (savings_acc) data_file << "," << savings_acc->get_withdraw_limit();
				}
				
				break;
			case bank_system::AccountType::Junior:
				{
					auto* junior_acc = dynamic_cast<bank_system::JuniorAccount*>(acc.get());
					if (junior_acc) data_file << "," << junior_acc->get_balance_limit();
				}
				break;
			default:
				break;
			}

			data_file << "\n";
		}

		data_file.close();
	}

	void write_transac_data(const std::vector<TransactionData>& transac_data) {
		std::ofstream audit_file("transactions.log", std::ios_base::app);

		if (!audit_file.is_open()) {
			throw std::runtime_error("Failed to open transaction log file!");
		}

		for (const TransactionData& transaction : transac_data) {
			audit_file << transaction._username << ",";
			audit_file << transac_type_to_string(transaction._type) << ",";
			audit_file << transaction._amount << ",";
			audit_file << transaction._balance << ",";
			audit_file << std::chrono::system_clock::to_time_t(transaction._time);
			audit_file << "\n";
		}
		audit_file.close();
	}
}