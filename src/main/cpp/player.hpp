#pragma once

#include "seasocks/WebSocket.h"

class Player {
	public:
		static constexpr size_t SIZE = 10;
		seasocks::WebSocket* connection;
		int table[SIZE * SIZE];
	private:
};
