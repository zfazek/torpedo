#pragma once

#include "player.hpp"

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
                auto player = std::make_shared<Player>();
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
            // printf("%s\n", data);
            if (strlen(data) == 2 * Player::SIZE * Player::SIZE - 1) {
                setTable(data);
                const std::string result = validateTable();
                _players[player_id]->connection->send(result.c_str());
            }
        }

        void onDisconnect(seasocks::WebSocket* connection) override {
            if (_players[0]->connection == connection) {
                if (_players[1]) {
                    _players[0] = std::move(_players[1]);
                    _player_idx = 1;
                } else {
                    _player_idx = 0;
                }
                std::cout << "Disconnected: " << connection->getRequestUri()
                    << " : " << seasocks::formatAddress(connection->getRemoteAddress()) << "\n";
            } else if (_players[1]->connection == connection) {
                _player_idx = 1;
                std::cout << "Disconnected: " << connection->getRequestUri()
                    << " : " << seasocks::formatAddress(connection->getRemoteAddress()) << "\n";
            }
        }

    private:
        const std::string OK = "OK";
        const std::vector<int> offsets = {-Player::SIZE, -1, 1, Player::SIZE};
        const std::vector<int> refShips = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4};

        std::vector<int> _table;
        int _player_idx;
        std::shared_ptr<Player> _players[2];
        seasocks::Server* _server;
        int _shipLength;
        int _shipIdx;

        void startServer() {
            auto logger = std::make_shared<seasocks::PrintfLogger>(seasocks::Logger::Level::Debug);
            _server = new seasocks::Server(logger);
            _server->addWebSocketHandler("/trpd", std::make_shared<Game>(*this));
            _server->serve("", 8080);
        }

        void init() {
            _player_idx = 0;
            _table.clear();
        }

        void setTable(const char* data) {
            _table.clear();
            for (unsigned i = 0; i < strlen(data); ++i) {
                if (data[i] == '1') {
                    _table.push_back(1);
                } else if (data[i] == '0') {
                    _table.push_back(0);
                }
            }
        }

        bool isValidPosition(const int idx, const int offset) {
            const int y1 = idx / Player::SIZE;
            const int x1 = idx - Player::SIZE * y1;
            const int idx2 = idx + offset;
            const int y2 = idx2 / Player::SIZE;
            const int x2 = idx2 - Player::SIZE * y2;
            return idx2 >= 0 &&
                idx2 < Player::SIZE * Player::SIZE &&
                x2 >= 0 &&
                x2 < Player::SIZE &&
                y2 >= 0 &&
                y2 < Player::SIZE &&
                std::abs(x2 - x1) < 2 &&
                std::abs(y2 - y1) < 2;
        }

        void scanTable(int idx) {
            if (_table[idx] != 1) {
                return;
            }
            ++_shipLength;
            _table[idx] = _shipIdx;
            for (const int offset : offsets) {
                if (isValidPosition(idx, offset)) {
                    scanTable(idx + offset);
                }
            }
        }

        std::string parseTable() {
            std::vector<int> ships;
            _shipIdx = 2;
            for (unsigned i = 0; i < _table.size(); ++i) {
                if (_table[i] == 1) {
                    _shipLength = 0;
                    scanTable(i);
                    // std::cout << i << " " << _shipIdx << " " << _shipLength << std::endl;
                    ships.push_back(_shipLength);
                    ++_shipIdx;
                }
            }
            std::sort(ships.begin(), ships.end());
            if (ships != refShips) {
                std::map<int, int> counters;
                std::map<int, int> refCounters;
                for (const int s : ships) {
                    counters[s]++;
                }
                for (const int s : refShips) {
                    refCounters[s]++;
                }
                std::set<std::string> errors;
                std::string error;
                for (const int s : ships) {
                    if (counters[s] != refCounters[s]) {
                        error = std::to_string(s) + "-es hajobol " + std::to_string(counters[s]) + " van " + std::to_string(refCounters[s]) + " helyet. ";
                        errors.insert(error);
                    }
                }
                for (const int s : refShips) {
                    if (counters[s] == 0) {
                        error = "Nincs " + std::to_string(s) + "-es hajo. ";
                        errors.insert(error);
                    }
                }
                std::string result;
                for (const std::string& error : errors) {
                    result += error;
                }
                return result;
            }
            return OK;
        }

        std::string validateTable() {
            std::string result;
            result = parseTable();
            if (result != OK) {
                return result;
            }
            return OK;
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

