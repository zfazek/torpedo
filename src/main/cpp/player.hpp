#pragma once

#include "seasocks/WebSocket.h"

#include <vector>

class Player {
    public:
        Player() {
            init();
        }

        static constexpr int SIZE = 10;
        seasocks::WebSocket* connection;
        std::vector<int> table;
        bool hasValidTable;

        void init() {
            hasValidTable = false;
            table.clear();
        }

    private:
};
