#include "account.h"
#include "bank.h"
#include <chrono>
#include "constants.h"
#include <ctime>
#include "database.h"
#include "imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "GuiManager.h"
#include <iostream>
#include "simple_test.h"
#include <SQLiteCpp/Database.h>
#include <string>
#include "transaction.h"
#include "Windows.h"
#include "ui.h"

namespace ui {

	// Some UI colour data
	constexpr ImVec4 button_red_default = ImVec4(0.65f, 0.18f, 0.18f, 1.0f);
	constexpr ImVec4 button_red_hovered = ImVec4(0.80f, 0.25f, 0.25f, 1.0f);
	constexpr ImVec4 button_red_active = ImVec4(0.50f, 0.12f, 0.12f, 1.0f);

	constexpr ImVec4 button_green_default = ImVec4(0.15f, 0.55f, 0.22f, 1.0f);
	constexpr ImVec4 button_green_hovered = ImVec4(0.20f, 0.68f, 0.28f, 1.0f);
	constexpr ImVec4 button_green_active = ImVec4(0.10f, 0.42f, 0.16f, 1.0f);

	// Keep track of selected currency
	static bank_system::Currency selected_currency = bank_system::Currency::GBP;

	bool init_ui(const char* window_name, int width, int height) {
		// Need this to force windows to make the program DPI aware so the font isn't blurry
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		// Initialise UI window
		return GuiManager::Init(window_name, width, height);
	}

	static std::string GetTransactionErrorMessage(bank_system::TransactionStatus transaction_status) {
		switch (transaction_status) {
		case bank_system::TransactionStatus::InsufficientFunds:
			return "You have insufficient funds for this transaction!";
			break;
		case bank_system::TransactionStatus::ExceedsAccountLimit:
			return "This amount exceeds your account's withdrawal limit!";
			break;
		case bank_system::TransactionStatus::ExceedsBankLimit:
			return "This amount exceeds the bank's withdrawal limit!";
			break;
		case bank_system::TransactionStatus::AccountLocked:
			return "Cannot complete transaction - your account is locked!";
			break;
		case bank_system::TransactionStatus::InvalidAmount:
			return "This amount is not valid! Make sure it is not negative.";
			break;
		case bank_system::TransactionStatus::UnknownAccount:
			return "Destination account cannot be found!";
			break;
		case bank_system::TransactionStatus::SameAccount:
			return "You are attempting to transfer to and from the same account!";
			break;
		default:
			return "No Error";
			break;
		}
	}

