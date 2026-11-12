#include "ATM.hpp"
#include "CheckingAccount.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace bank {

namespace {
void ensureParentDirectory(const std::string& filePath) {
    const std::filesystem::path path(filePath);
    if (!path.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
}
} // namespace

ATM::ATM(std::string accountsFilePath,
         std::string transactionsFilePath,
         std::string auditFilePath,
         std::string adminPin)
    : accountsFilePath_(std::move(accountsFilePath)),
      transactionsFilePath_(std::move(transactionsFilePath)),
      logger_(std::move(auditFilePath)),
      adminPin_(std::move(adminPin)),
      seededOnInit_(false) {}

void ATM::initialize() {
    accounts_.clear();
    transactions_.clear();
    seededOnInit_ = false;

    if (std::filesystem::exists(accountsFilePath_)) {
        loadAccounts();
        if (std::filesystem::exists(transactionsFilePath_)) {
            loadTransactions();
        } else {
            saveTransactions();
        }
        logger_.log("SYSTEM_INIT", "SYSTEM", true,
                    "Loaded " + std::to_string(accounts_.size()) + " account(s) from disk.");
    } else {
        seedDefaultAccounts();
        seededOnInit_ = true;
    }
}

void ATM::seedDefaultAccounts() {
    accounts_.push_back(std::make_unique<SavingsAccount>(
        "1001", "Ahmed Hassan", 5000.00, "1234",
        SavingsAccount::DEFAULT_MINIMUM_BALANCE,
        SavingsAccount::DEFAULT_ANNUAL_INTEREST_RATE));
    addTransaction("1001", "ACCOUNT_OPENED", 5000.00, 5000.00, "Initial seeded Savings account");

    accounts_.push_back(std::make_unique<CheckingAccount>(
        "2001", "Sara Mohamed", 1500.00, "4321",
        CheckingAccount::DEFAULT_OVERDRAFT_LIMIT,
        CheckingAccount::DEFAULT_OVERDRAFT_FEE));
    addTransaction("2001", "ACCOUNT_OPENED", 1500.00, 1500.00, "Initial seeded Checking account");

    accounts_.push_back(std::make_unique<SavingsAccount>(
        "1002", "Omar Khaled", 2500.00, "5678",
        SavingsAccount::DEFAULT_MINIMUM_BALANCE,
        SavingsAccount::DEFAULT_ANNUAL_INTEREST_RATE));
    addTransaction("1002", "ACCOUNT_OPENED", 2500.00, 2500.00, "Initial seeded Savings account");

    saveAccounts();
    saveTransactions();
    logger_.log("SYSTEM_INIT", "SYSTEM", true, "Seeded default sample accounts (1001, 2001, 1002).");
}

void ATM::loadAccounts() {
    std::ifstream in(accountsFilePath_);
    if (!in.is_open()) {
        throw PersistenceException("Failed to open accounts file for reading: " + accountsFilePath_);
    }

    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        const std::string trimmed = utils::trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        try {
            auto account = deserializeAccount(trimmed);
            ensureUniqueAccount(*account);
            accounts_.push_back(std::move(account));
        } catch (const BankException& ex) {
            throw PersistenceException(
                "Error loading account at line " + std::to_string(lineNumber) + ": " + ex.what());
        }
    }
}

void ATM::loadTransactions() {
    std::ifstream in(transactionsFilePath_);
    if (!in.is_open()) {
        throw PersistenceException("Failed to open transactions file for reading: " + transactionsFilePath_);
    }

    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        const std::string trimmed = utils::trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        try {
            transactions_.push_back(TransactionRecord::deserialize(trimmed));
        } catch (const BankException& ex) {
            throw PersistenceException(
                "Error loading transaction at line " + std::to_string(lineNumber) + ": " + ex.what());
        }
    }
}

