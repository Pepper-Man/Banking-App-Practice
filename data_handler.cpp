#include "data_handler.h"
#include <fstream>
#include <stdexcept>

namespace bank_system {
	std::vector<std::unique_ptr<Account>> read_account_data() {
		std::vector<std::unique_ptr<Account>> accounts;

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