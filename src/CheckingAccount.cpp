#include "CheckingAccount.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <utility>

namespace bank {

namespace {
constexpr double kEpsilon = 1e-9;
}

CheckingAccount::CheckingAccount(std::string accountNumber,
                                 std::string accountHolderName,
                                 double initialBalance,
                                 std::string pin,
                                 double overdraftLimit,
                                 double overdraftFee,
                                 int failedPinAttempts,
                                 bool locked)
    : Account(std::move(accountNumber),
              std::move(accountHolderName),
              initialBalance,
              std::move(pin),
              failedPinAttempts,
              locked),
      overdraftLimit_(overdraftLimit),
      overdraftFee_(overdraftFee),
      lastFeeCharged_(0.0) {
    if (!std::isfinite(overdraftLimit_) || overdraftLimit_ < 0.0) {
        throw InvalidAmountException("Overdraft limit cannot be negative.");
    }
    if (!std::isfinite(overdraftFee_) || overdraftFee_ < 0.0) {
        throw InvalidAmountException("Overdraft fee cannot be negative.");
    }
    if (balance_ + kEpsilon < -overdraftLimit_) {
        throw InsufficientFundsException(
            "Initial balance cannot be lower than the negative overdraft limit (-" +
            utils::formatCurrency(overdraftLimit_) + ").");
    }
}

std::string CheckingAccount::getAccountType() const {
    return "Checking";
}

void CheckingAccount::withdraw(double amount) {
    lastFeeCharged_ = 0.0;
    validatePositiveAmount(amount, "Withdrawal");

    const double balanceAfterAmount = balance_ - amount;
    if (balanceAfterAmount >= -kEpsilon) {
        setBalance(std::abs(balanceAfterAmount) < kEpsilon ? 0.0 : balanceAfterAmount);
        return;
    }

    const double balanceAfterFee = balanceAfterAmount - overdraftFee_;
    if (balanceAfterFee + kEpsilon < -overdraftLimit_) {
        throw InsufficientFundsException(
            "Insufficient funds: withdrawal of " + utils::formatCurrency(amount) +
            " plus overdraft fee (" + utils::formatCurrency(overdraftFee_) +
            ") exceeds available balance + overdraft limit (" +
            utils::formatCurrency(getAvailableFunds()) + ").");
    }

    setBalance(balanceAfterFee);
    lastFeeCharged_ = overdraftFee_;
}

void CheckingAccount::formatSummary(std::ostream& os) const {
    Account::formatSummary(os);
    os << " | OD Limit: " << utils::formatCurrency(overdraftLimit_)
       << " | OD Fee: " << utils::formatCurrency(overdraftFee_);
}

void CheckingAccount::displayDetails(std::ostream& os) const {
    Account::displayDetails(os);
    os << "Overdraft Limit: " << utils::formatCurrency(overdraftLimit_) << '\n'
       << "Overdraft Fee  : " << utils::formatCurrency(overdraftFee_) << '\n'
       << "Available Funds: " << utils::formatCurrency(getAvailableFunds()) << '\n';
}

std::string CheckingAccount::serialize() const {
    std::ostringstream oss;
    oss << "CHECKING|"
        << utils::sanitizeField(accountNumber_) << '|'
        << utils::sanitizeField(accountHolderName_) << '|'
        << std::fixed << std::setprecision(2) << balance_ << '|'
        << utils::sanitizeField(pin_) << '|'
        << failedPinAttempts_ << '|'
        << (locked_ ? 1 : 0) << '|'
        << std::fixed << std::setprecision(2) << overdraftLimit_ << '|'
        << std::fixed << std::setprecision(2) << overdraftFee_;
    return oss.str();
}

double CheckingAccount::getOverdraftLimit() const {
    return overdraftLimit_;
}

double CheckingAccount::getOverdraftFee() const {
    return overdraftFee_;
}

double CheckingAccount::getLastFeeCharged() const {
    return lastFeeCharged_;
}

double CheckingAccount::getAvailableFunds() const {
    return std::max(0.0, balance_ + overdraftLimit_);
}

} // namespace bank

