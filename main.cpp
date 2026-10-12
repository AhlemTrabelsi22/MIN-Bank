
#include "Bank.h"
#include "Database.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

int main() {
    const char* password = std::getenv("MINBANK_DB_PASSWORD");

    if (password == nullptr) {
        std::cerr << "Mot de passe PostgreSQL non configure.\n";
        return 1;
    }

    std::string connectionString =
        "host=localhost port=5432 "
        "dbname=min_bank user=postgres password=";

    connectionString += password;

    Database database(connectionString);

    if (!database.isConnected()) {
        std::cerr << "Connexion PostgreSQL impossible.\n";
        return 1;
    }

    Bank bank(database);

    int choice = -1;


    do {
        std::cout << "\n===== MIN-BANK =====\n";
        std::cout << "1. Creer un compte\n";
        std::cout << "2. Afficher les comptes\n";
        std::cout << "3. Effectuer un depot\n";
        std::cout << "4. Effectuer un retrait\n";
        std::cout << "5. Effectuer un virement\n";
        std::cout << "6. Consulter l'historique\n";
        std::cout << "0. Quitter\n";
        std::cout << "Votre choix : ";

        if (!(std::cin >> choice)) {
            std::cout << "Entree invalide.\n";
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(), '\n'
            );
            continue;
        }

        int id;
        int fromId;
        int toId;
        long long amount;
        std::string owner;

        switch (choice) {
        case 1:
            std::cout << "Identifiant du compte : ";
            if (!(std::cin >> id)) {
                std::cout << "Identifiant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << "Nom du titulaire : ";
            std::getline(std::cin >> std::ws, owner);

            std::cout << "Solde initial en millimes : ";
            if (!(std::cin >> amount)) {
                std::cout << "Montant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << (bank.createAccount(id, owner, amount)
                ? "Compte cree avec succes.\n"
                : "Creation refusee.\n");
            break;

        case 2:
            bank.displayAccounts();
            break;

        case 3:
            std::cout << "Identifiant du compte : ";
            if (!(std::cin >> id)) {
                std::cout << "Identifiant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << "Montant du depot en millimes : ";
            if (!(std::cin >> amount)) {
                std::cout << "Montant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << (bank.deposit(id, amount)
                ? "Depot reussi.\n"
                : "Depot refuse.\n");
            break;

        case 4:
            std::cout << "Identifiant du compte : ";
            if (!(std::cin >> id)) {
                std::cout << "Identifiant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << "Montant du retrait en millimes : ";
            if (!(std::cin >> amount)) {
                std::cout << "Montant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << (bank.withdraw(id, amount)
                ? "Retrait reussi.\n"
                : "Retrait refuse.\n");
            break;

        case 5:
            std::cout << "Identifiant du compte source : ";
            if (!(std::cin >> fromId)) {
                std::cout << "Identifiant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << "Identifiant du compte destinataire : ";
            if (!(std::cin >> toId)) {
                std::cout << "Identifiant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << "Montant du virement en millimes : ";
            if (!(std::cin >> amount)) {
                std::cout << "Montant invalide.\n";
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(), '\n'
                );
                break;
            }

            std::cout << (bank.transfer(fromId, toId, amount)
                ? "Virement reussi.\n"
                : "Virement refuse.\n");
            break;

        case 6:
            bank.displayHistory();
            break;

        case 0:
            std::cout << "Fermeture de MIN-Bank.\n";
            break;

        default:
            std::cout << "Choix invalide. Reessayez.\n";
        }

    } while (choice != 0);

    return 0;
}