
#pragma once

#include <libpq-fe.h>
#include <string>
#include <vector>

class Database {
private:
    PGconn* connection;

public:
    explicit Database(const std::string& connectionString);

    ~Database();

    bool isConnected() const;

    PGresult* execute(const std::string& query);

    PGresult* executeParams(
        const std::string& query,
        const std::vector<std::string>& params
    );
        std::string transfer(
        int fromAccountId,
        int toAccountId,
        long long amount,
        long long& fromBalance,
        long long& toBalance
    );
    
    std::string deposit(
    int accountId,
    long long amount,
    long long& newBalance
);

   std::string withdraw(
    int accountId,
    long long amount,
    long long& newBalance
);


    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
};