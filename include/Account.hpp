#ifndef BANK_ATM_ACCOUNT_HPP
#define BANK_ATM_ACCOUNT_HPP

#include <iosfwd>
#include <string>

namespace bank {

/**
 * @brief Abstract base class representing a bank account with PIN security,
 *        polymorphic operations, and overloaded formatting/comparison operators.
 */
class Account {
public:
    static constexpr int MAX_PIN_ATTEMPTS = 3;

    Account(std::string accountNumber,
            std::string accountHolderName,
            double initialBalance,
            std::string pin,
            int failedPinAttempts = 0,
            bool locked = false);

    virtual ~Account() = default;

    // Core polymorphic banking interface
    virtual std::string getAccountType() const = 0;
    virtual void deposit(double amount);
    virtual void withdraw(double amount) = 0;
    virtual void formatSummary(std::ostream& os) const;
    virtual void displayDetails(std::ostream& os) const;
    virtual std::string serialize() const = 0;

    // Security and state management
    void authenticate(const std::string& enteredPin);
    void unlock();

    // Getters
    const std::string& getAccountNumber() const;
    const std::string& getAccountHolderName() const;
    double getBalance() const;
    const std::string& getPin() const;
    int getFailedPinAttempts() const;
    bool isLocked() const;

    // Comparison operators:
    // == and != check account number identity; <, >, <=, >= compare account balances.
    bool operator==(const Account& other) const;
    bool operator!=(const Account& other) const;
    bool operator<(const Account& other) const;
    bool operator>(const Account& other) const;
    bool operator<=(const Account& other) const;
    bool operator>=(const Account& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Account& account);

protected:
    void setBalance(double newBalance);
    void validatePositiveAmount(double amount, const std::string& operationName) const;

    std::string accountNumber_;
    std::string accountHolderName_;
    double balance_;
    std::string pin_;
    int failedPinAttempts_;
    bool locked_;
};

} // namespace bank

#endif // BANK_ATM_ACCOUNT_HPP

