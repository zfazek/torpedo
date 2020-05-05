#pragma once

#include "player.hpp"

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"
#include "seasocks/util/Json.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

class Game : public seasocks::WebSocket::Handler {
	public:
		Game() {
			init();
			startServer();
		}

		void onConnect(seasocks::WebSocket* connection) override {
			if (_player_idx == 2) {
				connection->send("Two players has already joined. Quitting...");
				connection->close();
				return;
			} else {
				Player* player = new Player();
				player->connection = connection;
				_players[_player_idx++] = std::move(player);
				std::cout << "Connected: " << connection->getRequestUri()
					<< " : " << seasocks::formatAddress(connection->getRemoteAddress())
					<< "\nCredentials: " << *(connection->credentials()) << "\n";
			}
		}

		void onData(seasocks::WebSocket* connection, const char* data) override {
			if (0 == strcmp("die", data)) {
				_server->terminate();
				return;
			}
			if (0 == strcmp("close", data)) {
				std::cout << "Closing..\n";
				connection->close();
				std::cout << "Closed.\n";
				return;
			}
			const int player_id = getPlayerId(connection);
			if (player_id < 0) {
				return;
			}
			if (strlen(data) == 199) {
				const std::string result = validateTable(data);
				_players[player_id]->connection->send(result.c_str());
			}
		}

		void onDisconnect(seasocks::WebSocket* connection) override {
			if (_players[0]->connection == connection) {
				delete _players[0];
				_players[0] = std::move(_players[1]);
				_player_idx = 0;
				std::cout << "Disconnected: " << connection->getRequestUri()
					<< " : " << seasocks::formatAddress(connection->getRemoteAddress()) << "\n";
			} else if (_players[1]->connection == connection) {
				delete _players[1];
				_player_idx = 1;
				std::cout << "Disconnected: " << connection->getRequestUri()
					<< " : " << seasocks::formatAddress(connection->getRemoteAddress()) << "\n";
			}
		}

	private:
		int _player_idx;
		Player* _players[2];
		seasocks::Server* _server;

		void startServer() {
			auto logger = std::make_shared<seasocks::PrintfLogger>(seasocks::Logger::Level::Debug);
			_server = new seasocks::Server(logger);
			_server->addWebSocketHandler("/trpd", std::make_shared<Game>(*this));
			_server->serve("", 8080);
		}

		void init() {
			_player_idx = 0;
		}

		std::string validateTable(const char* data) const {
			int counter = 0;
			for (unsigned i = 0; i < strlen(data); ++i) {
				if (data[i] == '1') {
					++counter;
				}
			}
			if (counter < 20) {
				return "Tul keves hajo";
			} else if (counter > 20) {
				return "Tul sok hajo";
			}
			return "OK";
		}

		int getPlayerId(const seasocks::WebSocket* connection) const {
			if (_players[0]->connection == connection) {
				return 0;
			} else if (_players[1]->connection == connection) {
				return 1;
			} else {
				return -1;
			}
		}
};