std::unique_ptr<Account> ATM::deserializeAccount(const std::string& line) const {
    const auto fields = utils::splitDelimited(line, '|');
    if (fields.size() != 9) {
        throw PersistenceException("Malformed account record.");
    }

    const std::string& type = fields[0];
    const std::string& accNum = fields[1];
    const std::string& holder = fields[2];
    const double balance = utils::parseDoubleStrict(fields[3]);
    const std::string& pin = fields[4];
    const int failedAttempts = utils::parseIntStrict(fields[5]);
    const bool locked = utils::parseIntStrict(fields[6]) != 0;
    const double paramOne = utils::parseDoubleStrict(fields[7]);
    const double paramTwo = utils::parseDoubleStrict(fields[8]);

    if (type == "SAVINGS") {
        return std::make_unique<SavingsAccount>(
            accNum, holder, balance, pin, paramOne, paramTwo, failedAttempts, locked);
    }
    if (type == "CHECKING") {
        return std::make_unique<CheckingAccount>(
            accNum, holder, balance, pin, paramOne, paramTwo, failedAttempts, locked);
    }
    throw PersistenceException("Unknown account type in file: " + type);
}

void ATM::saveAccounts() const {
    ensureParentDirectory(accountsFilePath_);
    std::ofstream out(accountsFilePath_, std::ios::trunc);
    if (!out.is_open()) {
        throw PersistenceException("Failed to open accounts file for writing: " + accountsFilePath_);
    }

    for (const auto& account : accounts_) {
        out << account->serialize() << '\n';
    }
}

void ATM::saveTransactions() const {
    ensureParentDirectory(transactionsFilePath_);
    std::ofstream out(transactionsFilePath_, std::ios::trunc);
    if (!out.is_open()) {
        throw PersistenceException("Failed to open transactions file for writing: " + transactionsFilePath_);
    }

    for (const auto& record : transactions_) {
        out << record.serialize() << '\n';
    }
}

void ATM::ensureUniqueAccount(const Account& candidate) const {
    const bool exists = std::any_of(accounts_.begin(), accounts_.end(),
                                    [&](const std::unique_ptr<Account>& existing) {
                                        return *existing == candidate;
                                    });
    if (exists) {
        throw DuplicateAccountException(
            "Account number #" + candidate.getAccountNumber() + " already exists.");
    }
}

void ATM::addTransaction(const std::string& accountNumber,
                         const std::string& type,
                         double amount,
                         double balanceAfter,
                         const std::string& details) {
    transactions_.push_back(TransactionRecord{
        utils::currentTimestamp(),
        accountNumber,
        type,
        amount,
        balanceAfter,
        details
    });
}

Account* ATM::findAccount(const std::string& accountNumber) const {
    const std::string cleaned = utils::trim(accountNumber);
    for (const auto& account : accounts_) {
        if (account->getAccountNumber() == cleaned) {
            return account.get();
        }
    }
    return nullptr;
}

Account& ATM::login(const std::string& accountNumber, const std::string& pin) {
    const std::string cleanedAcc = utils::trim(accountNumber);
    Account* account = findAccount(cleanedAcc);
    if (!account) {
        logger_.log("LOGIN", cleanedAcc, false, "Account not found.");
        throw AccountNotFoundException("Account #" + cleanedAcc + " was not found.");
    }

    try {
        account->authenticate(pin);
        saveAccounts();
        logger_.log("LOGIN", account->getAccountNumber(), true,
                    "Authenticated holder: " + account->getAccountHolderName());
        return *account;
    } catch (const AccountLockedException& ex) {
        saveAccounts();
        logger_.log("ACCOUNT_LOCKED", account->getAccountNumber(), false, ex.what());
        throw;
    } catch (const InvalidPinException& ex) {
        saveAccounts();
        logger_.log("LOGIN", account->getAccountNumber(), false, ex.what());
        throw;
    }
}

