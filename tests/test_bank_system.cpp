#include "ATM.hpp"
#include "CheckingAccount.hpp"
#include "Exceptions.hpp"
#include "SavingsAccount.hpp"
#include "Utils.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

struct TestDirGuard {
    std::filesystem::path dir;
    explicit TestDirGuard(const std::string& name) {
        dir = std::filesystem::temp_directory_path() / ("bank_atm_test_" + name);
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
    }
    ~TestDirGuard() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Assertion failed: " + message);
    }
}

template <typename ExceptionType, typename Fn>
void requireThrows(Fn&& fn, const std::string& message) {
    bool caught = false;
    try {
        fn();
    } catch (const ExceptionType&) {
        caught = true;
    } catch (const std::exception& ex) {
        throw std::runtime_error("Assertion failed (" + message + "): caught wrong exception: " + ex.what());
    }
    if (!caught) {
        throw std::runtime_error("Assertion failed (" + message + "): expected exception was not thrown");
    }
}

std::string readFileText(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

void testCurrencyAndInputValidation() {
    require(bank::utils::formatCurrency(1500.0) == "1,500.00 EGP", "format 1500");
    require(bank::utils::formatCurrency(0.0) == "0.00 EGP", "format 0");
    require(bank::utils::formatCurrency(-0.001) == "0.00 EGP", "avoid -0.00");
    require(bank::utils::formatCurrency(-335.5) == "-335.50 EGP", "format negative");
    require(bank::utils::formatCurrency(999.999) == "1,000.00 EGP", "rounding carry");
    require(bank::utils::formatCurrency(1234567.89) == "1,234,567.89 EGP", "millions grouping");

    requireThrows<bank::InvalidInputException>(
        [] { bank::utils::parseDoubleStrict("abc"); }, "non-numeric text");
    requireThrows<bank::InvalidInputException>(
        [] { bank::utils::parseDoubleStrict("120xyz"); }, "trailing characters");
    requireThrows<bank::InvalidInputException>(
        [] { bank::utils::parseDoubleStrict(""); }, "empty string");
    requireThrows<bank::InvalidInputException>(
        [] { bank::utils::parseDoubleStrict("NaN"); }, "NaN rejected");
    requireThrows<bank::InvalidAmountException>(
        [] { bank::utils::parsePositiveAmount("-25"); }, "negative amount");
    requireThrows<bank::InvalidAmountException>(
        [] { bank::utils::parsePositiveAmount("0"); }, "zero amount");
    require(std::abs(bank::utils::parsePositiveAmount("  250.75 ") - 250.75) < 1e-9, "trimmed positive parse");
}

void testPolymorphismAndOperators() {
    bank::SavingsAccount sav("1001", "Alice", 2000.0, "1234", 500.0, 0.05);
    bank::CheckingAccount chk("2001", "Bob", 1500.0, "4321", 1000.0, 35.0);
    bank::SavingsAccount savSameNum("1001", "Alice Copy", 900.0, "9999", 500.0, 0.05);

    // Operator == / != on account number
    require(sav == savSameNum, "== compares account number");
    require(sav != chk, "!= detects different account numbers");

    // Operator < / > / <= / >= on balance
    require(chk < sav, "< compares balances");
    require(sav > chk, "> compares balances");
    require(sav >= savSameNum, ">= compares balances");

    // Operator << uses polymorphic formatSummary with EGP formatting
    std::ostringstream ossSav;
    std::ostringstream ossChk;
    const bank::Account& polySav = sav;
    const bank::Account& polyChk = chk;
    ossSav << polySav;
    ossChk << polyChk;
    require(ossSav.str().find("[Savings]") != std::string::npos, "Savings summary tag");
    require(ossSav.str().find("2,000.00 EGP") != std::string::npos, "Savings formatted balance");
    require(ossChk.str().find("[Checking]") != std::string::npos, "Checking summary tag");
    require(ossChk.str().find("1,500.00 EGP") != std::string::npos, "Checking formatted balance");

    // Savings minimum balance rule & interest
    requireThrows<bank::InsufficientFundsException>(
        [&] { sav.withdraw(1501.0); }, "Savings cannot go below 500.00");
    sav.withdraw(1500.0); // exactly at 500.00 floor
    require(std::abs(sav.getBalance() - 500.0) < 1e-9, "Savings balance exactly at floor");

    const double earned = sav.applyInterest(1.0); // 5% of 500 = 25.00
    require(std::abs(earned - 25.0) < 1e-9, "Savings interest earned");
    require(std::abs(sav.getBalance() - 525.0) < 1e-9, "Savings balance after interest");

    // Checking overdraft + flat fee rule
    chk.withdraw(1500.0); // goes to 0.00 without overdraft fee
    require(std::abs(chk.getBalance() - 0.0) < 1e-9, "Checking exact zero without fee");
    require(std::abs(chk.getLastFeeCharged()) < 1e-9, "No fee when balance stays >= 0");

    chk.withdraw(300.0); // triggers 35.00 fee -> -335.00
    require(std::abs(chk.getLastFeeCharged() - 35.0) < 1e-9, "Overdraft fee recorded");
    require(std::abs(chk.getBalance() - (-335.0)) < 1e-9, "Checking balance after overdraft + fee");

    // Exceeding overdraft limit (-1000.00) including fee
    requireThrows<bank::InsufficientFundsException>(
        [&] { chk.withdraw(640.0); }, "-335 - 640 - 35 = -1010 breaches -1000 limit");
    require(std::abs(chk.getBalance() - (-335.0)) < 1e-9, "Balance unchanged after failed overdraft");
}

void testPinLockoutPersistenceAndUnlock() {
    TestDirGuard guard("lockout");
    const auto accFile = guard.dir / "accounts.txt";
    const auto txFile = guard.dir / "transactions.txt";
    const auto auditFile = guard.dir / "audit.txt";

    {
        bank::ATM atm(accFile.string(), txFile.string(), auditFile.string());
        atm.initialize();

        requireThrows<bank::InvalidPinException>(
            [&] { atm.login("1001", "0000"); }, "Wrong PIN attempt 1");
        requireThrows<bank::InvalidPinException>(
            [&] { atm.login("1001", "0001"); }, "Wrong PIN attempt 2");
        requireThrows<bank::AccountLockedException>(
            [&] { atm.login("1001", "0002"); }, "Wrong PIN attempt 3 locks account");

        requireThrows<bank::AccountLockedException>(
            [&] { atm.login("1001", "1234"); }, "Valid PIN rejected while locked");
    }

    // Reload from disk and verify lock persisted
    {
        bank::ATM atmReloaded(accFile.string(), txFile.string(), auditFile.string());
        atmReloaded.initialize();

        bank::Account* acc = atmReloaded.findAccount("1001");
        require(acc != nullptr && acc->isLocked(), "Lock state persisted across restarts");

        requireThrows<bank::AccountLockedException>(
            [&] { atmReloaded.login("1001", "1234"); }, "Locked state persists after reload");

        requireThrows<bank::InvalidPinException>(
            [&] { atmReloaded.unlockAccount("1001", "0000"); }, "Wrong Admin PIN rejected");

        atmReloaded.unlockAccount("1001", bank::ATM::DEFAULT_ADMIN_PIN);
        bank::Account& loggedIn = atmReloaded.login("1001", "1234");
        require(!loggedIn.isLocked(), "Account unlocked and login succeeds");
    }

    const std::string auditText = readFileText(auditFile);
    require(auditText.find("[ACCOUNT_LOCKED]") != std::string::npos, "Audit log records lockout");
    require(auditText.find("[ACCOUNT_UNLOCKED]") != std::string::npos, "Audit log records unlock");
    require(auditText.find("[SUCCESS] [LOGIN]") != std::string::npos, "Audit log records login success");
}

void testTransfersMiniStatementAndPersistence() {
    TestDirGuard guard("transfer");
    const auto accFile = guard.dir / "accounts.txt";
    const auto txFile = guard.dir / "transactions.txt";
    const auto auditFile = guard.dir / "audit.txt";

    {
        bank::ATM atm(accFile.string(), txFile.string(), auditFile.string());
        atm.initialize();

        bank::Account& sav = atm.login("1001", "1234");
        bank::Account* chk = atm.findAccount("2001");
        require(chk != nullptr, "Seeded checking account exists");

        // Self transfer fails
        requireThrows<bank::InvalidInputException>(
            [&] { atm.transfer(sav, "1001", 100.0); }, "Self-transfer rejected");

        // Excessive transfer fails atomically
        requireThrows<bank::InsufficientFundsException>(
            [&] { atm.transfer(sav, "2001", 4600.0); }, "Savings floor enforced on transfer");
        require(std::abs(sav.getBalance() - 5000.0) < 1e-9, "Source unchanged on failed transfer");
        require(std::abs(chk->getBalance() - 1500.0) < 1e-9, "Target unchanged on failed transfer");

        // Valid transfer succeeds
        atm.transfer(sav, "2001", 1000.0);
        require(std::abs(sav.getBalance() - 4000.0) < 1e-9, "Source debited on transfer");
        require(std::abs(chk->getBalance() - 2500.0) < 1e-9, "Target credited on transfer");

        // Create new account & duplicate check
        atm.createCheckingAccount("3001", "Nour Adel", "7777", 200.0);
        requireThrows<bank::DuplicateAccountException>(
            [&] { atm.createCheckingAccount("3001", "Dup User", "8888", 100.0); },
            "Duplicate account number rejected");

        // Checking overdraft via transfer logs overdraft fee + transfer
        bank::Account& chkLogged = atm.login("2001", "4321");
        atm.transfer(chkLogged, "3001", 2700.0); // 2500 - 2700 - 35 = -235.00
        require(std::abs(chkLogged.getBalance() - (-235.0)) < 1e-9, "Checking overdraft transfer balance");
    }

    // Reload and verify mini-statement and balances persisted
    {
        bank::ATM atm2(accFile.string(), txFile.string(), auditFile.string());
        atm2.initialize();

        bank::Account* chk2 = atm2.findAccount("2001");
        bank::Account* acc3001 = atm2.findAccount("3001");
        require(chk2 != nullptr && acc3001 != nullptr, "Accounts loaded from disk");
        require(std::abs(chk2->getBalance() - (-235.0)) < 1e-9, "Overdraft balance persisted");
        require(std::abs(acc3001->getBalance() - 2900.0) < 1e-9, "New account balance persisted");

        const auto mini = atm2.getMiniStatement("2001", 10);
        require(mini.size() == 4, "Expected 4 transaction rows for #2001 (open, in, out, fee)");
        require(mini.back().type == "OVERDRAFT_FEE", "Latest transaction on #2001 is OVERDRAFT_FEE");
    }
}

void testInteractiveConsoleScript() {
    TestDirGuard guard("interactive");
    const auto accFile = guard.dir / "accounts.txt";
    const auto txFile = guard.dir / "transactions.txt";
    const auto auditFile = guard.dir / "audit.txt";

    std::istringstream inputScript(
        "not_a_number\n" // invalid main menu input -> handled cleanly
        "1\n"            // Login
        "1001\n"         // Account 1001
        "1111\n"         // Wrong PIN 1
        "2222\n"         // Wrong PIN 2
        "3333\n"         // Wrong PIN 3 -> locks account
        "4\n"            // Admin Unlock
        "1001\n"         // Account 1001
        "9999\n"         // Admin PIN
        "1\n"            // Login again
        "1001\n"
        "1234\n"         // Valid PIN
        "1\n"            // View Full Account Details
        "2\n"            // Deposit
        "abc\n"          // Non-numeric deposit -> handled cleanly
        "2\n"            // Deposit again
        "-50\n"          // Negative deposit -> handled cleanly
        "2\n"            // Deposit valid
        "500\n"          // +500 -> 5500
        "3\n"            // Withdraw
        "5200\n"         // Violates min balance 500 -> rejected
        "4\n"            // Transfer
        "2001\n"         // To 2001
        "500\n"          // 5500 -> 5000
        "5\n"            // Mini-Statement
        "6\n"            // Interest menu
        "2\n"            // Apply annual interest (+250 -> 5250)
        "7\n"            // Logout
        "3\n"            // Compare two accounts
        "1001\n"
        "2001\n"
        "5\n"            // Exit
    );

    std::ostringstream output;
    bank::ATM atm(accFile.string(), txFile.string(), auditFile.string());
    atm.run(inputScript, output);

    const std::string outText = output.str();
    require(outText.find("Invalid integer input") != std::string::npos, "Caught non-numeric menu input");
    require(outText.find("has been locked after 3 consecutive failed PIN attempts") != std::string::npos,
            "Locked after 3 wrong PINs");
    require(outText.find("Account #1001 has been unlocked") != std::string::npos, "Admin unlocked account");
    require(outText.find("Invalid numeric input") != std::string::npos, "Caught non-numeric amount");
    require(outText.find("Amount must be greater than zero") != std::string::npos, "Caught negative amount");
    require(outText.find("violates the minimum balance requirement") != std::string::npos,
            "Caught insufficient funds");
    require(outText.find("Transfer successful") != std::string::npos, "Interactive transfer succeeded");
    require(outText.find("Annual interest credited: 250.00 EGP") != std::string::npos,
            "Interactive annual interest credited");
    require(outText.find("5,250.00 EGP") != std::string::npos, "Updated balance displayed in EGP");
    require(outText.find("Goodbye!") != std::string::npos, "Clean exit");

    const std::string auditText = readFileText(auditFile);
    require(auditText.find("[FAILED] [VALIDATION_ERROR]") != std::string::npos, "Validation errors logged");
    require(auditText.find("[FAILED] [DEPOSIT]") != std::string::npos, "Failed deposit logged");
    require(auditText.find("[FAILED] [WITHDRAW]") != std::string::npos, "Failed withdraw logged");
    require(auditText.find("[SUCCESS] [TRANSFER]") != std::string::npos, "Successful transfer logged");
}

} // namespace

int main() {
    try {
        testCurrencyAndInputValidation();
        testPolymorphismAndOperators();
        testPinLockoutPersistenceAndUnlock();
        testTransfersMiniStatementAndPersistence();
        testInteractiveConsoleScript();
        std::cout << "ALL TESTS PASSED (5/5 test suites).\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "TEST FAILURE: " << ex.what() << '\n';
        return 1;
    }
}

