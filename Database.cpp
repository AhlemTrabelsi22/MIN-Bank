
#include "Database.h"

#include <iostream>
#include <algorithm>
#include <map>
Database::Database(const std::string& connectionString) {
    connection = PQconnectdb(connectionString.c_str());

    if (connection == nullptr ||
        PQstatus(connection) != CONNECTION_OK) {

        std::cerr << "Erreur de connexion PostgreSQL : ";

        if (connection != nullptr) {
            std::cerr << PQerrorMessage(connection);
        }

        std::cerr << '\n';

    } else {
        std::cout << "Connexion PostgreSQL reussie !\n";
    }
}

Database::~Database() {
    if (connection != nullptr) {
        PQfinish(connection);
    }
}

bool Database::isConnected() const {
    return connection != nullptr &&
           PQstatus(connection) == CONNECTION_OK;
}

PGresult* Database::execute(const std::string& query) {
    if (!isConnected()) {
        return nullptr;
    }

    PGresult* result = PQexec(connection, query.c_str());

    if (result == nullptr) {
        std::cerr << "Echec de l'execution SQL.\n";
        return nullptr;
    }

    ExecStatusType status = PQresultStatus(result);

    if (status != PGRES_COMMAND_OK &&
        status != PGRES_TUPLES_OK) {

        std::cerr << "Erreur SQL : "
                  << PQresultErrorMessage(result)
                  << '\n';

        PQclear(result);
        return nullptr;
    }

    return result;
}

PGresult* Database::executeParams(
    const std::string& query,
    const std::vector<std::string>& params
) {
    if (!isConnected()) {
        return nullptr;
    }

    std::vector<const char*> values;
    values.reserve(params.size());

    for (const std::string& param : params) {
        values.push_back(param.c_str());
    }

    PGresult* result = PQexecParams(
        connection,
        query.c_str(),
        static_cast<int>(values.size()),
        nullptr,
        values.empty() ? nullptr : values.data(),
        nullptr,
        nullptr,
        0
    );

    if (result == nullptr) {
        std::cerr << "Erreur d'execution SQL.\n";
        return nullptr;
    }

    ExecStatusType status = PQresultStatus(result);

    if (status != PGRES_COMMAND_OK &&
        status != PGRES_TUPLES_OK) {

        std::cerr << "Erreur SQL : "
                  << PQresultErrorMessage(result)
                  << '\n';

        PQclear(result);
        return nullptr;
    }

    return result;
}

std::string Database::transfer(
    int fromAccountId,
    int toAccountId,
    long long amount,
    long long& fromBalance,
    long long& toBalance
) {
    if (fromAccountId <= 0 || toAccountId <= 0 || amount <= 0) {
        return "INVALID";
    }

    if (fromAccountId == toAccountId) {
        return "SAME_ACCOUNT";
    }

    // Demarrer la transaction PostgreSQL.
    PGresult* result = execute("BEGIN");

    if (result == nullptr) {
        return "DB_ERROR";
    }

    PQclear(result);

    // Verrouiller les deux comptes dans un ordre stable.
    // Cela limite les risques de conflits entre virements simultanes.
    result = executeParams(
        "SELECT id, balance "
        "FROM accounts "
        "WHERE id IN ($1::integer, $2::integer) "
        "ORDER BY id FOR UPDATE",
        {
            std::to_string(fromAccountId),
            std::to_string(toAccountId)
        }
    );

    if (result == nullptr) {
        execute("ROLLBACK");
        return "DB_ERROR";
    }

    std::map<int, long long> balances;

    for (int i = 0; i < PQntuples(result); ++i) {
        balances[std::stoi(PQgetvalue(result, i, 0))] =
            std::stoll(PQgetvalue(result, i, 1));
    }

    PQclear(result);

    if (balances.count(fromAccountId) == 0 ||
        balances.count(toAccountId) == 0) {
        result = execute("ROLLBACK");
        if (result != nullptr) PQclear(result);
        return "NOT_FOUND";
    }

    if (balances[fromAccountId] < amount) {
        result = execute("ROLLBACK");
        if (result != nullptr) PQclear(result);
        return "INSUFFICIENT";
    }

    // Debiter le compte emetteur.
    result = executeParams(
        "UPDATE accounts "
        "SET balance = balance - $1::bigint "
        "WHERE id = $2::integer "
        "RETURNING balance",
        {
            std::to_string(amount),
            std::to_string(fromAccountId)
        }
    );

    if (result == nullptr || PQntuples(result) != 1) {
        if (result != nullptr) PQclear(result);
        execute("ROLLBACK");
        return "DB_ERROR";
    }

    fromBalance = std::stoll(PQgetvalue(result, 0, 0));
    PQclear(result);

    // Crediter le compte destinataire.
    result = executeParams(
        "UPDATE accounts "
        "SET balance = balance + $1::bigint "
        "WHERE id = $2::integer "
        "RETURNING balance",
        {
            std::to_string(amount),
            std::to_string(toAccountId)
        }
    );

    if (result == nullptr || PQntuples(result) != 1) {
        if (result != nullptr) PQclear(result);
        execute("ROLLBACK");
        return "DB_ERROR";
    }

    toBalance = std::stoll(PQgetvalue(result, 0, 0));
    PQclear(result);

    // Enregistrer le virement dans le meme contexte transactionnel.
    result = executeParams(
        "INSERT INTO transactions "
        "(from_account_id, to_account_id, amount, type, status) "
        "VALUES ($1::integer, $2::integer, $3::bigint, "
        "'VIREMENT', 'REUSSI')",
        {
            std::to_string(fromAccountId),
            std::to_string(toAccountId),
            std::to_string(amount)
        }
    );

    if (result == nullptr) {
        execute("ROLLBACK");
        return "DB_ERROR";
    }

    PQclear(result);

    // Valider debit, credit et historique ensemble.
    result = execute("COMMIT");

    if (result == nullptr) {
        execute("ROLLBACK");
        return "DB_ERROR";
    }

    PQclear(result);
    return "OK";
}

std::string Database::deposit(
    int accountId,
    long long amount,
    long long& newBalance
) {
    if (accountId <= 0 || amount <= 0) {
        return "INVALID";
    }

    PGresult* result = execute("BEGIN");
    if (result == nullptr) {
        return "DB_ERROR";
    }
    PQclear(result);

    result = executeParams(
        "UPDATE accounts "
        "SET balance = balance + $1::bigint "
        "WHERE id = $2::integer "
        "RETURNING balance",
        {std::to_string(amount), std::to_string(accountId)}
    );

    if (result == nullptr) {
        execute("ROLLBACK");
        return "DB_ERROR";
    }

    if (PQntuples(result) == 0) {
        PQclear(result);
        result = execute("ROLLBACK");
        if (result != nullptr) PQclear(result);
        return "NOT_FOUND";
    }

    newBalance = std::stoll(PQgetvalue(result, 0, 0));
    PQclear(result);

    result = executeParams(
        "INSERT INTO transactions "
        "(from_account_id, to_account_id, amount, type, status) "
        "VALUES (NULL, $1::integer, $2::bigint, 'DEPOT', 'REUSSI')",
        {std::to_string(accountId), std::to_string(amount)}
    );

    if (result == nullptr) {
        execute("ROLLBACK");
        return "DB_ERROR";
    }
    PQclear(result);

    result = execute("COMMIT");
    if (result == nullptr) {
        execute("ROLLBACK");
        return "DB_ERROR";
    }
    PQclear(result);

    return "OK";
}

