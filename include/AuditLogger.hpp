#ifndef BANK_ATM_AUDIT_LOGGER_HPP
#define BANK_ATM_AUDIT_LOGGER_HPP

#include <string>

namespace bank {

/**
 * @brief Appends timestamped audit entries for all successful and failed ATM actions.
 */
class AuditLogger {
public:
    explicit AuditLogger(std::string filePath = "audit.txt");

    void log(const std::string& eventType,
             const std::string& accountNumber,
             bool success,
             const std::string& details) const;

    const std::string& getFilePath() const;

private:
    std::string filePath_;
};

} // namespace bank

#endif // BANK_ATM_AUDIT_LOGGER_HPP

