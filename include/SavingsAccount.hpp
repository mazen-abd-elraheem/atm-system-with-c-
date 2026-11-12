#ifndef BANK_ATM_SAVINGS_ACCOUNT_HPP
#define BANK_ATM_SAVINGS_ACCOUNT_HPP

#include "Account.hpp"

namespace bank {

/**
 * @brief Savings account enforcing a minimum balance floor and earning interest.
 */
class SavingsAccount : public Account {
public:
    static constexpr double DEFAULT_MINIMUM_BALANCE = 500.0;
    static constexpr double DEFAULT_ANNUAL_INTEREST_RATE = 0.05;

    SavingsAccount(std::string accountNumber,
                   std::string accountHolderName,
                   double initialBalance,
                   std::string pin,
                   double minimumBalance = DEFAULT_MINIMUM_BALANCE,
                   double annualInterestRate = DEFAULT_ANNUAL_INTEREST_RATE,
                   int failedPinAttempts = 0,
                   bool locked = false);

    std::string getAccountType() const override;
    void withdraw(double amount) override;
    void formatSummary(std::ostream& os) const override;
    void displayDetails(std::ostream& os) const override;
    std::string serialize() const override;

    double calculateProjectedInterest(double fractionOfYear = 1.0) const;
    double applyInterest(double fractionOfYear = 1.0);

    double getMinimumBalance() const;
    double getAnnualInterestRate() const;
    double getAvailableForWithdrawal() const;

private:
    double minimumBalance_;
    double annualInterestRate_;
};

} // namespace bank

#endif // BANK_ATM_SAVINGS_ACCOUNT_HPP

