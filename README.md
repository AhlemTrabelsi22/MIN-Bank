# MIN-Bank — Banking Transaction Engine

A C++ banking transaction engine connected to PostgreSQL, designed to manage bank accounts and execute financial operations with transactional consistency.

## Overview

MIN-Bank is a personal academic project developed in C++17. It provides a simple banking backend with persistent data storage using PostgreSQL.

The application currently runs through a console interface and supports account management, deposits, withdrawals, transfers, and transaction history.

## Features

* **Account Management:** Create and list bank accounts.
* **Deposits:** Add funds to an account.
* **Withdrawals:** Withdraw funds while checking the available balance.
* **Transfers:** Transfer funds between two accounts.
* **Transaction History:** Store and display transaction details, timestamps, and statuses.
* **Database Integration:** Persist accounts and transactions in PostgreSQL.
* **SQL Parameterization:** Use parameterized queries to handle input values safely.
* **Transactional Operations:** Use SQL transactions to keep account balances and transaction records consistent.
* **Input Validation:** Reject invalid account identifiers and non-positive transaction amounts.

## Tech Stack

* **Language:** C++17
* **Database:** PostgreSQL
* **Database Driver:** libpq
* **Compiler:** GCC / MinGW-w64
* **Version Control:** Git and GitHub

## Project Structure

```text
MIN-Bank/
├── main.cpp
├── Account.h
├── Account.cpp
├── Bank.h
├── Bank.cpp
├── Transaction.h
├── Database.h
├── Database.cpp
├── README.md
└── .gitignore
```

## Database Schema

### Accounts

The `accounts` table stores bank account information.

| Column    | Description                 |
| --------- | --------------------------- |
| `id`      | Unique account identifier   |
| `owner`   | Account holder              |
| `balance` | Account balance in millimes |

### Transactions

The `transactions` table stores financial operations.

| Column            | Description                          |
| ----------------- | ------------------------------------ |
| `id`              | Unique transaction identifier        |
| `from_account_id` | Source account, when applicable      |
| `to_account_id`   | Destination account, when applicable |
| `amount`          | Transaction amount in millimes       |
| `type`            | `DEPOT`, `RETRAIT`, or `VIREMENT`    |
| `status`          | Transaction status                   |
| `created_at`      | Transaction timestamp                |

Amounts are stored as integer millimes to avoid floating-point rounding errors.

## Prerequisites

Before running the project, install:

* GCC with C++17 support
* PostgreSQL
* PostgreSQL development libraries (`libpq`)
* Git

## Database Setup

Create a PostgreSQL database named `min_bank` and execute the following SQL:

```sql
CREATE TABLE accounts (
    id INTEGER PRIMARY KEY,
    owner VARCHAR(100) NOT NULL,
    balance BIGINT NOT NULL DEFAULT 0,
    CHECK (id > 0),
    CHECK (balance >= 0)
);

CREATE TABLE transactions (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    from_account_id INTEGER,
    to_account_id INTEGER,
    amount BIGINT NOT NULL,
    type VARCHAR(20) NOT NULL,
    status VARCHAR(20) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (from_account_id) REFERENCES accounts(id),
    FOREIGN KEY (to_account_id) REFERENCES accounts(id),
    CHECK (amount > 0),
    CHECK (type IN ('DEPOT', 'RETRAIT', 'VIREMENT')),
    CHECK (status IN ('REUSSI', 'REFUSE'))
);
```

## Configuration

Set the PostgreSQL password in an environment variable instead of hardcoding it in the source code.

For MSYS2 UCRT64:

```bash
export MINBANK_DB_PASSWORD='your_local_password'
```

Configure the connection string in the application to use the local PostgreSQL database `min_bank`.

Never commit passwords, API keys, or other secrets to GitHub.

## Build and Run

From the project directory, compile the application:

```bash
g++ -std=c++17 -Wall -Wextra \
    main.cpp Account.cpp Bank.cpp Database.cpp \
    -o min-bank.exe -lpq
```

Run the application:

```bash
./min-bank.exe
```

## Console Menu

```text
===== MIN-BANK =====
1. Create an account
2. Display accounts
3. Make a deposit
4. Make a withdrawal
5. Transfer funds
6. View transaction history
0. Exit
```

## Testing

The following operations have been tested manually against the local PostgreSQL database:

* Account creation and listing
* Deposits and balance updates
* Withdrawals
* Transfers between accounts
* Transaction history retrieval

## Roadmap

* Develop a REST API in C++ to expose banking operations.
* Build an Angular frontend.
* Add automated unit and integration tests.
* Improve error handling and logging.
* Add authentication and authorization.
* Add Docker-based development and deployment.

## Author

Developed as a personal academic project to practice C++, object-oriented programming, SQL database integration, and transaction management.
