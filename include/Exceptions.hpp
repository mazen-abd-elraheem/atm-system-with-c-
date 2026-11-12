#ifndef BANK_ATM_EXCEPTIONS_HPP
#define BANK_ATM_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace bank {

/**
 * @brief Base exception class for all banking and ATM domain errors.
 */
class BankException : public std::runtime_error {
public:
    explicit BankException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Thrown when a withdrawal or transfer exceeds available funds or limits.
 */
class InsufficientFundsException : public BankException {
public:
    explicit InsufficientFundsException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when a monetary amount or rate is negative or zero where positive is required.
 */
class InvalidAmountException : public BankException {
public:
    explicit InvalidAmountException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when an entered PIN is invalid or does not match the account PIN.
 */
class InvalidPinException : public BankException {
public:
    explicit InvalidPinException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when login or operation is attempted on a locked account.
 */
class AccountLockedException : public BankException {
public:
    explicit AccountLockedException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when an account number cannot be found in the bank registry.
 */
class AccountNotFoundException : public BankException {
public:
    explicit AccountNotFoundException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when attempting to create an account with an existing account number.
 */
class DuplicateAccountException : public BankException {
public:
    explicit DuplicateAccountException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when user input cannot be parsed or violates structural rules.
 */
class InvalidInputException : public BankException {
public:
    explicit InvalidInputException(const std::string& message)
        : BankException(message) {}
};

/**
 * @brief Thrown when reading or writing persistent bank files fails.
 */
class PersistenceException : public BankException {
public:
    explicit PersistenceException(const std::string& message)
        : BankException(message) {}
};

} // namespace bank

#endif // BANK_ATM_EXCEPTIONS_HPP

