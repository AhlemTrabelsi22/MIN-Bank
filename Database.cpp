
#include "Database.h"

#include <iostream>

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