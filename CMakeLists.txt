# CMakeList.txt : CMake project for BankingAppLatest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("BankingAppLatest")

# Add source to this project's executable.
add_executable (BankingAppLatest "banking_testing.h" "simple_test.h" "account.h" "account.cpp" "main.cpp"  "bank.h" "bank.cpp" "data_handler.h" "data_handler.cpp" "timer_util.h" "savings_account.h" "savings_account.cpp" "constants.h" "junior_account.h" "junior_account.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BankingAppLatest PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
