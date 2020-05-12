#pragma once

#include "player.hpp"
#include "state.hpp"

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
        typedef std::vector<int> Table;

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
            if (strlen(data) > 0 && data[0] == 'v') {
                Table table = getTable(data);
                const std::string result = "v" + validateTable(table);
                _players[player_id]->connection->send(result.c_str());
                return;
            }
            // printf("%s\n", data);
            if (isTableSent(data)) {
                if (_players[player_id] && _players[player_id]->hasValidTable) {
                    return;
                }
                Table table = getTable(data);
                const std::string result = validateTable(table);
                _players[player_id]->connection->send(result.c_str());
                if (result == "OK") {
                    _players[player_id]->table = table;
                    _players[player_id]->hasValidTable = true;
                    const int otherPlayerId = 1 - player_id;
                    if (_players[otherPlayerId] && _players[otherPlayerId]->hasValidTable) {
                        _state = State::GAME;
                    }
                }
                return;
            }
        }

    private:
        const std::string OK = "OK";
        const std::vector<int> offsets1 = {-Player::SIZE, -1, 1, Player::SIZE};
        const std::vector<int> offsets2 = {-Player::SIZE - 1, Player::SIZE + 1,
            Player::SIZE - 1, Player::SIZE + 1};
        const std::vector<int> refShips = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4};

        int _player_idx;
        std::shared_ptr<Player> _players[2];
        seasocks::Server* _server;
        State _state;

        void startServer() {
            auto logger = std::make_shared<seasocks::PrintfLogger>(seasocks::Logger::Level::Debug);
            _server = new seasocks::Server(logger);
            _server->addWebSocketHandler("/trpd", std::make_shared<Game>(*this));
            _server->serve("", 8080);
        }

        void init() {
            _player_idx = 0;
            _state = State::INIT;
            for (unsigned i = 0; i < 2; ++i) {
                if (_players[i]) {
                    _players[i]->init();
                }
            }
        }

        bool isTableSent(const char* data) const {
            return strlen(data) == 2 * Player::SIZE * Player::SIZE - 1;
        }

        Table getTable(const char* data) const {
            Table table;
            for (unsigned i = 0; i < strlen(data); ++i) {
                if (data[i] == '1') {
                    table.push_back(1);
                } else if (data[i] == '0') {
                    table.push_back(0);
                }
            }
            return table;
        }

        bool isValidPosition(const int idx, const int offset) const {
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

        void scanTable(Table& table, int idx, const int shipIdx, int& shipLength) const {
            if (table[idx] != 1) {
                return;
            }
            ++shipLength;
            table[idx] = shipIdx;
            for (const int offset : offsets1) {
                if (isValidPosition(idx, offset)) {
                    scanTable(table, idx + offset, shipIdx, shipLength);
                }
            }
        }

        std::string parseTable(Table& table) const {
            std::vector<int> ships;
            int shipIdx = 2;
            for (unsigned i = 0; i < table.size(); ++i) {
                if (table[i] == 1) {
                    int shipLength = 0;
                    scanTable(table, i, shipIdx, shipLength);
                    // std::cout << i << " " << shipIdx << " " << shipLength << std::endl;
                    ships.push_back(shipLength);
                    ++shipIdx;
                }
            }
            for (unsigned i = 0; i < table.size(); ++i) {
                if (table[i] != 0) {
                    for (const int offset : offsets2) {
                        if (isValidPosition(i, offset)&&  table[i + offset] != 0 && table[i] !=
                                table[i+ offset]) {
                                return "Osszeero hajok";
                        }
                    }
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
                        error = std::to_string(s) + "-es hajobol " + std::to_string(counters[s]) +
                            " van " + std::to_string(refCounters[s]) + " helyet. ";
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

        std::string validateTable(Table& table) const {
            std::string result;
            result = parseTable(table);
            if (result != OK) {
                return result;
            }
            for (unsigned i = 0; i < table.size(); ++i) {
                if (table[i] > 0) {
                    table[i] = 1;
                }
            }
            return OK;
        }

        int getPlayerId(const seasocks::WebSocket* connection) const {
            if (_players[0] && _players[0]->connection == connection) {
                return 0;
            } else if (_players[1] && _players[1]->connection == connection) {
                return 1;
            } else {
                return -1;
            }
        }
};

