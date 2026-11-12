#include "ATM.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        bank::ATM atm("accounts.txt", "transactions.txt", "audit.txt");
        atm.run(std::cin, std::cout);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL ERROR] " << ex.what() << '\n';
        return 1;
    }
}

