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

		// BANK SYSTEM TESTS
		// Only run long tests if checkbox checked
		static bool run_long = false;
		ImGui::Checkbox("Run Long Tests", &run_long);

		if (ImGui::Button("Run Bank Tests")) {
			run_bank_tests(run_long);
		}

		GuiManager::EndFrame();
	}

	GuiManager::Shutdown();
	return 0;
}