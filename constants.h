#pragma once
#include <stdexcept>
#include <string>
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

	enum class TransactionType {
		Deposit,
		Withdrawal,
		TransferOut,
		TransferIn,
		Interest,
		PasswordChange
	};

	inline TransactionType string_to_transac_type(const std::string& s) {
		static const std::unordered_map<std::string, TransactionType> stringToEnum{
			{"Deposit",      TransactionType::Deposit},
			{"Withdrawal",   TransactionType::Withdrawal},
			{"Transfer Out", TransactionType::TransferOut},
			{"Transfer In",  TransactionType::TransferIn},
			{"Interest",  TransactionType::Interest},
			{"Password Change",  TransactionType::PasswordChange}
		};

		auto it = stringToEnum.find(s);
		if (it != stringToEnum.end()) {
			return it->second;
		}

		throw std::runtime_error("Invalid transaction type in log: " + s);
	}

	inline std::string transac_type_to_string(TransactionType type) {
		switch (type) {
		case TransactionType::Deposit:
			return "Deposit";
			break;
		case TransactionType::Withdrawal:
			return "Withdrawal";
			break;
		case TransactionType::TransferOut:
			return "Transfer Out";
			break;
		case TransactionType::TransferIn:
			return "Transfer In";
			break;
		case TransactionType::Interest:
			return "Interest";
			break;
		case TransactionType::PasswordChange:
			return "Password Change";
			break;
		default:
			throw std::runtime_error("Invalid TransactionType enum!");
		}
	}

	inline const std::unordered_map<Currency, double> exchange_rates {
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