	// Helper function for rendering the deposit/withdrawal overlay window
	static void RenderFundsChangeModal(bank_system::Bank& bank, bank_system::Account* logged_in_account, const std::string& window_name, const std::string& display_text) {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 0)); // Fixed 500px width, auto-fit height

		static bank_system::TransactionStatus transaction_status{};

		if (ImGui::BeginPopupModal(window_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			static double funds_change_amount;
			ImGui::Text(display_text.c_str());
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::Text("£");
			ImGui::SameLine();
			ImGui::InputDouble("##depositamount", &funds_change_amount, 0.0, 0.0, "%.2f");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Cancel button (red)
			ImGui::PushStyleColor(ImGuiCol_Button, button_red_default);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_red_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_red_active);
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				funds_change_amount = 0.0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			// Confirm button (green)
			ImGui::PushStyleColor(ImGuiCol_Button, button_green_default);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_green_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_green_active);
			if (ImGui::Button("Confirm", ImVec2(120, 0))) {
				if (funds_change_amount > 0.0) {

					if (window_name == "Deposit Funds") {
						transaction_status = bank.deposit_to_account(logged_in_account->get_username(), funds_change_amount);
					}
					else if (window_name == "Withdraw Funds") {
						transaction_status = bank.withdraw_from_account(logged_in_account->get_username(), funds_change_amount);
					}
					else {
						std::cout << "Unknown fund change type!" << std::endl;
					}

					if (transaction_status == bank_system::TransactionStatus::Success) {
						bank.save();
						funds_change_amount = 0.0;
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::PopStyleColor(3);

			// If error occurred, show error message to user
			if (transaction_status != bank_system::TransactionStatus::Success) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), GetTransactionErrorMessage(transaction_status).c_str());
			}

			ImGui::EndPopup();
		}
	}

	// Helper function for rendering the transfer overlay window
	static void RenderTransferWindowModal(bank_system::Bank& bank, bank_system::Account* logged_in_account, const std::string& window_name, const std::string& display_text) {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 0));

		static bank_system::TransactionStatus transaction_status{};

		if (ImGui::BeginPopupModal(window_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			static double transfer_amount;
			static std::string recipient_acc_name;
			ImGui::Text(display_text.c_str());
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::Text("£");
			ImGui::SameLine();
			ImGui::InputDouble("##transferamount", &transfer_amount, 0.0, 0.0, "%.2f");

			ImGui::Spacing();
			ImGui::InputTextWithHint("##recipient", "Recipient account name...", &recipient_acc_name);
			
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Cancel button (red)
			ImGui::PushStyleColor(ImGuiCol_Button, button_red_default);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_red_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_red_active);
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				transfer_amount = 0.0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			// Confirm button (green)
			ImGui::PushStyleColor(ImGuiCol_Button, button_green_default);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_green_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_green_active);
			if (ImGui::Button("Confirm", ImVec2(120, 0))) {
				if (transfer_amount > 0.0) {

					transaction_status = bank.transfer(logged_in_account->get_username(), recipient_acc_name, transfer_amount);

					if (transaction_status == bank_system::TransactionStatus::Success) {
						bank.save();
						transfer_amount = 0.0;
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::PopStyleColor(3);

			// If error occurred, show error message to user
			if (transaction_status != bank_system::TransactionStatus::Success) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), GetTransactionErrorMessage(transaction_status).c_str());
			}

			ImGui::EndPopup();
		}
	}

	static void RenderPasswordChangeModal(bank_system::Bank& bank, bank_system::Account* account) {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 0));

		if (ImGui::BeginPopupModal("Change Password", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			static std::string old_password;
			static std::string new_password;
			static std::string confirm_password;

			// UI error flags
			static bool show_mismatch_error = false;
			static bool show_incorrect_old_password_error = false;
			static bool show_empty_fields_error = false;

			// Clear state whenever popup is opened
			if (ImGui::IsWindowAppearing()) {
				old_password.clear();
				new_password.clear();
				confirm_password.clear();
				show_mismatch_error = false;
				show_incorrect_old_password_error = false;
				show_empty_fields_error = false;
			}

			ImGui::InputTextWithHint("##oldpasswordlabel", "Old password...", &old_password, ImGuiInputTextFlags_Password);
			ImGui::Spacing();
			ImGui::InputTextWithHint("##newpasswordlabel", "New password...", &new_password, ImGuiInputTextFlags_Password);
			ImGui::Spacing();
			ImGui::InputTextWithHint("##newpasswordconfirmlabel", "Confirm new password...", &confirm_password, ImGuiInputTextFlags_Password);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Cancel button (red)
			ImGui::PushStyleColor(ImGuiCol_Button, button_red_default);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_red_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_red_active);
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			// Confirm button (green)
			ImGui::PushStyleColor(ImGuiCol_Button, button_green_default);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_green_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_green_active);
			if (ImGui::Button("Confirm", ImVec2(120, 0))) {
				// Reset errors on submit attempt
				show_mismatch_error = false;
				show_incorrect_old_password_error = false;
				show_empty_fields_error = false;

				if (old_password.empty() || new_password.empty() || confirm_password.empty()) {
					show_empty_fields_error = true;
				}
				else if (new_password != confirm_password) {
					show_mismatch_error = true;
				}
				else if (bank.login(account->get_username(), old_password) == nullptr) {
					show_incorrect_old_password_error = true;
				}
				else if (account->change_password(old_password, new_password)) {
					bank.save();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::PopStyleColor(3);

			if (show_empty_fields_error) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not all password fields are filled!");
			}

			if (show_mismatch_error) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Password confirmation does not match!");
			}

			if (show_incorrect_old_password_error) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Old password is wrong!");
			}

			ImGui::EndPopup();
		}
	}

	static void RenderTransactionsTable(const bank_system::Account& acc, const bank_system::TransactionType& transac_type) {
		const auto& history = acc.get_history_by_type(transac_type);
		if (history.empty()) {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No transaction history found.");
			return;
		}

		int columns = 4; // type, amount, balance, time

		constexpr ImGuiTableFlags table_flags =
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_PadOuterX;

		// Get currency symbol
		const char* curr_symbol = bank_system::currency_symbols[static_cast<int>(selected_currency)];
		double exchange_rate_multiplier = bank_system::exchange_rates.at(selected_currency);

		if (ImGui::BeginTable("transactionstable", columns, table_flags)) {
			// Column headers
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Balance", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			// History data
			for (auto it = history.rbegin(); it != history.rend(); it++) {
				const bank_system::TransactionData& transaction = *it;

				// Handle currency exchange rate
				double display_balance = transaction._balance * exchange_rate_multiplier;
				double display_amount = transaction._amount * exchange_rate_multiplier;

				ImGui::TableNextRow();

				std::string type_str = bank_system::transac_type_to_string(transaction._type);

				// Type
				ImGui::TableNextColumn();
				ImGui::Text(type_str.c_str());

				// Amount (green incoming, red outgoing)
				ImGui::TableNextColumn();
				bool is_deposit = (type_str.find("Deposit") != std::string::npos || type_str.find("Transfer In") != std::string::npos);

				ImVec4 amount_color = is_deposit
					? ImVec4(0.35f, 0.85f, 0.35f, 1.0f)   // Soft green
					: ImVec4(0.90f, 0.40f, 0.40f, 1.0f);  // Soft red

				ImGui::TextColored(amount_color, "%s%s%.2f", is_deposit ? "+" : "-", curr_symbol, display_amount);

				// Balance
				ImGui::TableNextColumn();
				ImGui::Text("%s%.2f", curr_symbol, display_balance);

				// Time
				ImGui::TableNextColumn();
				std::time_t raw_time = std::chrono::system_clock::to_time_t(transaction._time);
				std::tm local_time{};
				localtime_s(&local_time, &raw_time);
				char time_buffer[32];
				std::strftime(time_buffer, sizeof(time_buffer), "%d/%m/%Y %H:%M:%S", &local_time);
				ImGui::Text(time_buffer);
			}

			ImGui::EndTable();
		}
	}

	static bool RenderCurrencySelector() {
		int current_index = static_cast<int>(selected_currency);

		ImGui::SetNextItemWidth(90.0f);
		if (ImGui::Combo("##CurrencySelector", &current_index, bank_system::currency_names, IM_COUNTOF(bank_system::currency_names))) {
			selected_currency = static_cast<bank_system::Currency>(current_index);
			return true; // Value changed this frame
		}

		return false;
	}

	void RenderBankTestsTab() {
		static bool run_main = false;
		static bool run_long = false;
		static bool run_database = false;

		// Only run "main" tests if checkbox checked
		ImGui::Checkbox("Run Main Tests", &run_main);

		// Only run long tests if checkbox checked
		ImGui::Checkbox("Run Long Tests", &run_long);

		// Only run database tests if checkbox checked
		ImGui::Checkbox("Run Database Tests", &run_database);

		if (ImGui::Button("Run Bank Tests")) {
			run_bank_tests(run_main, run_long, run_database);
		}
	}

	void RenderAllAccountsTable(const bank_system::Bank& bank) {
		const auto& accounts = bank.get_all_accounts();
		if (accounts.empty()) {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No accounts registered!");
			return;
		}

		int columns = 6; // username, password hash, real name, age, account type, balance

		constexpr ImGuiTableFlags table_flags =
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_PadOuterX;

		if (ImGui::BeginTable("accountstable", columns, table_flags)) {
			// Column headers
			ImGui::TableSetupColumn("Username", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("Pswd Hash", ImGuiTableColumnFlags_WidthFixed, 170.0f);
			ImGui::TableSetupColumn("Full Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Age", ImGuiTableColumnFlags_WidthFixed, 25.0f);
			ImGui::TableSetupColumn("Account Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("Balance", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			for (const auto& [username, acc] : accounts) {
				if (!acc) continue;

				ImGui::TableNextRow();

				// Username
				ImGui::TableNextColumn();
				ImGui::Text(acc->get_username().c_str());

				// Password hash
				ImGui::TableNextColumn();
				ImGui::TextDisabled(acc->get_psw_hash().c_str());

				// Full name
				ImGui::TableNextColumn();
				ImGui::Text(acc->get_leg_name().c_str());

				// Age
				ImGui::TableNextColumn();
				ImGui::Text("%i", acc->get_age());

				// Account type
				ImGui::TableNextColumn();
				ImGui::Text(bank_system::account_type_to_string(acc->get_type()).c_str());

				// Balance
				ImGui::TableNextColumn();
				ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.35f, 1.0f), "£%.2f", acc->get_balance());
			}
		}

		ImGui::EndTable();
	}

	void RenderAdminTab(SQLite::Database& db, bank_system::Bank& bank) {
		if (ImGui::Button("DELETE ALL USER DATA")) {
			database::clear_database(db, bank);
			database::clear_transac_data(db, bank);
		}

		if (ImGui::Button("DELETE TRANSACTIONS ONLY")) {
			database::clear_transac_data(db, bank);
		}

		RenderAllAccountsTable(bank);
	}

	void RenderUserTab(bank_system::Bank& bank) {
		static bank_system::Account* logged_in_account = nullptr;
		static bool logged_in = false;
		static bool log_in_failed = false;
		bool currency_changed_this_frame = false;
		static UserSubView activeView = UserSubView::None;

		ImGui::PushID("User Home Page Scope");

		if (!logged_in) {
			if (ImGui::Button("Create Account")) {
				activeView = UserSubView::CreateAccount;
			}
			ImGui::SameLine();
			if (ImGui::Button("Log In")) {
				activeView = UserSubView::LogIn;
			}
		}
		else {
			ImGui::Text("Logged in as %s", logged_in_account->get_username().c_str());
			ImGui::SameLine();

			// Log out button and state reset
			if (ImGui::Button("Log Out")) {
				logged_in_account = nullptr;
				logged_in = false;
				log_in_failed = false;
				activeView = UserSubView::LogIn;
			}

			ImGui::SameLine();
			currency_changed_this_frame = RenderCurrencySelector();
		}

		ImGui::PopID();

		ImGui::Separator();

		switch (activeView) {
		case UserSubView::CreateAccount: {
			ImGui::Text("Account Creation");

			static std::string accName = "";
			ImGui::InputTextWithHint("##accountnamelabel", "Account name...", &accName);

			static std::string realName = "";
			ImGui::InputTextWithHint("##realnamelabel", "Full name...", &realName);

			static int age = 0;
			ImGui::InputInt("##agelabel", &age);
			ImGui::SameLine();
			ImGui::Text("Age");

			static std::string password = "";
			ImGui::InputTextWithHint("##passwordnamelabel", "Password...", &password, ImGuiInputTextFlags_Password);

			static std::string confirmPass = "";
			ImGui::InputTextWithHint("##confirmpassnamelabel", "Confirm password...", &confirmPass, ImGuiInputTextFlags_Password);

			static bank_system::AccountType selectedType = bank_system::AccountType::Standard;
			const char* accountTypes[] = { "Standard", "Savings", "Junior" };
			static int item_current = 0;
			if (ImGui::Combo("Account Type", &item_current, accountTypes, IM_COUNTOF(accountTypes))) {
				selectedType = static_cast<bank_system::AccountType>(item_current);
			}

			static bool password_mismatch = false;

			if (ImGui::Button("Submit")) {
				if (password != confirmPass) {
					password_mismatch = true;
				}
				else {
					password_mismatch = false;
					bank.create_account(selectedType, accName, password, realName, age);

					accName.clear();
					realName.clear();
					age = 0;
					password.clear();
					confirmPass.clear();
					activeView = UserSubView::CreationSuccess;
				}
			}

			if (password_mismatch) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Passwords do not match!");
			}
			break;
		}

		case UserSubView::CreationSuccess: {
			ImGui::Text("Account successfully created!\nPlease click the button below to log in.");
			if (ImGui::Button("Go to log-in page")) {
				activeView = UserSubView::LogIn;
			}
			break;
		}

		case UserSubView::LogIn: {
			ImGui::PushID("User Log In Scope");

			ImGui::BeginDisabled(logged_in);

			static std::string accName = "";
			ImGui::InputTextWithHint("##loginnamelabel", "Account name...", &accName);

			static std::string password = "";
			ImGui::InputTextWithHint("##loginpasswordlabel", "Password...", &password, ImGuiInputTextFlags_Password);

			if (ImGui::Button("Log In")) {
				logged_in_account = bank.login(accName, password);

				if (logged_in_account != nullptr) {
					std::cout << "User \"" << accName << "\" successfully logged in!" << std::endl;
					logged_in = true;
					log_in_failed = false;
				}
				else {
					std::cout << "User \"" << accName << "\" incorrect login details!" << std::endl;
					logged_in = false;
					log_in_failed = true;
				}
			}

			ImGui::EndDisabled();

			if (logged_in) {
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "User \"%s\" successfully logged in!\nPress the button below to proceed to your Home Page.", accName.c_str());
				if (ImGui::Button("Home Page")) {
					activeView = UserSubView::AccountHome;
				}
			}
			else if (log_in_failed) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s login failed!\nAre your username and password correct?", accName.c_str());
			}

			ImGui::PopID();
			break;
		}

		case UserSubView::AccountHome: {
			if (logged_in_account == nullptr) {
				ImGui::Text("No active user session.");
				break;
			}

			static double acc_balance = 0.00;
			static bool has_fetched_balance = false;
			static bool show_transactions = false;
			static std::string show_transaction_button_text = "Show Transactions";

			ImGui::Text("User: %s", logged_in_account->get_username().c_str());
			ImGui::Text("Full Name: %s", logged_in_account->get_leg_name().c_str());
			ImGui::Text("Age: %i", logged_in_account->get_age());

			if (ImGui::Button("Get Balance") || currency_changed_this_frame) {
				acc_balance = logged_in_account->get_balance_in_currency(selected_currency);
				has_fetched_balance = true;
			}
			if (has_fetched_balance) {
				ImGui::SameLine();
				int currency_index = static_cast<int>(selected_currency);
				const char* currency_symbol = bank_system::currency_symbols[currency_index];
				ImGui::Text("%s%.2f", currency_symbol, acc_balance);
			}

			if (ImGui::Button("Deposit")) {
				ImGui::OpenPopup("Deposit Funds");
			}
			RenderFundsChangeModal(bank, logged_in_account, "Deposit Funds", "Enter amount to deposit:");

			if (ImGui::Button("Withdraw")) {
				ImGui::OpenPopup("Withdraw Funds");
			}
			RenderFundsChangeModal(bank, logged_in_account, "Withdraw Funds", "Enter amount to withdraw:");

			if (ImGui::Button("Transfer")) {
				ImGui::OpenPopup("Transfer Funds");
			}
			RenderTransferWindowModal(bank, logged_in_account, "Transfer Funds", "Enter amount to transfer:");

			if (ImGui::Button("Change Password")) {
				ImGui::OpenPopup("Change Password");
			}
			RenderPasswordChangeModal(bank, logged_in_account);

			if (ImGui::Button(show_transaction_button_text.c_str())) {
				if (show_transactions) {
					show_transactions = false;
					show_transaction_button_text = "Show Transactions";
				}
				else {
					show_transactions = true;
					show_transaction_button_text = "Hide Transactions";
				}
			}

			if (show_transactions) {
				// Adds a dropdown Combo to allow the user to filter transaction history by type
				static bank_system::TransactionType selected_type_filter = bank_system::TransactionType::All;

				ImGui::SetNextItemWidth(180.0f);
				if (ImGui::BeginCombo("##transactionfilter", bank_system::transac_type_to_string(selected_type_filter).c_str())) {
					for (int i = 0; i < static_cast<int>(bank_system::TransactionType::Count); i++) {
						auto value = static_cast<bank_system::TransactionType>(i);
						bool is_selected = (selected_type_filter == value);

						if (ImGui::Selectable(bank_system::transac_type_to_string(value).c_str(), is_selected)) {
							selected_type_filter = value;
						}

						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				RenderTransactionsTable(*logged_in_account, selected_type_filter);
			}

			break;
		}

		case UserSubView::None:
		default:
			ImGui::Text("Please select an option above.");
			break;
		}
	}

	void ui_loop(bank_system::Bank& bank, SQLite::Database& db) {
		GuiManager::BeginFrame();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

		ImGui::Begin("MainLayout", nullptr, windowFlags);

		if (ImGui::BeginTabBar("MainNavigationTabBar")) {
			if (ImGui::BeginTabItem("Bank System Tests")) {
				RenderBankTestsTab();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("User")) {
				RenderUserTab(bank);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Admin")) {
				RenderAdminTab(db, bank);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
		GuiManager::EndFrame();
	}
}