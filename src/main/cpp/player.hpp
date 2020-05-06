#pragma once

#include "seasocks/WebSocket.h"

class Player {
    public:
        static constexpr int SIZE = 10;
        seasocks::WebSocket* connection;
        int table[SIZE * SIZE];
    private:
};
