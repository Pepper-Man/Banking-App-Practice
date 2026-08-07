#include "banking_testing.h"
#include "imgui.h"
#include "GuiManager.h"
#include <iostream>

int main() {
	// Initialise UI window
	if (!GuiManager::Init("Banking System", 400, 300)) {
		return -1;
	}

	std::cout << "Running UI..." << std::endl;

	// UI loop
	while (GuiManager::IsRunning()) {
		GuiManager::BeginFrame();


		GuiManager::EndFrame();
	}

	// Run bank stuff
	get_going();

	GuiManager::Shutdown();
	return 0;
}