Account& ATM::createSavingsAccount(const std::string& accountNumber,
                                   const std::string& holderName,
                                   const std::string& pin,
                                   double initialDeposit,
                                   double minimumBalance,
                                   double annualInterestRate) {
    try {
        if (initialDeposit <= 0.0) {
            throw InvalidAmountException("Initial deposit must be greater than zero.");
        }
        auto account = std::make_unique<SavingsAccount>(
            accountNumber, holderName, initialDeposit, pin, minimumBalance, annualInterestRate);
        ensureUniqueAccount(*account);

        Account& ref = *account;
        accounts_.push_back(std::move(account));
        addTransaction(ref.getAccountNumber(), "ACCOUNT_OPENED", initialDeposit,
                       ref.getBalance(), "New Savings account created");
        saveAccounts();
        saveTransactions();
        logger_.log("CREATE_ACCOUNT", ref.getAccountNumber(), true,
                    "Created Savings account for " + ref.getAccountHolderName() +
                    " with initial deposit " + utils::formatCurrency(initialDeposit));
        return ref;
    } catch (const BankException& ex) {
        logger_.log("CREATE_ACCOUNT", utils::trim(accountNumber), false, ex.what());
        throw;
    }
}

Account& ATM::createCheckingAccount(const std::string& accountNumber,
                                    const std::string& holderName,
                                    const std::string& pin,
                                    double initialDeposit,
                                    double overdraftLimit,
                                    double overdraftFee) {
    try {
        if (initialDeposit < 0.0) {
            throw InvalidAmountException("Initial deposit cannot be negative.");
        }
        auto account = std::make_unique<CheckingAccount>(
            accountNumber, holderName, initialDeposit, pin, overdraftLimit, overdraftFee);
        ensureUniqueAccount(*account);

        Account& ref = *account;
        accounts_.push_back(std::move(account));
        addTransaction(ref.getAccountNumber(), "ACCOUNT_OPENED", initialDeposit,
                       ref.getBalance(), "New Checking account created");
        saveAccounts();
        saveTransactions();
        logger_.log("CREATE_ACCOUNT", ref.getAccountNumber(), true,
                    "Created Checking account for " + ref.getAccountHolderName() +
                    " with initial deposit " + utils::formatCurrency(initialDeposit));
        return ref;
    } catch (const BankException& ex) {
        logger_.log("CREATE_ACCOUNT", utils::trim(accountNumber), false, ex.what());
        throw;
    }
}

void ATM::deposit(Account& account, double amount) {
    if (account.isLocked()) {
        logger_.log("DEPOSIT", account.getAccountNumber(), false, "Attempted deposit on locked account.");
        throw AccountLockedException("Cannot deposit into locked account #" + account.getAccountNumber() + ".");
    }
    try {
        account.deposit(amount);
        addTransaction(account.getAccountNumber(), "DEPOSIT", amount,
                       account.getBalance(), "Cash deposit");
        saveAccounts();
        saveTransactions();
        logger_.log("DEPOSIT", account.getAccountNumber(), true,
                    "Deposited " + utils::formatCurrency(amount) +
                    " | New balance: " + utils::formatCurrency(account.getBalance()));
    } catch (const BankException& ex) {
        logger_.log("DEPOSIT", account.getAccountNumber(), false, ex.what());
        throw;
    }
}

void ATM::withdraw(Account& account, double amount) {
    if (account.isLocked()) {
        logger_.log("WITHDRAW", account.getAccountNumber(), false, "Attempted withdrawal on locked account.");
        throw AccountLockedException("Cannot withdraw from locked account #" + account.getAccountNumber() + ".");
    }
    try {
        account.withdraw(amount);

        double fee = 0.0;
        if (auto* checking = dynamic_cast<CheckingAccount*>(&account)) {
            fee = checking->getLastFeeCharged();
        }

        if (fee > 0.0) {
            addTransaction(account.getAccountNumber(), "WITHDRAW", amount,
                           account.getBalance() + fee, "Cash withdrawal (overdraft triggered)");
            addTransaction(account.getAccountNumber(), "OVERDRAFT_FEE", fee,
                           account.getBalance(), "Flat overdraft fee");
            logger_.log("WITHDRAW", account.getAccountNumber(), true,
                        "Withdrew " + utils::formatCurrency(amount) +
                        " + Overdraft Fee " + utils::formatCurrency(fee) +
                        " | New balance: " + utils::formatCurrency(account.getBalance()));
        } else {
            addTransaction(account.getAccountNumber(), "WITHDRAW", amount,
                           account.getBalance(), "Cash withdrawal");
            logger_.log("WITHDRAW", account.getAccountNumber(), true,
                        "Withdrew " + utils::formatCurrency(amount) +
                        " | New balance: " + utils::formatCurrency(account.getBalance()));
        }

        saveAccounts();
        saveTransactions();
    } catch (const BankException& ex) {
        logger_.log("WITHDRAW", account.getAccountNumber(), false, ex.what());
        throw;
    }
}

