# Bank Account & ATM Transaction Simulator (C++17)

An interactive **Bank Account & ATM Transaction Simulator** written in modern C++17 demonstrating Object-Oriented Programming (abstraction, inheritance, runtime polymorphism), operator overloading, strict input validation & custom exception handling, file persistence, and timestamped audit logging.

---

## Project Structure

```text
.
├── CMakeLists.txt
├── README.md
├── include/
│   ├── ATM.hpp
│   ├── Account.hpp
│   ├── AuditLogger.hpp
│   ├── CheckingAccount.hpp
│   ├── Exceptions.hpp
│   ├── SavingsAccount.hpp
│   ├── Transaction.hpp
│   └── Utils.hpp
├── src/
│   ├── ATM.cpp
│   ├── Account.cpp
│   ├── AuditLogger.cpp
│   ├── CheckingAccount.cpp
│   ├── SavingsAccount.cpp
│   ├── Transaction.cpp
│   ├── Utils.cpp
│   └── main.cpp
├── tests/
│   └── test_bank_system.cpp
└── docs/
    ├── class_diagram.md
    ├── class_diagram.puml
    └── user_test_script.md
```

---

## Key Features Implemented

1. **OOP Architecture & Polymorphism**
   - **`Account` (Abstract Base Class)**: Holds `accountNumber_`, `accountHolderName_`, `balance_`, `pin_`, `failedPinAttempts_`, and `locked_`; defines pure virtual `withdraw()`, `getAccountType()`, and `serialize()`, plus virtual `deposit()`, `formatSummary()`, and `displayDetails()`.
   - **`SavingsAccount`**: Enforces a minimum balance (`500.00 EGP` default) during withdrawals/transfers and supports annual/monthly interest calculation & application (`5.00%` annual default).
   - **`CheckingAccount`**: Supports overdraft up to `-1,000.00 EGP` with an automatic `35.00 EGP` flat overdraft fee charged whenever a withdrawal or transfer enters or extends an overdraft.
   - **`ATM`**: Central controller handling authentication, account creation, deposits, withdrawals, atomic transfers, mini-statements, account comparisons, admin unlock, and persistence.

2. **Operator Overloading**
   - `operator<<`: Polymorphically formats account summary and balance in `EGP` (e.g., `5,000.00 EGP`).
   - `operator==` / `operator!=`: Compares account identity by account number (prevents duplicate accounts and self-transfers).
   - `operator<` / `operator>` / `operator<=` / `operator>=`: Compares account balances.

3. **Validation & Custom Exception Hierarchy**
   - Custom exceptions inheriting `std::runtime_error` (`BankException`, `InsufficientFundsException`, `InvalidAmountException`, `InvalidPinException`, `AccountLockedException`, `AccountNotFoundException`, `DuplicateAccountException`, `InvalidInputException`, `PersistenceException`).
   - Strict numeric parsing prevents crashes or infinite input loops when non-numeric text (e.g., `"abc"`, `"12xyz"`) is entered.
   - 3 consecutive incorrect PIN entries automatically lock the account and persist the lock state across restarts until unlocked by an administrator.

4. **Persistence & Audit Logging**
   - **`accounts.txt`**: Saves and reloads all accounts, balances, failed PIN attempt counts, and lock flags.
   - **`transactions.txt`**: Persists structured transaction history used for each account's Mini-Statement (last 10 transactions).
   - **`audit.txt`**: Appends timestamped `[YYYY-MM-DD HH:MM:SS] [SUCCESS|FAILED] [EVENT]` records for every login, lockout, unlock, deposit, withdrawal, transfer, account creation, and validation failure.

---

## Build & Run Instructions

### Option 1: CMake + Ninja (Recommended)

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
.\build\atm_simulator.exe
```

### Option 2: Direct `g++` Compilation

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/Utils.cpp src/Transaction.cpp src/AuditLogger.cpp src/Account.cpp src/SavingsAccount.cpp src/CheckingAccount.cpp src/ATM.cpp src/main.cpp -o atm_simulator.exe
.\atm_simulator.exe
```

To compile and run tests directly with `g++`:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/Utils.cpp src/Transaction.cpp src/AuditLogger.cpp src/Account.cpp src/SavingsAccount.cpp src/CheckingAccount.cpp src/ATM.cpp tests/test_bank_system.cpp -o test_bank_system.exe
.\test_bank_system.exe
```

---

## Default Demo Accounts & Admin PIN

On first run (when `accounts.txt` does not yet exist), the simulator seeds three sample accounts:

| Account # | Type | Holder | PIN | Initial Balance |
| :--- | :--- | :--- | :--- | :--- |
| `1001` | Savings | Ahmed Hassan | `1234` | `5,000.00 EGP` |
| `2001` | Checking | Sara Mohamed | `4321` | `1,500.00 EGP` |
| `1002` | Savings | Omar Khaled | `5678` | `2,500.00 EGP` |

- **Administrator Unlock PIN**: `9999`

---

## Deliverables & Documentation

- **Class Diagram**: [`docs/class_diagram.md`](docs/class_diagram.md) and [`docs/class_diagram.puml`](docs/class_diagram.puml)
- **User Test Script**: [`docs/user_test_script.md`](docs/user_test_script.md)

