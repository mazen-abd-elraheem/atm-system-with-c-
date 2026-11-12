#include "AuditLogger.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"

#include <filesystem>
#include <fstream>
#include <utility>

namespace bank {

AuditLogger::AuditLogger(std::string filePath)
    : filePath_(std::move(filePath)) {}

void AuditLogger::log(const std::string& eventType,
                      const std::string& accountNumber,
                      bool success,
                      const std::string& details) const {
    const std::filesystem::path path(filePath_);
    if (!path.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ofstream out(filePath_, std::ios::app);
    if (!out.is_open()) {
        throw PersistenceException("Failed to open audit log file: " + filePath_);
    }

    out << '[' << utils::currentTimestamp() << "] "
        << '[' << (success ? "SUCCESS" : "FAILED") << "] "
        << '[' << eventType << "] "
        << "Account: " << (accountNumber.empty() ? "N/A" : accountNumber)
        << " | " << utils::sanitizeField(details) << '\n';
}

const std::string& AuditLogger::getFilePath() const {
    return filePath_;
}

} // namespace bank