void ATM::transfer(Account& source, const std::string& targetAccountNumber, double amount) {
    const std::string cleanedTarget = utils::trim(targetAccountNumber);
    if (source.isLocked()) {
        logger_.log("TRANSFER", source.getAccountNumber(), false, "Source account is locked.");
        throw AccountLockedException("Source account #" + source.getAccountNumber() + " is locked.");
    }

    Account* target = findAccount(cleanedTarget);
    if (!target) {
        logger_.log("TRANSFER", source.getAccountNumber(), false,
                    "Target account #" + cleanedTarget + " not found.");
        throw AccountNotFoundException("Target account #" + cleanedTarget + " was not found.");
    }

    if (source == *target) {
        logger_.log("TRANSFER", source.getAccountNumber(), false,
                    "Attempted self-transfer to #" + cleanedTarget + ".");
        throw InvalidInputException("Source and destination accounts cannot be the same.");
    }

    if (target->isLocked()) {
        logger_.log("TRANSFER", source.getAccountNumber(), false,
                    "Target account #" + target->getAccountNumber() + " is locked.");
        throw AccountLockedException(
            "Target account #" + target->getAccountNumber() + " is locked and cannot receive transfers.");
    }

    try {
        source.withdraw(amount);
        target->deposit(amount);

        double fee = 0.0;
        if (auto* checking = dynamic_cast<CheckingAccount*>(&source)) {
            fee = checking->getLastFeeCharged();
        }

        if (fee > 0.0) {
            addTransaction(source.getAccountNumber(), "TRANSFER_OUT", amount,
                           source.getBalance() + fee,
                           "To #" + target->getAccountNumber() + " (overdraft triggered)");
            addTransaction(source.getAccountNumber(), "OVERDRAFT_FEE", fee,
                           source.getBalance(),
                           "Overdraft fee on transfer to #" + target->getAccountNumber());
        } else {
            addTransaction(source.getAccountNumber(), "TRANSFER_OUT", amount,
                           source.getBalance(), "To #" + target->getAccountNumber());
        }

        addTransaction(target->getAccountNumber(), "TRANSFER_IN", amount,
                       target->getBalance(), "From #" + source.getAccountNumber());

        saveAccounts();
        saveTransactions();

        std::string logDetails = "Transferred " + utils::formatCurrency(amount) +
                                 " to #" + target->getAccountNumber();
        if (fee > 0.0) {
            logDetails += " (Overdraft fee applied: " + utils::formatCurrency(fee) + ")";
        }
        logger_.log("TRANSFER", source.getAccountNumber(), true, logDetails);
    } catch (const BankException& ex) {
        logger_.log("TRANSFER", source.getAccountNumber(), false, ex.what());
        throw;
    }
}

double ATM::applySavingsInterest(SavingsAccount& savings, double fractionOfYear) {
    if (savings.isLocked()) {
        logger_.log("INTEREST", savings.getAccountNumber(), false, "Cannot apply interest to locked account.");
        throw AccountLockedException("Account #" + savings.getAccountNumber() + " is locked.");
    }
    try {
        const double earned = savings.applyInterest(fractionOfYear);
        addTransaction(savings.getAccountNumber(), "INTEREST", earned,
                       savings.getBalance(), "Interest credited");
        saveAccounts();
        saveTransactions();
        logger_.log("INTEREST", savings.getAccountNumber(), true,
                    "Credited interest " + utils::formatCurrency(earned) +
                    " | New balance: " + utils::formatCurrency(savings.getBalance()));
        return earned;
    } catch (const BankException& ex) {
        logger_.log("INTEREST", savings.getAccountNumber(), false, ex.what());
        throw;
    }
}

