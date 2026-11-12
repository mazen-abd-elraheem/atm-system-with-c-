#include "Transaction.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>

namespace bank {

std::string TransactionRecord::serialize() const {
    std::ostringstream oss;
    oss << utils::sanitizeField(timestamp) << '|'
        << utils::sanitizeField(accountNumber) << '|'
        << utils::sanitizeField(type) << '|'
        << std::fixed << std::setprecision(2) << amount << '|'
        << std::fixed << std::setprecision(2) << balanceAfter << '|'
        << utils::sanitizeField(details);
    return oss.str();
}

TransactionRecord TransactionRecord::deserialize(const std::string& line) {
    const auto fields = utils::splitDelimited(line, '|');
    if (fields.size() != 6) {
        throw PersistenceException("Malformed transaction record line.");
    }

    TransactionRecord record;
    record.timestamp = fields[0];
    record.accountNumber = fields[1];
    record.type = fields[2];
    record.amount = utils::parseDoubleStrict(fields[3]);
    record.balanceAfter = utils::parseDoubleStrict(fields[4]);
    record.details = fields[5];
    return record;
}

std::ostream& operator<<(std::ostream& os, const TransactionRecord& record) {
    os << record.timestamp << " | "
       << std::left << std::setw(14) << record.type << " | "
       << std::right << std::setw(16) << utils::formatCurrency(record.amount) << " | Bal: "
       << std::setw(16) << utils::formatCurrency(record.balanceAfter);
    if (!record.details.empty()) {
        os << " | " << record.details;
    }
    return os;
}

} // namespace bank

