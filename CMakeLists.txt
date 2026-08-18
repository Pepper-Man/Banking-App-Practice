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

# Automatically collect all .cpp source files inside the ImGui directory
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ImGui/*.cpp")

# Add source to this project's executable.
add_executable (BankingAppLatest 
	"banking_testing.h" 
	"simple_test.h" 
	"account.h" 
	"account.cpp" 
	"main.cpp"  
	"bank.h" 
	"bank.cpp" 
	 
	 
	"timer_util.h" 
	"savings_account.h" 
	"savings_account.cpp" 
	"constants.h" 
	"junior_account.h" 
	"junior_account.cpp" 
	 
	
	${IMGUI_SOURCES}
 "database.cpp" "database.h" "transaction.h")

target_include_directories(BankingAppLatest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ImGui")

# SQLiteCpp
include (FetchContent)

FetchContent_Declare(
	SQLiteCpp
	GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
	GIT_TAG 3.3.3
)

FetchContent_MakeAvailable(SQLiteCpp)
target_link_libraries(BankingAppLatest PRIVATE SQLiteCpp)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BankingAppLatest PROPERTY CXX_STANDARD 20)
endif()

if (MSVC)
	add_compile_options(/CMAKE_VS_JUST_MY_CODE_DEBUGGING)
	add_compile_options(/JMC)
endif()