void ATM::unlockAccount(const std::string& accountNumber, const std::string& adminPin) {
    const std::string cleanedAcc = utils::trim(accountNumber);
    if (utils::trim(adminPin) != adminPin_) {
        logger_.log("ADMIN_UNLOCK", cleanedAcc, false, "Invalid administrator PIN provided.");
        throw InvalidPinException("Invalid administrator PIN.");
    }

    Account* account = findAccount(cleanedAcc);
    if (!account) {
        logger_.log("ADMIN_UNLOCK", cleanedAcc, false, "Account not found.");
        throw AccountNotFoundException("Account #" + cleanedAcc + " was not found.");
    }

    account->unlock();
    saveAccounts();
    logger_.log("ACCOUNT_UNLOCKED", account->getAccountNumber(), true,
                "Account unlocked by administrator.");
}

std::string ATM::compareAccounts(const std::string& firstAccountNumber,
                                 const std::string& secondAccountNumber) const {
    const std::string accOneId = utils::trim(firstAccountNumber);
    const std::string accTwoId = utils::trim(secondAccountNumber);

    const Account* first = findAccount(accOneId);
    if (!first) {
        throw AccountNotFoundException("First account #" + accOneId + " was not found.");
    }
    const Account* second = findAccount(accTwoId);
    if (!second) {
        throw AccountNotFoundException("Second account #" + accTwoId + " was not found.");
    }

    std::ostringstream oss;
    oss << "Account 1: " << *first << '\n'
        << "Account 2: " << *second << '\n'
        << "Identity (==): "
        << ((*first == *second) ? "Same account number" : "Different account numbers") << '\n';

    if (*first < *second) {
        oss << "Balance (<): Account #" << first->getAccountNumber()
            << " has a LOWER balance than #" << second->getAccountNumber()
            << " by " << utils::formatCurrency(second->getBalance() - first->getBalance()) << '.';
    } else if (*first > *second) {
        oss << "Balance (>): Account #" << first->getAccountNumber()
            << " has a HIGHER balance than #" << second->getAccountNumber()
            << " by " << utils::formatCurrency(first->getBalance() - second->getBalance()) << '.';
    } else {
        oss << "Balance (==): Both accounts have the EXACT SAME balance ("
            << utils::formatCurrency(first->getBalance()) << ").";
    }
    return oss.str();
}

std::vector<TransactionRecord> ATM::getMiniStatement(const std::string& accountNumber,
                                                     std::size_t maxCount) const {
    const std::string cleaned = utils::trim(accountNumber);
    std::vector<TransactionRecord> filtered;
    for (const auto& record : transactions_) {
        if (record.accountNumber == cleaned) {
            filtered.push_back(record);
        }
    }

    if (filtered.size() > maxCount) {
        const auto startOffset = static_cast<std::ptrdiff_t>(filtered.size() - maxCount);
        return std::vector<TransactionRecord>(filtered.begin() + startOffset, filtered.end());
    }
    return filtered;
}

const std::vector<std::unique_ptr<Account>>& ATM::getAccounts() const {
    return accounts_;
}

const AuditLogger& ATM::getAuditLogger() const {
    return logger_;
}

bool ATM::readLine(std::istream& in, std::ostream& out, const std::string& prompt, std::string& value) const {
    out << prompt;
    out.flush();
    if (!std::getline(in, value)) {
        return false;
    }
    value = utils::trim(value);
    return true;
}

void ATM::printMainMenu(std::ostream& out) const {
    out << "\n=========================================\n"
        << "   BANK ACCOUNT & ATM SIMULATOR (EGP)\n"
        << "=========================================\n"
        << "1. Login to Account\n"
        << "2. Create New Account\n"
        << "3. Compare Two Accounts (== / < Demo)\n"
        << "4. Admin: Unlock Locked Account\n"
        << "5. Exit\n"
        << "-----------------------------------------\n";
}

void ATM::printAccountMenu(const Account& account, std::ostream& out) const {
    out << "\n-----------------------------------------\n"
        << "Logged in as: " << account << '\n'
        << "-----------------------------------------\n"
        << "1. View Full Account Details\n"
        << "2. Deposit Funds\n"
        << "3. Withdraw Funds\n"
        << "4. Transfer Funds\n"
        << "5. Mini-Statement (Recent Transactions)\n"
        << "6. Apply / Calculate Savings Interest\n"
        << "7. Logout\n"
        << "-----------------------------------------\n";
}

