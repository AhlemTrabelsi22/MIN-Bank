
#include "Database.h"

#include <drogon/drogon.h>
#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
    const char* password = std::getenv("MINBANK_DB_PASSWORD");

    if (password == nullptr)
    {
        std::cerr << "Mot de passe PostgreSQL non configure.\n";
        return 1;
    }

    std::string connectionString =
        "host=localhost port=5432 "
        "dbname=min_bank user=postgres password=";

    connectionString += password;

    Database database(connectionString);

    if (!database.isConnected())
    {
        std::cerr << "Connexion PostgreSQL impossible.\n";
        return 1;
    }

    // =====================================================
    // GET /api/accounts : consulter tous les comptes
    // =====================================================
    drogon::app().registerHandler(
        "/api/accounts",
        [&database](
            const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            PGresult* result = database.execute(
                "SELECT id, owner, balance "
                "FROM accounts ORDER BY id"
            );

            if (result == nullptr)
            {
                Json::Value error;
                error["error"] = "Erreur lors de la lecture des comptes";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
                return;
            }

            Json::Value accounts(Json::arrayValue);

            for (int i = 0; i < PQntuples(result); ++i)
            {
                Json::Value account;
                account["id"] = std::stoi(PQgetvalue(result, i, 0));
                account["owner"] = PQgetvalue(result, i, 1);
                account["balance_millimes"] =
                    static_cast<Json::Int64>(
                        std::stoll(PQgetvalue(result, i, 2))
                    );

                accounts.append(account);
            }

            PQclear(result);

            callback(
                drogon::HttpResponse::newHttpJsonResponse(accounts)
            );
        },
        {drogon::Get}
    );

    // =====================================================
    // POST /api/accounts : creer un compte
    // =====================================================
    drogon::app().registerHandler(
        "/api/accounts",
        [&database](
            const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json ||
                !json->isMember("id") ||
                !(*json)["id"].isInt() ||
                !json->isMember("owner") ||
                !(*json)["owner"].isString())
            {
                Json::Value error;
                error["error"] =
                    "Les champs id et owner sont obligatoires";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            int id = (*json)["id"].asInt();
            std::string owner = (*json)["owner"].asString();

            if (id <= 0 || owner.empty() || owner.size() > 100)
            {
                Json::Value error;
                error["error"] =
                    "ID invalide ou nom du titulaire incorrect";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            PGresult* result = database.executeParams(
                "INSERT INTO accounts (id, owner, balance) "
                "VALUES ($1::integer, $2, 0) "
                "RETURNING id, owner, balance",
                {std::to_string(id), owner}
            );

            if (result == nullptr)
            {
                Json::Value error;
                error["error"] =
                    "Creation impossible : ID deja utilise ou erreur SQL";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k409Conflict);
                callback(response);
                return;
            }

            Json::Value account;
            account["id"] = std::stoi(PQgetvalue(result, 0, 0));
            account["owner"] = PQgetvalue(result, 0, 1);
            account["balance_millimes"] =
                static_cast<Json::Int64>(
                    std::stoll(PQgetvalue(result, 0, 2))
                );

            PQclear(result);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(account);
            response->setStatusCode(drogon::k201Created);
            callback(response);
        },
        {drogon::Post}
    );

    // =====================================================
    // POST /api/deposit : depot avec historique atomique
    // =====================================================
    drogon::app().registerHandler(
        "/api/deposit",
        [&database](
            const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json ||
                !json->isMember("account_id") ||
                !(*json)["account_id"].isInt() ||
                !json->isMember("amount_millimes") ||
                !(*json)["amount_millimes"].isInt64())
            {
                Json::Value error;
                error["error"] =
                    "account_id et amount_millimes sont obligatoires";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            int accountId = (*json)["account_id"].asInt();
            Json::Int64 amount =
                (*json)["amount_millimes"].asInt64();

            if (accountId <= 0 || amount <= 0)
            {
                Json::Value error;
                error["error"] =
                    "Le compte et le montant doivent etre positifs";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            long long newBalance = 0;

            std::string status = database.deposit(
                accountId,
                static_cast<long long>(amount),
                newBalance
            );

            if (status != "OK")
            {
                Json::Value error;

                if (status == "NOT_FOUND")
                    error["error"] = "Compte introuvable";
                else if (status == "INVALID")
                    error["error"] = "Identifiant ou montant invalide";
                else
                    error["error"] = "Erreur lors du depot";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    status == "NOT_FOUND"
                        ? drogon::k404NotFound
                        : status == "INVALID"
                            ? drogon::k400BadRequest
                            : drogon::k500InternalServerError
                );

                callback(response);
                return;
            }

            Json::Value result;
            result["message"] = "Depot effectue avec succes";
            result["account_id"] = accountId;
            result["amount_millimes"] = amount;
            result["balance_millimes"] =
                static_cast<Json::Int64>(newBalance);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(result);
            response->setStatusCode(drogon::k200OK);
            callback(response);
        },
        {drogon::Post}
    );

    // =====================================================
    // POST /api/withdraw : retrait avec historique atomique
    // =====================================================
    drogon::app().registerHandler(
        "/api/withdraw",
        [&database](
            const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json ||
                !json->isMember("account_id") ||
                !(*json)["account_id"].isInt() ||
                !json->isMember("amount_millimes") ||
                !(*json)["amount_millimes"].isInt64())
            {
                Json::Value error;
                error["error"] =
                    "account_id et amount_millimes sont obligatoires";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            int accountId = (*json)["account_id"].asInt();
            Json::Int64 amount =
                (*json)["amount_millimes"].asInt64();

            if (accountId <= 0 || amount <= 0)
            {
                Json::Value error;
                error["error"] =
                    "Le compte et le montant doivent etre positifs";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            // Une seule instruction SQL :
            // 1. Debiter le compte si le solde est suffisant.
            // 2. Enregistrer le retrait uniquement si le debit reussit.
            // PostgreSQL execute cette instruction de facon atomique.
            PGresult* result = database.executeParams(
                "WITH updated AS ("
                "  UPDATE accounts "
                "  SET balance = balance - $1::bigint "
                "  WHERE id = $2::integer "
                "    AND balance >= $1::bigint "
                "  RETURNING id, owner, balance"
                "), logged AS ("
                "  INSERT INTO transactions "
                "    (from_account_id, to_account_id, amount, type, status) "
                "  SELECT id, NULL, $1::bigint, 'RETRAIT', 'REUSSI' "
                "  FROM updated "
                "  RETURNING id"
                ") "
                "SELECT updated.id, updated.owner, updated.balance "
                "FROM updated "
                "JOIN logged ON TRUE",
                {std::to_string(amount), std::to_string(accountId)}
            );

            if (result == nullptr)
            {
                Json::Value error;
                error["error"] = "Erreur lors du retrait";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
                return;
            }

            if (PQntuples(result) == 0)
            {
                PQclear(result);

                // Aucun debit n'a ete effectue.
                // On verifie si le compte existe.
                PGresult* check = database.executeParams(
                    "SELECT id FROM accounts WHERE id = $1::integer",
                    {std::to_string(accountId)}
                );

                if (check == nullptr)
                {
                    Json::Value error;
                    error["error"] =
                        "Impossible de verifier le compte";

                    auto response =
                        drogon::HttpResponse::newHttpJsonResponse(error);
                    response->setStatusCode(
                        drogon::k500InternalServerError
                    );
                    callback(response);
                    return;
                }

                bool accountExists = PQntuples(check) > 0;
                PQclear(check);

                Json::Value error;
                error["error"] = accountExists
                    ? "Solde insuffisant"
                    : "Compte introuvable";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    accountExists
                        ? drogon::k409Conflict
                        : drogon::k404NotFound
                );

                callback(response);
                return;
            }

            Json::Value withdrawal;
            withdrawal["message"] = "Retrait effectue avec succes";
            withdrawal["account_id"] =
                std::stoi(PQgetvalue(result, 0, 0));
            withdrawal["owner"] = PQgetvalue(result, 0, 1);
            withdrawal["amount_millimes"] = amount;
            withdrawal["balance_millimes"] =
                static_cast<Json::Int64>(
                    std::stoll(PQgetvalue(result, 0, 2))
                );

            PQclear(result);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(withdrawal);
            response->setStatusCode(drogon::k200OK);
            callback(response);
        },
        {drogon::Post}
    );

    // =====================================================
    // POST /api/transfer : virement avec transaction SQL
    // =====================================================
    drogon::app().registerHandler(
        "/api/transfer",
        [&database](
            const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json ||
                !json->isMember("from_account_id") ||
                !(*json)["from_account_id"].isInt() ||
                !json->isMember("to_account_id") ||
                !(*json)["to_account_id"].isInt() ||
                !json->isMember("amount_millimes") ||
                !(*json)["amount_millimes"].isInt64())
            {
                Json::Value error;
                error["error"] =
                    "from_account_id, to_account_id et "
                    "amount_millimes sont obligatoires";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            int fromId = (*json)["from_account_id"].asInt();
            int toId = (*json)["to_account_id"].asInt();
            Json::Int64 amount =
                (*json)["amount_millimes"].asInt64();

            if (fromId <= 0 || toId <= 0 || amount <= 0)
            {
                Json::Value error;
                error["error"] =
                    "Les identifiants et le montant doivent etre positifs";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            if (fromId == toId)
            {
                Json::Value error;
                error["error"] =
                    "Les comptes source et destinataire doivent etre differents";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            long long fromBalance = 0;
            long long toBalance = 0;

            std::string status = database.transfer(
                fromId,
                toId,
                static_cast<long long>(amount),
                fromBalance,
                toBalance
            );

            if (status != "OK")
            {
                Json::Value error;

                if (status == "NOT_FOUND")
                    error["error"] =
                        "Un ou plusieurs comptes sont introuvables";
                else if (status == "INSUFFICIENT")
                    error["error"] = "Solde insuffisant";
                else if (status == "INVALID")
                    error["error"] = "Identifiants ou montant invalides";
                else if (status == "SAME_ACCOUNT")
                    error["error"] =
                        "Les deux comptes doivent etre differents";
                else
                    error["error"] = "Erreur lors du virement";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    status == "NOT_FOUND"
                        ? drogon::k404NotFound
                        : status == "INSUFFICIENT"
                            ? drogon::k409Conflict
                            : status == "INVALID" ||
                              status == "SAME_ACCOUNT"
                                ? drogon::k400BadRequest
                                : drogon::k500InternalServerError
                );

                callback(response);
                return;
            }

            Json::Value result;
            result["message"] = "Virement effectue avec succes";
            result["from_account_id"] = fromId;
            result["to_account_id"] = toId;
            result["amount_millimes"] = amount;
            result["from_balance_millimes"] =
                static_cast<Json::Int64>(fromBalance);
            result["to_balance_millimes"] =
                static_cast<Json::Int64>(toBalance);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(result);
            response->setStatusCode(drogon::k200OK);
            callback(response);
        },
        {drogon::Post}
    );

    // =====================================================
    // GET /api/transactions : consulter l'historique
    // =====================================================
    drogon::app().registerHandler(
        "/api/transactions",
        [&database](
            const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            PGresult* result = database.execute(
                "SELECT id, from_account_id, to_account_id, "
                "amount, type, status, created_at "
                "FROM transactions ORDER BY id DESC"
            );

            if (result == nullptr)
            {
                Json::Value error;
                error["error"] =
                    "Erreur lors de la lecture de l'historique";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
                return;
            }

            Json::Value transactions(Json::arrayValue);

            for (int i = 0; i < PQntuples(result); ++i)
            {
                Json::Value transaction;

                transaction["id"] =
                    static_cast<Json::Int64>(
                        std::stoll(PQgetvalue(result, i, 0))
                    );

                if (PQgetisnull(result, i, 1))
                    transaction["from_account_id"] = Json::Value();
                else
                    transaction["from_account_id"] =
                        std::stoi(PQgetvalue(result, i, 1));

                if (PQgetisnull(result, i, 2))
                    transaction["to_account_id"] = Json::Value();
                else
                    transaction["to_account_id"] =
                        std::stoi(PQgetvalue(result, i, 2));

                transaction["amount_millimes"] =
                    static_cast<Json::Int64>(
                        std::stoll(PQgetvalue(result, i, 3))
                    );

                transaction["type"] = PQgetvalue(result, i, 4);
                transaction["status"] = PQgetvalue(result, i, 5);
                transaction["created_at"] = PQgetvalue(result, i, 6);

                transactions.append(transaction);
            }

            PQclear(result);

            callback(
                drogon::HttpResponse::newHttpJsonResponse(transactions)
            );
        },
        {drogon::Get}
    );

    // =====================================================
    // Demarrer le serveur
    // =====================================================
    drogon::app().addListener("127.0.0.1", 8080);

    std::cout << "MIN-Bank API demarre sur http://127.0.0.1:8080\n";
    std::cout << "GET  /api/accounts\n";
    std::cout << "POST /api/accounts\n";
    std::cout << "POST /api/deposit\n";
    std::cout << "POST /api/withdraw\n";
    std::cout << "POST /api/transfer\n";
    std::cout << "GET  /api/transactions\n";

    drogon::app().run();

    return 0;
}