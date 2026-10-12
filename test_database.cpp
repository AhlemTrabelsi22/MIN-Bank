
#include "Database.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    const char* password = std::getenv("MINBANK_DB_PASSWORD");

    if (password == nullptr) {
        std::cerr << "Mot de passe non configure.\n";
        return 1;
    }

    std::string connectionString =
        "host=localhost "
        "port=5432 "
        "dbname=min_bank "
        "user=postgres "
        "password=";

    connectionString += password;

    Database database(connectionString);

    if (!database.isConnected()) {
        std::cerr << "Connexion impossible.\n";
        return 1;
    }

    PGresult* result = database.execute(
        "INSERT INTO accounts (id, owner, balance) "
        "VALUES (1003, 'Test C++', 25000) "
        "ON CONFLICT (id) DO NOTHING"
    );

    if (result == nullptr) {
        std::cerr << "Insertion echouee.\n";
        return 1;
    }

    std::cout << "Requete SQL executee avec succes !\n";

    PQclear(result);

    return 0;
}