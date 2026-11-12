# User Test Script / سيناريو الاختبار الشامل

This document provides a complete end-to-end **User Test Script** verifying all functional, security, exception-handling, polymorphism, operator-overloading, and persistence requirements of the **Bank Account & ATM Transaction Simulator**.

يوفر هذا المستند سيناريو اختبار عملياً خطوة بخطوة للتحقق من جميع حالات الاستخدام والأخطاء المتوقعة في النظام.

---

## 0. Pre-Conditions & Seeded Test Data / بيانات البدء الافتراضية

Run from a clean working directory (no prior `accounts.txt`, `transactions.txt`, or `audit.txt`) so the ATM seeds the default accounts automatically:

| Account Number | Account Type | Holder Name | PIN | Initial Balance | Special Rules |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `1001` | Savings | Ahmed Hassan | `1234` | `5,000.00 EGP` | Min Balance: `500.00 EGP`, Rate: `5.00%` |
| `2001` | Checking | Sara Mohamed | `4321` | `1,500.00 EGP` | Overdraft Limit: `1,000.00 EGP`, Fee: `35.00 EGP` |
| `1002` | Savings | Omar Khaled | `5678` | `2,500.00 EGP` | Min Balance: `500.00 EGP`, Rate: `5.00%` |

- **Administrator Unlock PIN / رمز فك القفل الإداري**: `9999`

---

## Test Case 1: Invalid Non-Numeric Input Handling / التحقق من المدخلات غير الرقمية

**Objective / الهدف**: Verify the application does not crash when letters are entered instead of numbers.

| Step | Menu / Prompt | User Input | Expected Result |
| :--- | :--- | :--- | :--- |
| 1.1 | Main Menu (`Select an option (1-5):`) | `abc` | Prints `[ERROR] Invalid integer input: 'abc'.`, logs `[FAILED] [VALIDATION_ERROR]` to `audit.txt`, and safely re-displays the Main Menu. |
| 1.2 | Main Menu (`Select an option (1-5):`) | `9` | Prints `[ERROR] Menu choice must be between 1 and 5.` and safely re-displays the Main Menu. |

---

## Test Case 2: Wrong PIN Attempts, Account Lockout & Admin Unlock / أخطاء الـ PIN، قفل الحساب، وفك القفل

**Objective / الهدف**: Verify 3 consecutive wrong PINs lock the account, persist the lock, and require Admin unlock.

| Step | Menu / Prompt | User Input | Expected Result |
| :--- | :--- | :--- | :--- |
| 2.1 | Main Menu | `1` | Prompts for Account Number. |
| 2.2 | Account Number | `1001` | Prompts for 4-digit PIN. |
| 2.3 | PIN (Attempt 1) | `0000` | Prints `[AUTH ERROR] Incorrect PIN. Remaining attempts before lockout: 2.` and logs `[FAILED] [LOGIN]`. |
| 2.4 | PIN (Attempt 2) | `1111` | Prints `[AUTH ERROR] Incorrect PIN. Remaining attempts before lockout: 1.` and logs `[FAILED] [LOGIN]`. |
| 2.5 | PIN (Attempt 3) | `2222` | Prints `[LOCKED] Account #1001 has been locked after 3 consecutive failed PIN attempts.` and logs `[FAILED] [ACCOUNT_LOCKED]`. |
| 2.6 | Main Menu -> `1` -> `1001` | (select `1`, enter `1001`) | Immediately rejects with `[ERROR] Account #1001 is locked. Contact an administrator to unlock it.` |
| 2.7 | Main Menu -> `4` (Admin Unlock) | `1001`, then wrong admin PIN `0000` | Prints `[ERROR] Invalid administrator PIN.` and logs `[FAILED] [ADMIN_UNLOCK]`. |
| 2.8 | Main Menu -> `4` (Admin Unlock) | `1001`, then valid admin PIN `9999` | Prints `[OK] Account #1001 has been unlocked.` and logs `[SUCCESS] [ACCOUNT_UNLOCKED]`. |

---

## Test Case 3: Valid Login, Deposit Validation, Insufficient Funds & Interest / تسجيل دخول صحيح، إيداع، سحب، وفوائد التوفير

**Objective / الهدف**: Verify valid PIN login, negative/text amount rejection, Savings minimum balance enforcement (`500.00 EGP`), and interest calculation.

| Step | Menu / Prompt | User Input | Expected Result |
| :--- | :--- | :--- | :--- |
| 3.1 | Main Menu -> `1` | `1001`, then `1234` | Prints `Login successful! Welcome, Ahmed Hassan.` and displays formatted summary via `<<`. |
| 3.2 | Account Menu -> `1` (Full Details) | `1` | Polymorphically displays Savings details including `Minimum Balance: 500.00 EGP`, `Withdrawable: 4,500.00 EGP`, `Interest Rate: 5.00% / year`, and projected monthly/annual interest. |
| 3.3 | Account Menu -> `2` (Deposit) | `hello` | Prints `[ERROR] Invalid numeric input: 'hello'. Letters or symbols are not allowed.` |
| 3.4 | Account Menu -> `2` (Deposit) | `-250` | Prints `[ERROR] Amount must be greater than zero.` and logs `[FAILED] [DEPOSIT]`. |
| 3.5 | Account Menu -> `2` (Deposit) | `500` | Prints `[OK] Deposit successful.` Balance updates to `5,500.00 EGP`. |
| 3.6 | Account Menu -> `3` (Withdraw) | `5200` | Rejected with `[ERROR] Insufficient funds: withdrawal of 5,200.00 EGP violates the minimum balance requirement of 500.00 EGP. Maximum withdrawable: 5,000.00 EGP.` |
| 3.7 | Account Menu -> `6` (Savings Interest) | `2` (Apply Annual Interest) | Credits `275.00 EGP` (`5%` of `5,500.00 EGP`), balance becomes `5,775.00 EGP`, logged in `transactions.txt` & `audit.txt`. |

