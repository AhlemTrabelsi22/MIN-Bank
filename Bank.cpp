
#include "Bank.h"
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

Bank::Bank(Database& database)
    : database(database) {
}

// =====================================================
// CREATION D'UN COMPTE
// =====================================================
bool Bank::createAccount(
    int id,
    const std::string& owner,
    long long initialBalance
) {
    if (id <= 0 || owner.empty() || initialBalance < 0) {
        return false;
    }

    PGresult* result = database.executeParams(
        "INSERT INTO accounts (id, owner, balance) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (id) DO NOTHING",
        {
            std::to_string(id),
            owner,
            std::to_string(initialBalance)
        }
    );

    if (result == nullptr) {
        return false;
    }

    bool created =
        std::string(PQcmdStatus(result)) == "INSERT 0 1";

    PQclear(result);

    return created;
}

// =====================================================
// RECHERCHE D'UN COMPTE
// =====================================================
Account* Bank::findAccount(int id) {
    PGresult* result = database.executeParams(
        "SELECT id, owner, balance "
        "FROM accounts WHERE id = $1",
        {std::to_string(id)}
    );

    if (result == nullptr) {
        return nullptr;
    }

    if (PQntuples(result) == 0) {
        PQclear(result);
        return nullptr;
    }

    Account* account = new Account(
        std::stoi(PQgetvalue(result, 0, 0)),
        PQgetvalue(result, 0, 1),
        std::stoll(PQgetvalue(result, 0, 2))
    );

    PQclear(result);

    return account;
}

// =====================================================
// AFFICHAGE DES COMPTES
// =====================================================
void Bank::displayAccounts() const {
    PGresult* result = database.execute(
        "SELECT id, owner, balance "
        "FROM accounts ORDER BY id"
    );

    if (result == nullptr) {
        std::cout << "Impossible de consulter les comptes.\n";
        return;
    }

    std::cout << "\n========== LISTE DES COMPTES ==========\n";

    int rows = PQntuples(result);

    if (rows == 0) {
        std::cout << "Aucun compte enregistre.\n";
    }

    for (int i = 0; i < rows; ++i) {
        long long balance =
            std::stoll(PQgetvalue(result, i, 2));

        std::cout << "\nNumero du compte : "
                  << PQgetvalue(result, i, 0) << '\n';

        std::cout << "Titulaire        : "
                  << PQgetvalue(result, i, 1) << '\n';

        std::cout << "Solde            : "
                  << balance / 1000 << '.'
                  << std::setfill('0') << std::setw(3)
                  << balance % 1000
                  << " TND"
                  << std::setfill(' ') << '\n';

        std::cout << "---------------------------------------\n";
    }

    PQclear(result);
}

