
#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>

int main() {
    const char* password = std::getenv("MINBANK_DB_PASSWORD");

    if (password == nullptr) {
        std::cerr << "Mot de passe non configure.\n";
        return 1;
    }

    const char* keywords[] = {
        "host", "port", "dbname", "user", "password", nullptr
    };

    const char* values[] = {
        "localhost", "5432", "min_bank", "postgres",
        password, nullptr
    };

    PGconn* connection = PQconnectdbParams(
        keywords, values, 0
    );

    if (PQstatus(connection) == CONNECTION_OK) {
        std::cout << "Connexion PostgreSQL reussie !\n";
    } else {
        std::cerr << "Erreur : "
                  << PQerrorMessage(connection) << '\n';
    }

    PQfinish(connection);
    return 0;
}