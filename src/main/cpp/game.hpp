#pragma once

#include "player.hpp"
#include "state.hpp"

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
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
            std::cout << "Disconnected: " << connection->getRequestUri()
                << " : " << seasocks::formatAddress(connection->getRemoteAddress()) << "\n";
            const int player_id = getPlayerId(connection);
            const int otherPlayerId = 1 - player_id;
            if (player_id < 0) {
                return;
            }
            if (_players[otherPlayerId] && _players[otherPlayerId]->connection) {
                _players[otherPlayerId]->connection->send("Other player exited. Quitting...");
                _players[otherPlayerId]->connection->close();
            }
            _players[player_id].reset();
            _player_idx = 0;
            _state = State::INIT;

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
            if (_state == State::GAME && strlen(data) < 3) {
                const int idx = std::atoi(data);
                if (idx >= 0 && idx < Player::SIZE * Player::SIZE) {
                    if (_players[1 - _player_to_move]->ownTable[idx] == 1) {
                        _players[1 - _player_to_move]->connection->send(data);
                        _players[_player_to_move]->otherTable[idx] = 1;
                        std::string msg = "";
                        if (isSunk(idx)) {
                            std::set<int> indicesOfSameShip = {idx};
                            setIndicesOfSameShip(_players[_player_to_move]->otherTable, idx, indicesOfSameShip);
                            for (const int i : indicesOfSameShip) {
                                _players[_player_to_move]->otherTable[i] = 5;
                            }
                        }
                        _players[_player_to_move]->connection->send(tableToString(_players[_player_to_move]->otherTable));
                        _players[_player_to_move]->connection->send("Shoot!");
                    } else {
                        _players[_player_to_move]->connection->send("You missed!");
                        _players[_player_to_move]->otherTable[idx] = 2;
                        _players[_player_to_move]->connection->send(tableToString(_players[_player_to_move]->otherTable));
                        const std::string str = "m" + std::string(data);
                        _players[1 - _player_to_move]->connection->send(str);
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                        _players[1 - _player_to_move]->connection->send(tableToString(_players[1 - _player_to_move]->otherTable));
                        _players[1 - _player_to_move]->connection->send("Shoot!");
                        _player_to_move = 1 - _player_to_move;
                    }
                }
                const int player_won = getPlayerWon();
                if (player_won == -1) {
                    return;
                }
                _state = State::END;
                _players[player_won]->connection->send("You won!");
                _players[1 - player_won]->connection->send("You lost!");
                for (unsigned i = 0; i < 2; ++i) {
                    _players[i]->connection->send(tableToString(_players[1 - i]->ownTable));
                    _players[i]->connection->close();
                }
                return;
            }
            // printf("%s\n", data);
            if (_state == State::INIT && isTableSent(data)) {
                if (_players[player_id] && _players[player_id]->hasValidTable) {
                    return;
                }
                Table table = getTable(data);
                const std::string result = validateTable(table);
                _players[player_id]->connection->send(result.c_str());
                if (result == "OK") {
                    _players[player_id]->ownTable = table;
                    _players[player_id]->hasValidTable = true;
                    const int otherPlayerId = 1 - player_id;
                    if (_players[otherPlayerId] && _players[otherPlayerId]->hasValidTable) {
                        _state = State::GAME;
                        _players[_player_to_move]->connection->send(tableToString(_players[_player_to_move]->otherTable));
                        _players[_player_to_move]->connection->send("Shoot!");
                    }
                }
                return;
            }
            if (_state == State::INIT && strcmp("random", data) == 0) {
                std::vector<int> randomTable = createRandomTable();
                _players[player_id]->connection->send(tableToString(randomTable));
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
        int _player_to_move;

        void startServer() {
            auto logger = std::make_shared<seasocks::PrintfLogger>(seasocks::Logger::Level::Debug);
            _server = new seasocks::Server(logger);
            _server->addWebSocketHandler("/trpd", std::make_shared<Game>(*this));
            _server->serve("", 8080);
        }

        void init() {
            srand(time(0));
            _player_idx = 0;
            _player_to_move = 0;
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

        std::string parseTable(Table& table, const bool checkOnlyOverlapping = false) const {
            std::vector<int> ships;
            int shipIdx = 2;
            for (unsigned i = 0; i < table.size(); ++i) {
                if (table[i] == 1) {
                    int shipLength = 0;
                    scanTable(table, i, shipIdx, shipLength);
                    ships.push_back(shipLength);
                    ++shipIdx;
                }
            }
            std::vector<int> offsets = offsets2;
            if (checkOnlyOverlapping) {
                offsets.insert(offsets.end(), offsets1.begin(), offsets1.end());
            }
            for (unsigned i = 0; i < table.size(); ++i) {
                if (table[i] != 0) {
                    for (const int offset : offsets) {
                        if (isValidPosition(i, offset)) {
                            if (table[i + offset] != 0 && table[i] != table[i + offset]) {
                                return "Overlapping ships";
                            }
                        }
                    }
                }
            }
            if (checkOnlyOverlapping) {
                return OK;
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
                        std::string plural = "";
                        if (counters[s] > 1) {
                            plural = "s";
                        }
                        error = "You have " + std::to_string(counters[s]) + " ship" + plural + " instead of "
                            + std::to_string(refCounters[s]) + " in size " + std::to_string(s) + ". ";
                        errors.insert(error);
                    }
                }
                for (const int s : refShips) {
                    if (counters[s] == 0) {
                        error = "No ship in size " + std::to_string(s) + ". ";
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

        void setIndicesOfSameShip(const Table& table, const int idx, std::set<int>& indicesOfSameShip) const {
            for (const int offset : offsets1) {
                if (isValidPosition(idx, offset) && table[idx + offset] == 1 &&
                        indicesOfSameShip.find(idx + offset) == indicesOfSameShip.end()) {
                    indicesOfSameShip.insert(idx + offset);
                    setIndicesOfSameShip(table, idx + offset, indicesOfSameShip);
                }
            }
        }

        bool isSunk(const int idx) const {
            const int other_player = 1 - _player_to_move;
            std::set<int> indicesOfSameShip1 = {idx};
            setIndicesOfSameShip(_players[other_player]->ownTable, idx, indicesOfSameShip1);
            std::set<int> indicesOfSameShip2 = {idx};
            setIndicesOfSameShip(_players[_player_to_move]->otherTable, idx, indicesOfSameShip2);
            return indicesOfSameShip1 == indicesOfSameShip2;
        }

        int getPlayerWon() const {
            for (unsigned i = 0; i < 2; ++i) {
                int counter = 0;
                for (const int e : _players[i]->otherTable) {
                    if (e == 1 || e == 5) {
                        ++counter;
                    }
                    if (counter == 20) {
                        return i;
                    }
                }
            }
            return -1;
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

        std::string tableToString(const Table& table) const {
            std::string ret;
            for (const int e : table) {
                ret += std::to_string(e);
            }
            return ret;
        }

        Table createRandomTable() const {
            Table table;
            for (int i = 0; i < Player::SIZE * Player::SIZE; ++i) {
                table.push_back(0);
            }
            const std::vector<int> refShips = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};
            int limit = 99;
            int shipIdx = 1;
            for (const int shipLength : refShips) {
                ++shipIdx;
                bool fail;
                do {
                    fail = false;
                    const int idx = rand() % (Player::SIZE * Player::SIZE);
                    if (table[idx] != 0) {
                        fail = true;
                        continue;
                    }
                    --limit;
                    std::vector<int> offsets = offsets1;
                    std::random_shuffle(offsets.begin(), offsets.end());
                    for (const int offset : offsets) {
                        fail = false;
                        Table lastTable = table;
                        for (int i = 0; i < shipLength; ++i) {
                            if (table[idx + i * offset] == 0 && isValidPosition(idx + (i - 1) * offset, offset)) {
                                table[idx + i * offset] = shipIdx;
                                Table tempTable = table;
                                const std::string result = parseTable(tempTable, true);
                                if (result != OK) {
                                    table = lastTable;
                                    fail = true;
                                    break;
                                }
                            } else {
                                table = lastTable;
                                fail = true;
                                break;
                            }
                        }
                        if (!fail) {
                            break;
                        }
                    }
                } while(fail && limit > 0);
            }
            for (unsigned i = 0; i < Player::SIZE * Player::SIZE; ++i) {
                if (table[i] > 0) {
                    table[i] = 1;
                }
            }
            return table;
        }
};

