# Class Diagram / الرسم التخطيطي للفئات

This document illustrates the Object-Oriented Architecture of the **Bank Account & ATM Transaction Simulator**, highlighting inheritance, polymorphism, composition, and exception dependencies across `Account`, `SavingsAccount`, `CheckingAccount`, `ATM`, `AuditLogger`, `TransactionRecord`, and the `BankException` hierarchy.

يوضح هذا المستند البنية الكائنية (OOP) لمحاكي المعاملات البنكية والصراف الآلي، بما في ذلك علاقات الوراثة، التعدد الشكلي (Polymorphism)، الاحتواء (Composition)، ونظام الاستثناءات المخصصة.

---

## UML Class Diagram (Mermaid)

```mermaid
classDiagram
    direction TB

    class Account {
        <<abstract>>
        +int MAX_PIN_ATTEMPTS$
        #string accountNumber_
        #string accountHolderName_
        #double balance_
        #string pin_
        #int failedPinAttempts_
        #bool locked_
        +Account(accountNumber, holderName, initialBalance, pin, failedPinAttempts, locked)
        +~Account()*
        +getAccountType()* string
        +deposit(amount: double) void
        +withdraw(amount: double)* void
        +formatSummary(os: ostream&) void
        +displayDetails(os: ostream&) void
        +serialize()* string
        +authenticate(enteredPin: string) void
        +unlock() void
        +getAccountNumber() string
        +getAccountHolderName() string
        +getBalance() double
        +isLocked() bool
        +operator==(other: Account&) bool
        +operator!=(other: Account&) bool
        +operator<(other: Account&) bool
        +operator>(other: Account&) bool
        +operator<=(other: Account&) bool
        +operator>=(other: Account&) bool
        +operator<<(os: ostream&, account: Account&) ostream&
    }

    class SavingsAccount {
        +double DEFAULT_MINIMUM_BALANCE$
        +double DEFAULT_ANNUAL_INTEREST_RATE$
        -double minimumBalance_
        -double annualInterestRate_
        +SavingsAccount(...)
        +getAccountType() string
        +withdraw(amount: double) void
        +formatSummary(os: ostream&) void
        +displayDetails(os: ostream&) void
        +serialize() string
        +calculateProjectedInterest(fractionOfYear: double) double
        +applyInterest(fractionOfYear: double) double
        +getMinimumBalance() double
        +getAnnualInterestRate() double
        +getAvailableForWithdrawal() double
    }

    class CheckingAccount {
        +double DEFAULT_OVERDRAFT_LIMIT$
        +double DEFAULT_OVERDRAFT_FEE$
        -double overdraftLimit_
        -double overdraftFee_
        -double lastFeeCharged_
        +CheckingAccount(...)
        +getAccountType() string
        +withdraw(amount: double) void
        +formatSummary(os: ostream&) void
        +displayDetails(os: ostream&) void
        +serialize() string
        +getOverdraftLimit() double
        +getOverdraftFee() double
        +getLastFeeCharged() double
        +getAvailableFunds() double
    }

    class ATM {
        +const char* DEFAULT_ADMIN_PIN$
        -string accountsFilePath_
        -string transactionsFilePath_
        -AuditLogger logger_
        -string adminPin_
        -bool seededOnInit_
        -vector~unique_ptr~Account~~ accounts_
        -vector~TransactionRecord~ transactions_
        +ATM(accountsFile, transactionsFile, auditFile, adminPin)
        +initialize() void
        +login(accountNumber: string, pin: string) Account&
        +createSavingsAccount(...) Account&
        +createCheckingAccount(...) Account&
        +deposit(account: Account&, amount: double) void
        +withdraw(account: Account&, amount: double) void
        +transfer(source: Account&, targetAccountNumber: string, amount: double) void
        +applySavingsInterest(savings: SavingsAccount&, fractionOfYear: double) double
        +unlockAccount(accountNumber: string, adminPin: string) void
        +compareAccounts(first: string, second: string) string
        +getMiniStatement(accountNumber: string, maxCount: size_t) vector~TransactionRecord~
        +findAccount(accountNumber: string) Account*
        +saveAccounts() void
        +saveTransactions() void
        +run(in: istream&, out: ostream&) void
    }

    class AuditLogger {
        -string filePath_
        +AuditLogger(filePath: string)
        +log(eventType: string, accountNumber: string, success: bool, details: string) void
        +getFilePath() string
    }

    class TransactionRecord {
        +string timestamp
        +string accountNumber
        +string type
        +double amount
        +double balanceAfter
        +string details
        +serialize() string
        +deserialize(line: string)$ TransactionRecord
    }

    class BankException {
        +BankException(message: string)
    }

    class InsufficientFundsException
    class InvalidAmountException
    class InvalidPinException
    class AccountLockedException
    class AccountNotFoundException
    class DuplicateAccountException
    class InvalidInputException
    class PersistenceException

    Account <|-- SavingsAccount : inherits
    Account <|-- CheckingAccount : inherits

    ATM *-- "0..*" Account : manages (unique_ptr)
    ATM *-- "1" AuditLogger : owns
    ATM *-- "0..*" TransactionRecord : stores

    BankException <|-- InsufficientFundsException
    BankException <|-- InvalidAmountException
    BankException <|-- InvalidPinException
    BankException <|-- AccountLockedException
    BankException <|-- AccountNotFoundException
    BankException <|-- DuplicateAccountException
    BankException <|-- InvalidInputException
    BankException <|-- PersistenceException

    ATM ..> BankException : throws & handles
    Account ..> BankException : throws
```

---

## Relationships Summary / ملخص العلاقات

| Relationship | Classes | Description (EN) | الوصف (AR) |
| :--- | :--- | :--- | :--- |
| **Inheritance** | `SavingsAccount`, `CheckingAccount` $\rightarrow$ `Account` | Derived classes implement specialized `withdraw()`, `displayDetails()`, `formatSummary()`, and `serialize()`. | الفئات المشتقة ترث الخصائص المشتركة وتعيد تعريف السحب وعرض التفاصيل بتعدد شكلي. |
| **Composition** | `ATM` $\rightarrow$ `Account` (`unique_ptr`) | `ATM` owns and manages the lifecycle of polymorphic accounts loaded from or saved to `accounts.txt`. | تدير فئة `ATM` دورة حياة جميع الحسابات المحملة والمحفوظة في `accounts.txt`. |
| **Composition** | `ATM` $\rightarrow$ `AuditLogger` | `ATM` logs every successful and failed login, transaction, lockout, and validation error to `audit.txt`. | تسجل `ATM` جميع العمليات الناجحة والفاشلة بالطابع الزمني في `audit.txt`. |
| **Composition** | `ATM` $\rightarrow$ `TransactionRecord` | `ATM` records and persists structured account movements to `transactions.txt` for Mini-Statements. | تحفظ `ATM` حركات كل حساب في `transactions.txt` لعرض كشف الحساب المختصر. |
| **Dependency** | `ATM`, `Account` hierarchy $\rightarrow$ `BankException` | Domain and input errors raise typed exceptions derived from `std::runtime_error`. | تُرمى استثناءات مخصصة عند تجاوز الرصيد أو إدخال قيم سالبة أو أخطاء PIN أو المدخلات غير الرقمية. |

> [!TIP]
> A standalone PlantUML version is also available in [`class_diagram.puml`](class_diagram.puml).