void ATM::run(std::istream& in, std::ostream& out) {
    initialize();

    out << "Welcome to the Bank Account & ATM Transaction Simulator.\n";
    if (seededOnInit_) {
        out << "Initial sample accounts created: #1001 (Savings), #2001 (Checking), #1002 (Savings).\n";
    }
    out << "Audit log file: " << logger_.getFilePath() << '\n';

    std::string choiceInput;
    while (true) {
        printMainMenu(out);
        if (!readLine(in, out, "Select an option (1-5): ", choiceInput)) {
            out << "\nInput stream closed. Exiting ATM Simulator.\n";
            break;
        }

        int choice = 0;
        try {
            choice = utils::parseIntStrict(choiceInput);
        } catch (const InvalidInputException& ex) {
            logger_.log("VALIDATION_ERROR", "SYSTEM", false, ex.what());
            out << "[ERROR] " << ex.what() << '\n';
            continue;
        }

        try {
            switch (choice) {
                case 1:
                    handleLoginFlow(in, out);
                    break;
                case 2:
                    handleCreateAccountFlow(in, out);
                    break;
                case 3:
                    handleCompareAccountsFlow(in, out);
                    break;
                case 4:
                    handleUnlockAccountFlow(in, out);
                    break;
                case 5:
                    out << "Thank you for using the ATM Simulator. Goodbye!\n";
                    logger_.log("SYSTEM_EXIT", "SYSTEM", true, "User exited the ATM simulator.");
                    return;
                default:
                    logger_.log("VALIDATION_ERROR", "SYSTEM", false,
                                "Menu choice out of range: " + choiceInput);
                    out << "[ERROR] Menu choice must be between 1 and 5.\n";
                    break;
            }
        } catch (const BankException& ex) {
            out << "[ERROR] " << ex.what() << '\n';
        }
    }
}

void ATM::handleLoginFlow(std::istream& in, std::ostream& out) {
    std::string accountNumber;
    if (!readLine(in, out, "Enter Account Number (or 'cancel'): ", accountNumber)) {
        return;
    }
    if (accountNumber == "cancel") {
        return;
    }

    Account* target = findAccount(accountNumber);
    if (!target) {
        logger_.log("LOGIN", accountNumber, false, "Account not found.");
        throw AccountNotFoundException("Account #" + accountNumber + " was not found.");
    }

    if (target->isLocked()) {
        // Trigger standard locked logging and exception via login()
        login(accountNumber, "");
        return;
    }

    while (true) {
        std::string pin;
        if (!readLine(in, out, "Enter 4-digit PIN (or 'cancel'): ", pin)) {
            return;
        }
        if (pin == "cancel") {
            return;
        }

        try {
            Account& authenticated = login(accountNumber, pin);
            out << "\nLogin successful! Welcome, " << authenticated.getAccountHolderName() << ".\n";
            runAccountSession(authenticated, in, out);
            return;
        } catch (const InvalidPinException& ex) {
            out << "[AUTH ERROR] " << ex.what() << '\n';
        } catch (const AccountLockedException& ex) {
            out << "[LOCKED] " << ex.what() << '\n';
            return;
        }
    }
}

