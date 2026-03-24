#pragma once
namespace bank_system {
	enum AccountType {
		Standard,
		Savings,
		Junior
	};

	// Double interest for Savings accounts, 1.2x interest for Junior accounts
	inline constexpr double SavingsInterestMultiplier = 2.0;
	inline constexpr double JuniorInterestMultiplier = 1.2;
}