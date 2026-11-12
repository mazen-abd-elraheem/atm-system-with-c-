#include "Account.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"

#include <cmath>
#include <ostream>
#include <utility>

namespace bank {

Account::Account(std::string accountNumber,
                 std::string accountHolderName,
                 double initialBalance,
                 std::string pin,
                 int failedPinAttempts,
                 bool locked)
    : accountNumber_(utils::sanitizeField(accountNumber)),
      accountHolderName_(utils::sanitizeField(accountHolderName)),
      balance_(initialBalance),
      pin_(utils::trim(pin)),
      failedPinAttempts_(failedPinAttempts < 0 ? 0 : failedPinAttempts),
      locked_(locked) {
    if (!utils::isValidAccountNumber(accountNumber_)) {
        throw InvalidInputException("Account number must be 3 to 12 digits.");
    }
    if (accountHolderName_.empty()) {
        throw InvalidInputException("Account holder name cannot be empty.");
    }
    if (!utils::isValidPin(pin_)) {
        throw InvalidPinException("PIN must be exactly 4 digits.");
    }
    if (!std::isfinite(balance_)) {
        throw InvalidAmountException("Initial balance must be a finite number.");
    }
    if (failedPinAttempts_ >= MAX_PIN_ATTEMPTS) {
        locked_ = true;
    }
}

void Account::validatePositiveAmount(double amount, const std::string& operationName) const {
    if (!std::isfinite(amount) || amount <= 0.0) {
        throw InvalidAmountException(operationName + " amount must be greater than zero.");
    }
}

void Account::deposit(double amount) {
    validatePositiveAmount(amount, "Deposit");
    balance_ += amount;
}

void Account::formatSummary(std::ostream& os) const {
    os << '[' << getAccountType() << "] #" << accountNumber_
       << " | " << accountHolderName_
       << " | Balance: " << utils::formatCurrency(balance_)
       << " | Status: " << (locked_ ? "LOCKED" : "Active");
}

void Account::displayDetails(std::ostream& os) const {
    os << "Account Type   : " << getAccountType() << '\n'
       << "Account Number : " << accountNumber_ << '\n'
       << "Holder Name    : " << accountHolderName_ << '\n'
       << "Balance        : " << utils::formatCurrency(balance_) << '\n'
       << "Status         : " << (locked_ ? "LOCKED" : "Active") << '\n';
}

void Account::authenticate(const std::string& enteredPin) {
    if (locked_) {
        throw AccountLockedException(
            "Account #" + accountNumber_ + " is locked. Contact an administrator to unlock it.");
    }

    if (utils::trim(enteredPin) == pin_) {
        failedPinAttempts_ = 0;
        return;
    }

    ++failedPinAttempts_;
    if (failedPinAttempts_ >= MAX_PIN_ATTEMPTS) {
        locked_ = true;
        throw AccountLockedException(
            "Account #" + accountNumber_ + " has been locked after " +
            std::to_string(MAX_PIN_ATTEMPTS) + " consecutive failed PIN attempts.");
    }

    const int remaining = MAX_PIN_ATTEMPTS - failedPinAttempts_;
    throw InvalidPinException(
        "Incorrect PIN. Remaining attempts before lockout: " + std::to_string(remaining) + ".");
}

void Account::unlock() {
    locked_ = false;
    failedPinAttempts_ = 0;
}

const std::string& Account::getAccountNumber() const {
    return accountNumber_;
}

const std::string& Account::getAccountHolderName() const {
    return accountHolderName_;
}

double Account::getBalance() const {
    return balance_;
}

const std::string& Account::getPin() const {
    return pin_;
}

int Account::getFailedPinAttempts() const {
    return failedPinAttempts_;
}

bool Account::isLocked() const {
    return locked_;
}

void Account::setBalance(double newBalance) {
    balance_ = newBalance;
}

bool Account::operator==(const Account& other) const {
    return accountNumber_ == other.accountNumber_;
}

bool Account::operator!=(const Account& other) const {
    return !(*this == other);
}

bool Account::operator<(const Account& other) const {
    return balance_ < other.balance_;
}

bool Account::operator>(const Account& other) const {
    return other < *this;
}

bool Account::operator<=(const Account& other) const {
    return !(other < *this);
}

bool Account::operator>=(const Account& other) const {
    return !(*this < other);
}

std::ostream& operator<<(std::ostream& os, const Account& account) {
    account.formatSummary(os);
    return os;
}

} // namespace bank

