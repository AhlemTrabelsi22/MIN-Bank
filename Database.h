
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

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
};