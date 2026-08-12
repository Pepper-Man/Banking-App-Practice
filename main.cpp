#include "account.h"
#include "bank.h"
#include "banking_testing.h"
#include "constants.h"
#include "data_handler.h"
#include "imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "GuiManager.h"
#include <iostream>
#include "simple_test.h"
#include <string>
#include "Windows.h"

// Forward declaration for tab renderers
void RenderBankTestsTab();
void RenderUserTab(bank_system::Bank& bank);
void RenderAdminTab();

int main() {
	// Need this to force windows to make the program DPI aware so the font isn't blurry
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Initialise UI window
	if (!GuiManager::Init("Banking System", 500, 350)) {
		return -1;
	}

	std::cout << "Running UI..." << std::endl;

	bank_system::Bank bank;

	// UI loop
	while (GuiManager::IsRunning()) {
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
				RenderAdminTab();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
		GuiManager::EndFrame();
	}

	GuiManager::Shutdown();
	return 0;
}

void RenderBankTestsTab() {
	static bool run_long = false;

	if (ImGui::Button("Run Bank Tests")) {
		run_bank_tests(run_long);
	}

	ImGui::SameLine();
	// Only run long tests if checkbox checked
	ImGui::Checkbox("Run Long Tests", &run_long);
}

void RenderAdminTab() {
	if (ImGui::Button("DELETE ALL USER DATA")) {
		bank_system::clear_saved_data();
		bank_system::clear_transac_data();
	}
}

enum class UserSubView {
	None,
	CreateAccount,
	LogIn,
	CreationSuccess,
	AccountHome
};

// Helper function for rendering the deposit/withdrawal overlay window
void RenderFundsChangeModal(bank_system::Bank& bank, bank_system::Account* logged_in_account, const std::string& window_name, const std::string& display_text) {
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(320, 0)); // Fixed 320px width, auto-fit height

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
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.18f, 0.18f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.25f, 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.12f, 0.12f, 1.0f));
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			funds_change_amount = 0.0;
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		// Confirm button (green)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.22f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.68f, 0.28f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.42f, 0.16f, 1.0f));
		if (ImGui::Button("Confirm", ImVec2(120, 0))) {
			if (funds_change_amount > 0.0) {
				if (window_name == "Deposit Funds") {
					logged_in_account->deposit(funds_change_amount);
					bank.save();
				}
				else if (window_name == "Withdraw Funds") {
					logged_in_account->withdraw(funds_change_amount);
					bank.save();
				}
				else {
					std::cout << "Unknown fund change type!" << std::endl;
				}

				funds_change_amount = 0.0; // Reset
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::PopStyleColor(3);

		ImGui::EndPopup();
	}
}

void RenderUserTab(bank_system::Bank& bank) {
	static bank_system::Account* logged_in_account = nullptr;
	static bool logged_in = false;
	static bool log_in_failed = false;
	static UserSubView activeView = UserSubView::None;

	ImGui::PushID("User Home Page Scope");
	if (ImGui::Button("Create Account")) {
		activeView = UserSubView::CreateAccount;
	}
	ImGui::SameLine();
	if (ImGui::Button("Log In")) {
		activeView = UserSubView::LogIn;
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
				bank.save();

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

		ImGui::Text("User: %s", logged_in_account->get_username().c_str());
		ImGui::Text("Full Name: %s", logged_in_account->get_leg_name().c_str());
		ImGui::Text("Age: %i", logged_in_account->get_age());

		if (ImGui::Button("Get Balance")) {
			acc_balance = logged_in_account->get_balance();
			has_fetched_balance = true;
		}
		if (has_fetched_balance) {
			ImGui::SameLine();
			ImGui::Text("£%.2f", acc_balance);
		}

		if (ImGui::Button("Deposit")) {
			ImGui::OpenPopup("Deposit Funds");
		}
		RenderFundsChangeModal(bank, logged_in_account, "Deposit Funds", "Enter amount to deposit :");

		if (ImGui::Button("Withdraw")) {
			ImGui::OpenPopup("Withdraw Funds");
		}
		RenderFundsChangeModal(bank, logged_in_account, "Withdraw Funds", "Enter amount to withdraw :");

		break;
	}

	case UserSubView::None:
	default:
		ImGui::Text("Please select an option above.");
		break;
	}
}