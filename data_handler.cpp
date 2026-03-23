#include "account.h"
#include "data_handler.h"
#include <fstream>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "constants.h"

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
			std::unique_ptr<Account> acc = std::make_unique<Account>(
				username, 
				acc_data[1], // password hash
				acc_data[2], // legal name
				std::stoi(acc_data[3]), // age, converted to int
				std::stod(acc_data[4]), // balance, converted to double
				static_cast<bank_system::AccountType>(std::stoi(acc_data[5])) // account type, converted to int then converted to enum
			);

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
			data_file << acc->get_type();
			data_file << "\n";
		}

		data_file.close();
	}

	void write_transac_data(const std::vector<std::string>& transac_data) {
		std::ofstream audit_file("transactions.log", std::ios_base::app);

		if (!audit_file.is_open()) {
			throw std::runtime_error("Failed to open transaction log file!");
		}

		for (const std::string& transaction : transac_data) {
			audit_file << transaction;
			audit_file << "\n";
		}
		audit_file.close();
	}
}