#ifndef BANK_ATM_TRANSACTION_HPP
#define BANK_ATM_TRANSACTION_HPP

#include <iosfwd>
#include <string>

namespace bank {

/**
 * @brief Represents a persisted financial transaction for an account's mini-statement.
 */
struct TransactionRecord {
    std::string timestamp;
    std::string accountNumber;
    std::string type;
    double amount = 0.0;
    double balanceAfter = 0.0;
    std::string details;

    std::string serialize() const;
    static TransactionRecord deserialize(const std::string& line);
};

std::ostream& operator<<(std::ostream& os, const TransactionRecord& record);

} // namespace bank

#endif // BANK_ATM_TRANSACTION_HPP

