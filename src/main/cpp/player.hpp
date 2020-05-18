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
        std::vector<int> ownTable;
        std::vector<int> otherTable;
        bool hasValidTable;

        void init() {
            hasValidTable = false;
            ownTable.clear();
            otherTable.clear();
            for (unsigned i = 0; i < SIZE * SIZE; ++i) {
                otherTable.push_back(0);
            }
        }

    private:
};
