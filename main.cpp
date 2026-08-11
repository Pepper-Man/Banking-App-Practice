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
			// ------------- BANK SYSTEM TESTS --------------------------------
			if (ImGui::BeginTabItem("Bank System Tests")) {
				static bool run_long = false;

				if (ImGui::Button("Run Bank Tests")) {
					run_bank_tests(run_long);
				}

				ImGui::SameLine();
				// Only run long tests if checkbox checked
				ImGui::Checkbox("Run Long Tests", &run_long);

				ImGui::EndTabItem();
			}
			// ----------------------------------------------------------------

			// ------------- USER AREA ----------------------------------------
			if (ImGui::BeginTabItem("User")) {
				enum class UserSubView {
					None,
					CreateAccount,
					LogIn,
					CreationSuccess,
					AccountHome
				};

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

						// Account name
						static std::string accName = "";
						ImGui::InputTextWithHint("##accountnamelabel", "Account name...", &accName);

						// Real name
						static std::string realName = "";
						ImGui::InputTextWithHint("##realnamelabel", "Full name...", &realName);

						// Age
						static int age;
						ImGui::InputInt("##agelabel", &age);
						ImGui::SameLine();
						ImGui::Text("Age");

						// Password
						static std::string password = "";
						ImGui::InputTextWithHint("##passwordnamelabel", "Password...", &password);

						// Password confirmation
						static std::string confirmPass = "";
						ImGui::InputTextWithHint("##confirmpassnamelabel", "Confirm password...", &confirmPass);

						// Account type
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

						// Error text for password mismatch
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

						// Disable the three input widgets when the user is logged in
						ImGui::BeginDisabled(logged_in);

						// Account name
						static std::string accName = "";
						ImGui::InputTextWithHint("##loginnamelabel", "Account name...", &accName);

						// Password
						static std::string password = "";
						ImGui::InputTextWithHint("##loginpasswordlabel", "Password...", &password);

						// Log in logic
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

						// Show login success/failure text to user, and home page button if successful
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

						ImGui::Text("User: %s", logged_in_account->get_username().c_str());
						break;
					}	
					case UserSubView::None:
					default:
						ImGui::Text("Please select an option above.");
						break;
				}

				

				ImGui::EndTabItem();
			}
			// ----------------------------------------------------------------

			// ------------- ADMIN AREA ---------------------------------------
			if (ImGui::BeginTabItem("Admin")) {

				if (ImGui::Button("DELETE ALL USER DATA")) {
					bank_system::clear_saved_data();
					bank_system::clear_transac_data();
				}

				ImGui::EndTabItem();
			}
			// ----------------------------------------------------------------

			ImGui::EndTabBar();
		}

		ImGui::End();
		GuiManager::EndFrame();
	}

	GuiManager::Shutdown();
	return 0;
}