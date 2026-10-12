
#pragma once

#include <string>

struct Transaction {
    int fromId;
    int toId;
    long long amount;
    std::string type;
    std::string status;
};