void ATM::handleCreateAccountFlow(std::istream& in, std::ostream& out) {
    out << "\n--- Create New Account ---\n"
        << "1. Savings Account (Min Balance: 500.00 EGP, 5.00% Annual Interest)\n"
        << "2. Checking Account (Overdraft Limit: 1,000.00 EGP, Overdraft Fee: 35.00 EGP)\n";

    std::string typeInput;
    if (!readLine(in, out, "Choose Account Type (1 or 2): ", typeInput)) {
        return;
    }
    int typeChoice = 0;
    try {
        typeChoice = utils::parseIntStrict(typeInput);
        if (typeChoice != 1 && typeChoice != 2) {
            throw InvalidInputException("Account type choice must be 1 (Savings) or 2 (Checking).");
        }
    } catch (const BankException& ex) {
        logger_.log("CREATE_ACCOUNT", "N/A", false, ex.what());
        throw;
    }

    std::string accNum;
    std::string holder;
    std::string pin;
    std::string initialStr;

    if (!readLine(in, out, "Enter New Account Number (3-12 digits): ", accNum)) {
        return;
    }
    if (!readLine(in, out, "Enter Account Holder Name: ", holder)) {
        return;
    }
    if (!readLine(in, out, "Set 4-digit PIN: ", pin)) {
        return;
    }
    if (!readLine(in, out, "Enter Initial Deposit (EGP): ", initialStr)) {
        return;
    }

    double initialDeposit = 0.0;
    try {
        initialDeposit = (typeChoice == 1)
            ? utils::parsePositiveAmount(initialStr)
            : utils::parseDoubleStrict(initialStr);
    } catch (const BankException& ex) {
        logger_.log("CREATE_ACCOUNT", utils::trim(accNum), false, ex.what());
        throw;
    }

    if (typeChoice == 1) {
        Account& created = createSavingsAccount(accNum, holder, pin, initialDeposit);
        out << "[OK] Created: " << created << '\n';
    } else {
        Account& created = createCheckingAccount(accNum, holder, pin, initialDeposit);
        out << "[OK] Created: " << created << '\n';
    }
}

void ATM::handleCompareAccountsFlow(std::istream& in, std::ostream& out) const {
    std::string firstAcc;
    std::string secondAcc;
    if (!readLine(in, out, "Enter First Account Number: ", firstAcc)) {
        return;
    }
    if (!readLine(in, out, "Enter Second Account Number: ", secondAcc)) {
        return;
    }

    try {
        const std::string report = compareAccounts(firstAcc, secondAcc);
        logger_.log("COMPARE_ACCOUNTS", firstAcc + " vs " + secondAcc, true,
                    "Compared account numbers and balances.");
        out << "\n--- Account Comparison ---\n" << report << '\n';
    } catch (const BankException& ex) {
        logger_.log("COMPARE_ACCOUNTS", firstAcc + " vs " + secondAcc, false, ex.what());
        throw;
    }
}

void ATM::handleUnlockAccountFlow(std::istream& in, std::ostream& out) {
    std::string accNum;
    std::string adminPin;
    if (!readLine(in, out, "Enter Account Number to Unlock: ", accNum)) {
        return;
    }
    if (!readLine(in, out, "Enter Administrator PIN: ", adminPin)) {
        return;
    }

    unlockAccount(accNum, adminPin);
    out << "[OK] Account #" << accNum << " has been unlocked.\n";
}