---

## Test Case 4: Successful Transfer & Self-Transfer Guard / التحويل الناجح ومنع التحويل لنفس الحساب

**Objective / الهدف**: Verify transfers update both accounts atomically and self-transfer is rejected using `operator==`.

| Step | Menu / Prompt | User Input | Expected Result |
| :--- | :--- | :--- | :--- |
| 4.1 | Account Menu (`#1001`) -> `4` (Transfer) | Target `1001`, Amount `100` | Prints `[ERROR] Source and destination accounts cannot be the same.` |
| 4.2 | Account Menu (`#1001`) -> `4` (Transfer) | Target `9999`, Amount `100` | Prints `[ERROR] Target account #9999 was not found.` |
| 4.3 | Account Menu (`#1001`) -> `4` (Transfer) | Target `2001`, Amount `775` | Prints `[OK] Transfer successful.` `#1001` becomes `5,000.00 EGP`; `#2001` becomes `2,275.00 EGP`. |
| 4.4 | Account Menu (`#1001`) -> `5` (Mini-Statement) | `5` | Displays timestamped rows (`ACCOUNT_OPENED`, `DEPOSIT`, `INTEREST`, `TRANSFER_OUT`) with `EGP` balances. |
| 4.5 | Account Menu -> `7` (Logout) | `7` | Logs out to Main Menu and records `[SUCCESS] [LOGOUT]`. |

---

## Test Case 5: Checking Account Overdraft & Fee / السحب على المكشوف في الحساب الجاري

**Objective / الهدف**: Verify `CheckingAccount` allows overdraft up to `1,000.00 EGP`, charges `35.00 EGP` flat overdraft fee, and rejects withdrawals exceeding limit.

| Step | Menu / Prompt | User Input | Expected Result |
| :--- | :--- | :--- | :--- |
| 5.1 | Main Menu -> `1` | `2001`, then `4321` | Logs in as `Sara Mohamed` (balance `2,275.00 EGP`). |
| 5.2 | Account Menu -> `3` (Withdraw) | `2500` | Succeeds! Overdraft triggered: `2,275 - 2,500 - 35 = -260.00 EGP`. |
| 5.3 | Account Menu -> `5` (Mini-Statement) | `5` | Shows `WITHDRAW` (`2,500.00 EGP`, Bal `-225.00 EGP`) followed by `OVERDRAFT_FEE` (`35.00 EGP`, Bal `-260.00 EGP`). |
| 5.4 | Account Menu -> `3` (Withdraw) | `800` | Rejected! `-260 - 800 - 35 = -1,095.00 EGP` exceeds `-1,000.00 EGP` overdraft limit. |
| 5.5 | Account Menu -> `6` (Savings Interest) | `6` | Prints `[INFO] Interest calculation is only available for Savings Accounts.` |
| 5.6 | Account Menu -> `7` (Logout) | `7` | Returns to Main Menu. |

---

## Test Case 6: Account Creation, Duplicate Detection & Account Comparison / إنشاء حساب، فحص التكرار، ومقارنة الحسابات

**Objective / الهدف**: Verify creating a new account, duplicate rejection via `==`, and balance comparison via `<` / `>`.

| Step | Menu / Prompt | User Input | Expected Result |
| :--- | :--- | :--- | :--- |
| 6.1 | Main Menu -> `2` (Create Account) | Type `1` (Savings), Acc `3001`, Name `Mariam Adel`, PIN `2468`, Deposit `1200` | Prints `[OK] Created: [Savings] #3001 | Mariam Adel | Balance: 1,200.00 EGP ...` |
| 6.2 | Main Menu -> `2` (Create Account) | Type `2` (Checking), Acc `3001`, Name `Test`, PIN `1111`, Deposit `500` | Rejected with `[ERROR] Account number #3001 already exists.` |
| 6.3 | Main Menu -> `3` (Compare Accounts) | First `1001`, Second `2001` | Displays formatted summaries via `<<`, `Identity (==): Different account numbers`, and `Balance (>): Account #1001 has a HIGHER balance than #2001...`. |
| 6.4 | Main Menu -> `5` (Exit) | `5` | Prints goodbye message and logs `[SUCCESS] [SYSTEM_EXIT]`. |

---

## Test Case 7: Automated Verification Suite / التحقق الآلي

In addition to manual interactive testing, run the automated C++ test binary (`test_bank_system`) which programmatically validates all 5 test suites above (including persistence across restarts and full piped interactive simulation):

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
```

