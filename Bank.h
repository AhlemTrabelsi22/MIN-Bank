#pragma once

#include "Account.h"
#include "Transaction.h"
#include "Database.h"

#include <string>

class Bank {
private:
    Database& database;

public:
    explicit Bank(Database& database);

    bool createAccount(
        int id,
        const std::string& owner,
        long long initialBalance = 0
    );

    Account* findAccount(int id);

    bool transfer(int fromId, int toId, long long amount);

    bool deposit(int accountId, long long amount);

    bool withdraw(int accountId, long long amount);

    void displayAccounts() const;

    void displayHistory() const;
};