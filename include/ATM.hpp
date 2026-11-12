#ifndef BANK_ATM_ATM_HPP
#define BANK_ATM_ATM_HPP

#include "Account.hpp"
#include "AuditLogger.hpp"
#include "SavingsAccount.hpp"
#include "Transaction.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace bank {

/**
 * @brief Central ATM controller managing accounts, authentication, transactions,
 *        persistence, audit logging, and the interactive console session.
 */
class ATM {
public:
    static constexpr const char* DEFAULT_ADMIN_PIN = "9999";

    ATM(std::string accountsFilePath = "accounts.txt",
        std::string transactionsFilePath = "transactions.txt",
        std::string auditFilePath = "audit.txt",
        std::string adminPin = DEFAULT_ADMIN_PIN);

    void initialize();

    // Core banking API
    Account& login(const std::string& accountNumber, const std::string& pin);
    Account& createSavingsAccount(const std::string& accountNumber,
                                  const std::string& holderName,
                                  const std::string& pin,
                                  double initialDeposit,
                                  double minimumBalance = SavingsAccount::DEFAULT_MINIMUM_BALANCE,
                                  double annualInterestRate = SavingsAccount::DEFAULT_ANNUAL_INTEREST_RATE);
    Account& createCheckingAccount(const std::string& accountNumber,
                                   const std::string& holderName,
                                   const std::string& pin,
                                   double initialDeposit,
                                   double overdraftLimit = 1000.0,
                                   double overdraftFee = 35.0);

    void deposit(Account& account, double amount);
    void withdraw(Account& account, double amount);
    void transfer(Account& source, const std::string& targetAccountNumber, double amount);
    double applySavingsInterest(SavingsAccount& savings, double fractionOfYear = 1.0);
    void unlockAccount(const std::string& accountNumber, const std::string& adminPin);

    std::string compareAccounts(const std::string& firstAccountNumber,
                                const std::string& secondAccountNumber) const;

    std::vector<TransactionRecord> getMiniStatement(const std::string& accountNumber,
                                                    std::size_t maxCount = 10) const;

    Account* findAccount(const std::string& accountNumber) const;
    const std::vector<std::unique_ptr<Account>>& getAccounts() const;
    const AuditLogger& getAuditLogger() const;

    void saveAccounts() const;
    void saveTransactions() const;

    // Interactive console loop
    void run(std::istream& in = std::cin, std::ostream& out = std::cout);

private:
    void loadAccounts();
    void loadTransactions();
    void seedDefaultAccounts();
    std::unique_ptr<Account> deserializeAccount(const std::string& line) const;
    void ensureUniqueAccount(const Account& candidate) const;
    void addTransaction(const std::string& accountNumber,
                        const std::string& type,
                        double amount,
                        double balanceAfter,
                        const std::string& details);

    // Interactive console helpers
    bool readLine(std::istream& in, std::ostream& out, const std::string& prompt, std::string& value) const;
    void handleLoginFlow(std::istream& in, std::ostream& out);
    void handleCreateAccountFlow(std::istream& in, std::ostream& out);
    void handleCompareAccountsFlow(std::istream& in, std::ostream& out) const;
    void handleUnlockAccountFlow(std::istream& in, std::ostream& out);
    void runAccountSession(Account& account, std::istream& in, std::ostream& out);
    void printMainMenu(std::ostream& out) const;
    void printAccountMenu(const Account& account, std::ostream& out) const;

    std::string accountsFilePath_;
    std::string transactionsFilePath_;
    AuditLogger logger_;
    std::string adminPin_;
    bool seededOnInit_;
    std::vector<std::unique_ptr<Account>> accounts_;
    std::vector<TransactionRecord> transactions_;
};

} // namespace bank

#endif // BANK_ATM_ATM_HPP

