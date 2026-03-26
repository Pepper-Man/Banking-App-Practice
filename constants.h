#pragma once
#include <unordered_map>

namespace bank_system {
	enum AccountType {
		Standard,
		Savings,
		Junior
	};

	enum Currency {
		GBP,
		USD,
		EUR,
		JPY,
		AUD,
		CAD
	};

	enum class TransactionStatus {
		Success,
		InsufficientFunds,
		ExceedsAccountLimit,
		ExceedsBankLimit,
		AccountLocked,
		InvalidAmount,
		UnknownAccount,
		SameAccount
	};

	inline const std::unordered_map<Currency, double> exchange_rates{
		{ GBP, 1.00 },
		{ USD, 1.34 },
		{ EUR, 1.16 },
		{ JPY, 212.73 },
		{ AUD, 1.92 },
		{ CAD, 1.85 }
	};

	// Double interest for Savings accounts, 1.2x interest for Junior accounts
	inline constexpr double SavingsInterestMultiplier = 2.0;
	inline constexpr double JuniorInterestMultiplier = 1.2;

	// Transaction fees + limits
	inline constexpr double TransferFee = 0.50;
	inline constexpr double TransferLimit = 5000.00;
}