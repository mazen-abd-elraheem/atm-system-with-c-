#ifndef BANK_ATM_UTILS_HPP
#define BANK_ATM_UTILS_HPP

#include <string>
#include <vector>

namespace bank::utils {

/**
 * @brief Formats a monetary value with thousands separators and currency suffix (default: EGP).
 */
std::string formatCurrency(double amount, const std::string& currency = "EGP");

/**
 * @brief Returns the current local timestamp formatted as "YYYY-MM-DD HH:MM:SS".
 */
std::string currentTimestamp();

/**
 * @brief Removes leading and trailing ASCII whitespace.
 */
std::string trim(const std::string& text);

/**
 * @brief Replaces pipe delimiters and newlines so serialized lines stay well-formed.
 */
std::string sanitizeField(const std::string& text);

/**
 * @brief Splits a delimited record line while preserving trailing empty fields.
 */
std::vector<std::string> splitDelimited(const std::string& line, char delimiter = '|');

/**
 * @brief Validates that an account number consists of 3 to 12 ASCII digits.
 */
bool isValidAccountNumber(const std::string& accountNumber);

/**
 * @brief Validates that a PIN consists of exactly 4 ASCII digits.
 */
bool isValidPin(const std::string& pin);

/**
 * @brief Strictly parses a floating-point value from text, throwing InvalidInputException on failure.
 */
double parseDoubleStrict(const std::string& text);

/**
 * @brief Strictly parses a monetary amount and ensures it is strictly positive (> 0).
 * @throws InvalidInputException for non-numeric text.
 * @throws InvalidAmountException for zero or negative amounts.
 */
double parsePositiveAmount(const std::string& text);

/**
 * @brief Strictly parses a signed integer from text, throwing InvalidInputException on failure.
 */
int parseIntStrict(const std::string& text);

} // namespace bank::utils

#endif // BANK_ATM_UTILS_HPP