void ATM::runAccountSession(Account& account, std::istream& in, std::ostream& out) {
    std::string choiceInput;
    while (true) {
        printAccountMenu(account, out);
        if (!readLine(in, out, "Select an action (1-7): ", choiceInput)) {
            return;
        }

        int choice = 0;
        try {
            choice = utils::parseIntStrict(choiceInput);
        } catch (const InvalidInputException& ex) {
            logger_.log("VALIDATION_ERROR", account.getAccountNumber(), false, ex.what());
            out << "[ERROR] " << ex.what() << '\n';
            continue;
        }

        try {
            switch (choice) {
                case 1: {
                    out << "\n--- Account Details ---\n";
                    account.displayDetails(out);
                    break;
                }
                case 2: {
                    std::string amountStr;
                    if (!readLine(in, out, "Enter deposit amount (EGP): ", amountStr)) {
                        return;
                    }
                    double amount = 0.0;
                    try {
                        amount = utils::parsePositiveAmount(amountStr);
                    } catch (const BankException& ex) {
                        logger_.log("DEPOSIT", account.getAccountNumber(), false, ex.what());
                        throw;
                    }
                    deposit(account, amount);
                    out << "[OK] Deposit successful. " << account << '\n';
                    break;
                }
                case 3: {
                    std::string amountStr;
                    if (!readLine(in, out, "Enter withdrawal amount (EGP): ", amountStr)) {
                        return;
                    }
                    double amount = 0.0;
                    try {
                        amount = utils::parsePositiveAmount(amountStr);
                    } catch (const BankException& ex) {
                        logger_.log("WITHDRAW", account.getAccountNumber(), false, ex.what());
                        throw;
                    }
                    withdraw(account, amount);
                    out << "[OK] Withdrawal successful. " << account << '\n';
                    break;
                }
                case 4: {
                    std::string targetAcc;
                    std::string amountStr;
                    if (!readLine(in, out, "Enter destination account number: ", targetAcc)) {
                        return;
                    }
                    if (!readLine(in, out, "Enter transfer amount (EGP): ", amountStr)) {
                        return;
                    }
                    double amount = 0.0;
                    try {
                        amount = utils::parsePositiveAmount(amountStr);
                    } catch (const BankException& ex) {
                        logger_.log("TRANSFER", account.getAccountNumber(), false, ex.what());
                        throw;
                    }
                    transfer(account, targetAcc, amount);
                    out << "[OK] Transfer successful. " << account << '\n';
                    break;
                }
                case 5: {
                    const auto history = getMiniStatement(account.getAccountNumber(), 10);
                    out << "\n--- Mini-Statement (Last " << history.size() << " Transactions) ---\n";
                    if (history.empty()) {
                        out << "No transactions recorded for this account yet.\n";
                    } else {
                        for (const auto& record : history) {
                            out << record << '\n';
                        }
                    }
                    break;
                }
                case 6: {
                    auto* savings = dynamic_cast<SavingsAccount*>(&account);
                    if (!savings) {
                        out << "[INFO] Interest calculation is only available for Savings Accounts.\n";
                        break;
                    }
                    out << "\n--- Savings Interest ---\n"
                        << "Current Balance     : " << utils::formatCurrency(savings->getBalance()) << '\n'
                        << "Annual Interest Rate: " << std::fixed << std::setprecision(2)
                        << (savings->getAnnualInterestRate() * 100.0) << "%\n"
                        << "Projected Monthly   : " << utils::formatCurrency(savings->calculateProjectedInterest(1.0 / 12.0)) << '\n'
                        << "Projected Annual    : " << utils::formatCurrency(savings->calculateProjectedInterest(1.0)) << '\n'
                        << "1. Apply Monthly Interest (1/12 year)\n"
                        << "2. Apply Annual Interest (1 year)\n"
                        << "3. Cancel\n";
                    std::string intChoiceStr;
                    if (!readLine(in, out, "Choose option (1-3): ", intChoiceStr)) {
                        return;
                    }
                    int intChoice = 0;
                    try {
                        intChoice = utils::parseIntStrict(intChoiceStr);
                    } catch (const InvalidInputException& ex) {
                        logger_.log("VALIDATION_ERROR", account.getAccountNumber(), false, ex.what());
                        throw;
                    }
                    if (intChoice == 1) {
                        const double earned = applySavingsInterest(*savings, 1.0 / 12.0);
                        out << "[OK] Monthly interest credited: " << utils::formatCurrency(earned)
                            << " | New Balance: " << utils::formatCurrency(savings->getBalance()) << '\n';
                    } else if (intChoice == 2) {
                        const double earned = applySavingsInterest(*savings, 1.0);
                        out << "[OK] Annual interest credited: " << utils::formatCurrency(earned)
                            << " | New Balance: " << utils::formatCurrency(savings->getBalance()) << '\n';
                    } else if (intChoice != 3) {
                        logger_.log("VALIDATION_ERROR", account.getAccountNumber(), false,
                                    "Invalid interest option: " + intChoiceStr);
                        throw InvalidInputException("Interest choice must be 1, 2, or 3.");
                    }
                    break;
                }
                case 7:
                    logger_.log("LOGOUT", account.getAccountNumber(), true, "User logged out.");
                    out << "Logged out of account #" << account.getAccountNumber() << ".\n";
                    return;
                default:
                    logger_.log("VALIDATION_ERROR", account.getAccountNumber(), false,
                                "Session menu choice out of range: " + choiceInput);
                    out << "[ERROR] Action choice must be between 1 and 7.\n";
                    break;
            }
        } catch (const BankException& ex) {
            out << "[ERROR] " << ex.what() << '\n';
        }
    }
}

} // namespace bank
