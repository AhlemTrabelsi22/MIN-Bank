
#include "Account.h"

#include <iomanip>
#include <iostream>
#include <utility>

Account::Account(int id, const std::string& owner,
                 long long initialBalance)
    : id(id),
      owner(owner),
      balance(initialBalance >= 0 ? initialBalance : 0) {
}

int Account::getId() const {
    return id;
}

std::string Account::getOwner() const {
    return owner;
}

long long Account::getBalance() const {
    return balance;
}

bool Account::deposit(long long amount) {
    if (amount <= 0) {
        return false;
    }

    balance += amount;
    return true;
}

bool Account::withdraw(long long amount) {
    if (amount <= 0 || amount > balance) {
        return false;
    }

    balance -= amount;
    return true;
}

void Account::display() const {
    std::cout << "Numero du compte : " << id << '\n';
    std::cout << "Titulaire        : " << owner << '\n';

    std::cout << "Solde            : "
              << balance / 1000 << '.'
              << std::setfill('0') << std::setw(3)
              << balance % 1000
              << " TND"
              << std::setfill(' ') << '\n';
}
bool Account::transferTo(Account& destination, long long amount) {
    if (&destination == this || amount <= 0 || amount > balance) {
        return false;
    }

    if (destination.balance > 9223372036854775807LL - amount) {
        return false;
    }

    balance -= amount;
    destination.balance += amount;

    return true;
}