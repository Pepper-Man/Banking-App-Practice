#include "banking_testing.h"
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

				ImGui::EndTabItem();
			}
			// ----------------------------------------------------------------

			// ------------- ADMIN AREA ---------------------------------------
			if (ImGui::BeginTabItem("Admin")) {

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