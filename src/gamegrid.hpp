/**
 *  GameOfLife - An implementation of game of life using xengine
 *  Copyright (C) 2021  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef GAMEOFLIFE_GAMEGRID_HPP
#define GAMEOFLIFE_GAMEGRID_HPP

#include <map>
#include <vector>
#include <set>

#include "math/vector2.hpp"

/**
 * @tparam T The type to use for cell position components
 */
template<typename T>
struct GameGrid {
    typedef xengine::Vector2<T> Position;

    std::unordered_map<T, std::set<T>> cells;

    int minSurvive = 2;
    int maxSurvive = 3;

    int minRevive = 3;
    int maxRevive = 3;

    void setCell(Position pos, bool alive) {
        if (alive)
            cells[pos.x].insert(pos.y);
        else
            cells[pos.x].erase(pos.y);
    }

    bool getCell(Position pos) {
        return cells[pos.x].find(pos.y) != cells[pos.x].end();
    }

    std::vector<Position> getNeighbourPositions(Position pos) {
        return {
                {pos.x - 1, pos.y},
                {pos.x + 1, pos.y},
                {pos.x,     pos.y - 1},
                {pos.x,     pos.y + 1},
                {pos.x - 1, pos.y - 1},
                {pos.x + 1, pos.y + 1},
                {pos.x - 1, pos.y + 1},
                {pos.x + 1, pos.y - 1}
        };
    }

    int getNeighbours(Position pos) {
        int ret = 0;
        auto positions = getNeighbourPositions(pos);
        for (auto &p: positions)
            if (getCell(p))
                ret++;
        return ret;
    }

    GameGrid<T> stepTime() {
        std::vector<Position> aliveCells;
        for (auto &x: cells) {
            for (auto &y: x.second) {
                Position pos(x.first, y);
                auto n = getNeighbours(pos);
                if (n >= minSurvive && n <= maxSurvive) {
                    aliveCells.emplace_back(pos);
                }

                //Check dead neighbour cells for resurrection, checks some dead cells twice
                auto neighbours = getNeighbourPositions(pos);
                for (auto &p: neighbours) {
                    if (!getCell(p)) {
                        auto nc = getNeighbours(p);
                        if (nc >= minRevive && nc <= maxRevive) {
                            // Check if cell was already added to aliveCells by a previous iteration
                            bool exists = false;
                            for (auto &cell: aliveCells) {
                                if (cell == p) {
                                    exists = true;
                                    break;
                                }
                            }
                            if (!exists)
                                aliveCells.emplace_back(p);
                        }
                    }
                }
            }
        }

        GameGrid<T> ret;
        for (auto &pos: aliveCells) {
            ret.setCell(pos, true);
        }
        return ret;
    }
};

#endif //GAMEOFLIFE_GAMEGRID_HPP
