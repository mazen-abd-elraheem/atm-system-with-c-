cmake_minimum_required(VERSION 3.16)
project(BankAccountATMSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bank_core STATIC
    src/Utils.cpp
    src/Transaction.cpp
    src/AuditLogger.cpp
    src/Account.cpp
    src/SavingsAccount.cpp
    src/CheckingAccount.cpp
    src/ATM.cpp
)

target_include_directories(bank_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(bank_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(bank_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(atm_simulator
    src/main.cpp
)
target_link_libraries(atm_simulator PRIVATE bank_core)

enable_testing()
add_executable(test_bank_system
    tests/test_bank_system.cpp
)
target_link_libraries(test_bank_system PRIVATE bank_core)
add_test(NAME BankATMSystemTests COMMAND test_bank_system)

