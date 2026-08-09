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

int main() {
	// Initialise UI window
	if (!GuiManager::Init("Banking System", 400, 300)) {
		return -1;
	}

	std::cout << "Running UI..." << std::endl;

	// UI loop
	while (GuiManager::IsRunning()) {
		GuiManager::BeginFrame();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

		ImGui::Begin("MainLayout", nullptr, windowFlags);

		bank_system::Bank bank;

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
					CreationSuccess
				};

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

						if (ImGui::Button("Submit")) {
							// TODO: Add verifcation logic

							bank.create_account(selectedType, accName, password, realName, age);
							bank.save();

							accName[0] = '\0'; // Clear when done
							activeView = UserSubView::CreationSuccess;
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

						// Account name
						static std::string accName = "";
						ImGui::InputTextWithHint("##loginnamelabel", "Account name...", &accName);

						// Password
						static std::string password = "";
						ImGui::InputTextWithHint("##loginpasswordlabel", "Password...", &password);

						// Log in
						if (ImGui::Button("Log In")) {
							bank_system::Account* acc = bank.login(accName, password);

							if (acc != nullptr) {
								std::cout << "User \"" << accName << "\" successfully logged in!" << std::endl;
							}
							else {
								std::cout << "User \"" << accName << "\" incorrect login details!" << std::endl;
							}
						}
						
						ImGui::PopID();

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