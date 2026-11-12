#include "SavingsAccount.hpp"
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

SavingsAccount::SavingsAccount(std::string accountNumber,
                               std::string accountHolderName,
                               double initialBalance,
                               std::string pin,
                               double minimumBalance,
                               double annualInterestRate,
                               int failedPinAttempts,
                               bool locked)
    : Account(std::move(accountNumber),
              std::move(accountHolderName),
              initialBalance,
              std::move(pin),
              failedPinAttempts,
              locked),
      minimumBalance_(minimumBalance),
      annualInterestRate_(annualInterestRate) {
    if (!std::isfinite(minimumBalance_) || minimumBalance_ < 0.0) {
        throw InvalidAmountException("Minimum balance cannot be negative.");
    }
    if (!std::isfinite(annualInterestRate_) || annualInterestRate_ < 0.0) {
        throw InvalidAmountException("Interest rate cannot be negative.");
    }
    if (balance_ + kEpsilon < minimumBalance_) {
        throw InsufficientFundsException(
            "Initial deposit for a Savings Account must be at least the minimum balance (" +
            utils::formatCurrency(minimumBalance_) + ").");
    }
}

std::string SavingsAccount::getAccountType() const {
    return "Savings";
}

void SavingsAccount::withdraw(double amount) {
    validatePositiveAmount(amount, "Withdrawal");
    if (balance_ - amount + kEpsilon < minimumBalance_) {
        throw InsufficientFundsException(
            "Insufficient funds: withdrawal of " + utils::formatCurrency(amount) +
            " violates the minimum balance requirement of " +
            utils::formatCurrency(minimumBalance_) +
            ". Maximum withdrawable: " + utils::formatCurrency(getAvailableForWithdrawal()) + ".");
    }
    setBalance(balance_ - amount);
}

void SavingsAccount::formatSummary(std::ostream& os) const {
    Account::formatSummary(os);
    os << " | Min Bal: " << utils::formatCurrency(minimumBalance_)
       << " | Rate: " << std::fixed << std::setprecision(2) << (annualInterestRate_ * 100.0) << '%';
}

void SavingsAccount::displayDetails(std::ostream& os) const {
    Account::displayDetails(os);
    os << "Minimum Balance: " << utils::formatCurrency(minimumBalance_) << '\n'
       << "Withdrawable   : " << utils::formatCurrency(getAvailableForWithdrawal()) << '\n'
       << "Interest Rate  : " << std::fixed << std::setprecision(2) << (annualInterestRate_ * 100.0)
       << "% / year\n"
       << "Est. Annual Int: " << utils::formatCurrency(calculateProjectedInterest(1.0)) << '\n'
       << "Est. Month Int : " << utils::formatCurrency(calculateProjectedInterest(1.0 / 12.0)) << '\n';
}

std::string SavingsAccount::serialize() const {
    std::ostringstream oss;
    oss << "SAVINGS|"
        << utils::sanitizeField(accountNumber_) << '|'
        << utils::sanitizeField(accountHolderName_) << '|'
        << std::fixed << std::setprecision(2) << balance_ << '|'
        << utils::sanitizeField(pin_) << '|'
        << failedPinAttempts_ << '|'
        << (locked_ ? 1 : 0) << '|'
        << std::fixed << std::setprecision(2) << minimumBalance_ << '|'
        << std::fixed << std::setprecision(6) << annualInterestRate_;
    return oss.str();
}

double SavingsAccount::calculateProjectedInterest(double fractionOfYear) const {
    if (!std::isfinite(fractionOfYear) || fractionOfYear <= 0.0) {
        throw InvalidAmountException("Interest period must be greater than zero.");
    }
    const double raw = balance_ * annualInterestRate_ * fractionOfYear;
    return static_cast<double>(std::llround(raw * 100.0)) / 100.0;
}

double SavingsAccount::applyInterest(double fractionOfYear) {
    const double earned = calculateProjectedInterest(fractionOfYear);
    if (earned > 0.0) {
        setBalance(balance_ + earned);
    }
    return earned;
}

double SavingsAccount::getMinimumBalance() const {
    return minimumBalance_;
}

double SavingsAccount::getAnnualInterestRate() const {
    return annualInterestRate_;
}

double SavingsAccount::getAvailableForWithdrawal() const {
    return std::max(0.0, balance_ - minimumBalance_);
}

} // namespace bank

