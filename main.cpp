#include "bank.h"
#include "banking_testing.h"
#include "constants.h"
#include "data_handler.h"
#include "imgui.h"
#include "GuiManager.h"
#include <iostream>
#include "simple_test.h"

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

				if (ImGui::Button("Create Account")) {
					activeView = UserSubView::CreateAccount;
				}

				ImGui::SameLine();

				if (ImGui::Button("Log In")) {
					activeView = UserSubView::LogIn;
				}

				ImGui::Separator();

				switch (activeView) {
					case UserSubView::CreateAccount: {
						ImGui::Text("Account Creation");

						// Account name
						static char accName[256] = "";
						ImGui::InputTextWithHint("##accountnamelabel", "Account name...", accName, IM_COUNTOF(accName));

						// Real name
						static char realName[256] = "";
						ImGui::InputTextWithHint("##realnamelabel", "Full name...", realName, IM_COUNTOF(realName));

						// Age
						static int age;
						ImGui::InputInt("##agelabel", &age);
						ImGui::SameLine();
						ImGui::Text("Age");

						// Password
						static char password[256] = "";
						ImGui::InputTextWithHint("##passwordnamelabel", "Password...", password, IM_COUNTOF(password));

						// Password confirmation
						static char confirmPass[256] = "";
						ImGui::InputTextWithHint("##confirmpassnamelabel", "Confirm password...", confirmPass, IM_COUNTOF(confirmPass));

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