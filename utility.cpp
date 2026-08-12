#include <string>
#include "utility.h"
#include <vector>

namespace utility {
	std::vector<std::string> split_csv_line(const std::string& line) {
		std::vector<std::string> parts;

		std::string part = "";
		for (const char& c : line) {
			if (c == ',') {
				parts.push_back(part);
				part = "";
			}
			else {
				part += c;
			}
		}

		parts.push_back(part);
		return parts;
	}
}