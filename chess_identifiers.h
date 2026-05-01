#ifndef CHESS_IDENTIFIERS_H
#define CHESS_IDENTIFIERS_H

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cctype>

std::unordered_map<std::string, std::string> fenToPieceMap(const std::string &fen);

std::unordered_map<std::string, std::string> updateFenToPieceMap(
    const std::unordered_map<std::string, std::string> &pieceMap,
    const std::string &uciMove);

std::unordered_map<std::string, std::string> fillSpawnedPieces(
    const std::unordered_map<std::string, std::string> &currentMap,
    const std::unordered_map<std::string, std::string> &newMap);

#endif