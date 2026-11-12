#include "Utils.hpp"
#include "Exceptions.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>

namespace bank::utils {

std::string formatCurrency(double amount, const std::string& currency) {
    if (!std::isfinite(amount)) {
        return "0.00 " + currency;
    }

    const long long totalCentsSigned = std::llround(amount * 100.0);
    const bool negative = totalCentsSigned < 0;
    const unsigned long long absCents = negative
        ? static_cast<unsigned long long>(-(totalCentsSigned + 1)) + 1ULL
        : static_cast<unsigned long long>(totalCentsSigned);

    const unsigned long long whole = absCents / 100ULL;
    const unsigned long long cents = absCents % 100ULL;

    std::string wholeStr = std::to_string(whole);
    std::string grouped;
    grouped.reserve(wholeStr.size() + wholeStr.size() / 3 + 1);

    int count = 0;
    for (auto it = wholeStr.rbegin(); it != wholeStr.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            grouped.push_back(',');
        }
        grouped.push_back(*it);
        ++count;
    }
    std::reverse(grouped.begin(), grouped.end());

    std::ostringstream oss;
    if (negative) {
        oss << '-';
    }
    oss << grouped << '.' << std::setw(2) << std::setfill('0') << cents << ' ' << currency;
    return oss.str();
}

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &nowTime);
#else
    localtime_r(&nowTime, &localTm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string trim(const std::string& text) {
    auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    auto first = std::find_if_not(text.begin(), text.end(), isSpace);
    if (first == text.end()) {
        return "";
    }
    auto last = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
    return std::string(first, last);
}

std::string sanitizeField(const std::string& text) {
    std::string cleaned = trim(text);
    for (char& ch : cleaned) {
        if (ch == '|' || ch == '\n' || ch == '\r') {
            ch = '/';
        }
    }
    return cleaned;
}

std::vector<std::string> splitDelimited(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = line.find(delimiter, start);
        if (pos == std::string::npos) {
            parts.push_back(line.substr(start));
            break;
        }
        parts.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

bool isValidAccountNumber(const std::string& accountNumber) {
    if (accountNumber.size() < 3 || accountNumber.size() > 12) {
        return false;
    }
    return std::all_of(accountNumber.begin(), accountNumber.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

bool isValidPin(const std::string& pin) {
    if (pin.size() != 4) {
        return false;
    }
    return std::all_of(pin.begin(), pin.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

double parseDoubleStrict(const std::string& text) {
    const std::string cleaned = trim(text);
    if (cleaned.empty()) {
        throw InvalidInputException("Input cannot be empty. Please enter a valid number.");
    }

    std::size_t idx = 0;
    double value = 0.0;
    try {
        value = std::stod(cleaned, &idx);
    } catch (const std::invalid_argument&) {
        throw InvalidInputException("Invalid numeric input: '" + cleaned + "'. Letters or symbols are not allowed.");
    } catch (const std::out_of_range&) {
        throw InvalidInputException("Numeric value is out of range: '" + cleaned + "'.");
    }

    if (idx != cleaned.size() || !std::isfinite(value)) {
        throw InvalidInputException("Invalid numeric input: '" + cleaned + "'.");
    }
    return value;
}

double parsePositiveAmount(const std::string& text) {
    const double amount = parseDoubleStrict(text);
    if (amount <= 0.0) {
        throw InvalidAmountException("Amount must be greater than zero.");
    }
    return amount;
}

int parseIntStrict(const std::string& text) {
    const std::string cleaned = trim(text);
    if (cleaned.empty()) {
        throw InvalidInputException("Input cannot be empty. Please enter a valid integer.");
    }

    std::size_t idx = 0;
    long long parsed = 0;
    try {
        parsed = std::stoll(cleaned, &idx, 10);
    } catch (const std::invalid_argument&) {
        throw InvalidInputException("Invalid integer input: '" + cleaned + "'.");
    } catch (const std::out_of_range&) {
        throw InvalidInputException("Integer value is out of range: '" + cleaned + "'.");
    }

    if (idx != cleaned.size() ||
        parsed < static_cast<long long>(std::numeric_limits<int>::min()) ||
        parsed > static_cast<long long>(std::numeric_limits<int>::max())) {
        throw InvalidInputException("Invalid integer input: '" + cleaned + "'.");
    }
    return static_cast<int>(parsed);
}

} // namespace bank::utils

