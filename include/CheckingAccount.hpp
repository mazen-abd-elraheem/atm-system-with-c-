#ifndef BANK_ATM_CHECKING_ACCOUNT_HPP
#define BANK_ATM_CHECKING_ACCOUNT_HPP

#include "Account.hpp"

namespace bank {

/**
 * @brief Checking account supporting overdraft up to a credit limit with a flat overdraft fee.
 */
class CheckingAccount : public Account {
public:
    static constexpr double DEFAULT_OVERDRAFT_LIMIT = 1000.0;
    static constexpr double DEFAULT_OVERDRAFT_FEE = 35.0;

    CheckingAccount(std::string accountNumber,
                    std::string accountHolderName,
                    double initialBalance,
                    std::string pin,
                    double overdraftLimit = DEFAULT_OVERDRAFT_LIMIT,
                    double overdraftFee = DEFAULT_OVERDRAFT_FEE,
                    int failedPinAttempts = 0,
                    bool locked = false);

    std::string getAccountType() const override;
    void withdraw(double amount) override;
    void formatSummary(std::ostream& os) const override;
    void displayDetails(std::ostream& os) const override;
    std::string serialize() const override;

    double getOverdraftLimit() const;
    double getOverdraftFee() const;
    double getLastFeeCharged() const;
    double getAvailableFunds() const;

private:
    double overdraftLimit_;
    double overdraftFee_;
    double lastFeeCharged_;
};

} // namespace bank

#endif // BANK_ATM_CHECKING_ACCOUNT_HPP

