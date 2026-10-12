
#pragma once

#include <string>

class Account {
private:
    int id;
    std::string owner;
    long long balance; // Solde en millimes

public:
    Account(int id, const std::string& owner,
            long long initialBalance = 0);

    int getId() const;
    std::string getOwner() const;
    long long getBalance() const;

    bool deposit(long long amount);
    bool withdraw(long long amount);

    void display() const;
    bool transferTo(Account& destination, long long amount);
};