// =====================================================
// DEPOT
// =====================================================
bool Bank::deposit(int accountId, long long amount) {
    if (accountId <= 0 || amount <= 0) {
        return false;
    }

    PGresult* result = database.execute("BEGIN");

    if (result == nullptr) {
        return false;
    }

    PQclear(result);

    result = database.executeParams(
        "UPDATE accounts "
        "SET balance = balance + $1 "
        "WHERE id = $2 "
        "AND balance <= 9223372036854775807 - $1",
        {
            std::to_string(amount),
            std::to_string(accountId)
        }
    );

    if (result == nullptr ||
        std::string(PQcmdStatus(result)) != "UPDATE 1") {

        if (result != nullptr) {
            PQclear(result);
        }

        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    result = database.executeParams(
        "INSERT INTO transactions "
        "(from_account_id, to_account_id, amount, type, status) "
        "VALUES (NULL, $1, $2, 'DEPOT', 'REUSSI')",
        {
            std::to_string(accountId),
            std::to_string(amount)
        }
    );

    if (result == nullptr) {
        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    result = database.execute("COMMIT");

    if (result == nullptr) {
        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    return true;
}

// =====================================================
// RETRAIT
// =====================================================
bool Bank::withdraw(int accountId, long long amount) {
    if (accountId <= 0 || amount <= 0) {
        return false;
    }

    PGresult* result = database.execute("BEGIN");

    if (result == nullptr) {
        return false;
    }

    PQclear(result);

    result = database.executeParams(
        "UPDATE accounts "
        "SET balance = balance - $1 "
        "WHERE id = $2 AND balance >= $1",
        {
            std::to_string(amount),
            std::to_string(accountId)
        }
    );

    if (result == nullptr ||
        std::string(PQcmdStatus(result)) != "UPDATE 1") {

        if (result != nullptr) {
            PQclear(result);
        }

        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    result = database.executeParams(
        "INSERT INTO transactions "
        "(from_account_id, to_account_id, amount, type, status) "
        "VALUES ($1, NULL, $2, 'RETRAIT', 'REUSSI')",
        {
            std::to_string(accountId),
            std::to_string(amount)
        }
    );

    if (result == nullptr) {
        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    result = database.execute("COMMIT");

    if (result == nullptr) {
        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    return true;
}

// =====================================================
// VIREMENT ENTRE DEUX COMPTES
// =====================================================
bool Bank::transfer(
    int fromId,
    int toId,
    long long amount
) {
    if (fromId <= 0 || toId <= 0 ||
        fromId == toId || amount <= 0) {
        return false;
    }

    PGresult* result = database.execute("BEGIN");

    if (result == nullptr) {
        return false;
    }

    PQclear(result);

    // Verrouiller les deux comptes dans l'ordre des identifiants
    // pour limiter les risques de blocage entre virements concurrents.
    result = database.executeParams(
        "SELECT id FROM accounts "
        "WHERE id = $1 OR id = $2 "
        "ORDER BY id FOR UPDATE",
        {
            std::to_string(fromId),
            std::to_string(toId)
        }
    );

    if (result == nullptr || PQntuples(result) != 2) {
        if (result != nullptr) {
            PQclear(result);
        }

        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    // Debiter le compte source.
    result = database.executeParams(
        "UPDATE accounts "
        "SET balance = balance - $1 "
        "WHERE id = $2 AND balance >= $1",
        {
            std::to_string(amount),
            std::to_string(fromId)
        }
    );

    if (result == nullptr ||
        std::string(PQcmdStatus(result)) != "UPDATE 1") {

        if (result != nullptr) {
            PQclear(result);
        }

        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    // Crediter le compte destinataire sans depasser BIGINT.
    result = database.executeParams(
        "UPDATE accounts "
        "SET balance = balance + $1 "
        "WHERE id = $2 "
        "AND balance <= 9223372036854775807 - $1",
        {
            std::to_string(amount),
            std::to_string(toId)
        }
    );

    if (result == nullptr ||
        std::string(PQcmdStatus(result)) != "UPDATE 1") {

        if (result != nullptr) {
            PQclear(result);
        }

        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    // Enregistrer le virement dans l'historique.
    result = database.executeParams(
        "INSERT INTO transactions "
        "(from_account_id, to_account_id, amount, type, status) "
        "VALUES ($1, $2, $3, 'VIREMENT', 'REUSSI')",
        {
            std::to_string(fromId),
            std::to_string(toId),
            std::to_string(amount)
        }
    );

    if (result == nullptr) {
        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    result = database.execute("COMMIT");

    if (result == nullptr) {
        PGresult* rollback = database.execute("ROLLBACK");

        if (rollback != nullptr) {
            PQclear(rollback);
        }

        return false;
    }

    PQclear(result);

    return true;
}

// =====================================================
// HISTORIQUE DES TRANSACTIONS
// =====================================================
void Bank::displayHistory() const {
    PGresult* result = database.execute(
        "SELECT id, from_account_id, to_account_id, "
        "amount, type, status, "
        "TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') "
        "FROM transactions "
        "ORDER BY id DESC"
    );

    if (result == nullptr) {
        std::cout << "Impossible de consulter l'historique.\n";
        return;
    }

    int rows = PQntuples(result);

    std::cout << "\n========== HISTORIQUE DES TRANSACTIONS ==========\n";

    if (rows == 0) {
        std::cout << "Aucune transaction enregistree.\n";
    }

    for (int i = 0; i < rows; ++i) {
        long long amount =
            std::stoll(PQgetvalue(result, i, 3));

        std::cout << "\nTransaction : "
                  << PQgetvalue(result, i, 0) << '\n';

        std::cout << "Type        : "
                  << PQgetvalue(result, i, 4) << '\n';

        std::cout << "Compte source : ";

        if (PQgetisnull(result, i, 1)) {
            std::cout << "--\n";
        } else {
            std::cout << PQgetvalue(result, i, 1) << '\n';
        }

        std::cout << "Compte destination : ";

        if (PQgetisnull(result, i, 2)) {
            std::cout << "--\n";
        } else {
            std::cout << PQgetvalue(result, i, 2) << '\n';
        }

        std::cout << "Montant     : "
                  << amount / 1000 << '.'
                  << std::setfill('0') << std::setw(3)
                  << amount % 1000
                  << " TND"
                  << std::setfill(' ') << '\n';

        std::cout << "Statut      : "
                  << PQgetvalue(result, i, 5) << '\n';

        std::cout << "Date        : "
                  << PQgetvalue(result, i, 6) << '\n';

        std::cout << "-----------------------------------------------\n";
    }

    PQclear(result);
}

