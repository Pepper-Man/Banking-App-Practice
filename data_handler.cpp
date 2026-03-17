#include "data_handler.h"
#include <fstream>
#include <stdexcept>
#include <vector>
#include <sstream>

namespace bank_system {
	void clear_saved_data() {
		std::ofstream file("data.csv");
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
			std::unique_ptr<Account> acc = std::make_unique<Account>(username, acc_data[1], acc_data[2], std::stoi(acc_data[3]), std::stod(acc_data[4]));

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
			data_file << acc->get_balance();
			data_file << "\n";
		}

		data_file.close();
	}
}