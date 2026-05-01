#ifndef CHESS_UTILS_H
#define CHESS_UTILS_H

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include "chess_types.h"

bool isSquareAttacked(const std::vector<std::vector<std::string>> &board,
                      int row, int col, bool byWhite);

bool isValidMove(const std::vector<std::vector<std::string>> &board, int fromRow, int fromCol, int toRow, int toCol, bool whiteTurn, CastlingRights castlingRights, std::vector<std::string> uciMoveList);

std::vector<std::pair<int, int>> getAllValidMoves(const std::vector<std::vector<std::string>> &board,
                                                  int fromRow, int fromCol, bool whiteTurn, CastlingRights castlingRights, std::vector<std::string> uciMoveList);

#endif