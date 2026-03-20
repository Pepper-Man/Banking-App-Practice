#include <chrono>
#include <iostream>
#include <string>

struct TestTimer {
	std::string name;
	std::chrono::high_resolution_clock::time_point start;

	TestTimer(std::string test_name) : name(test_name), start(std::chrono::high_resolution_clock::now()) {}

	~TestTimer() {
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

		std::cout << "[TIMER] " << name << " took " << duration << " microseconds (" << duration / 1000.0 << " ms)" << std::endl;
	}
};