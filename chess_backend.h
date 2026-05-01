#ifndef CHESS_BACKEND_H
#define CHESS_BACKEND_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <ctime>
#include <regex>
#include <utility>
#include "chess_types.h"
#include "chess_identifiers.h"

class ChessBoard
{
private:
    std::map<char, std::string> unicodePieces;

    std::vector<std::vector<std::string>> board;
    bool whiteTurn = true;
    int mateStatus = false;
    std::vector<std::pair<std::string, std::string>> moveList = {};
    std::vector<std::string> uciMoveList = {};
    std::vector<std::string> prevFENList = {};
    std::vector<std::unordered_map<std::string, std::string>> pieceMapList;
    bool minSelectedMove0 = false;
    bool startEnPassant = false;

    int halfMoves = 0;
    int completeMoves = 1;
    int startingCompleteMoves = 1;

    CastlingRights whiteCastling;
    CastlingRights blackCastling;

    void updateMoveList(std::string move, bool whiteTurn);
    std::string getBasicMoveNotation(int fromRow, int fromCol, int toRow, int toCol, std::string promotion, bool enPassant);
    void displayBoard();
    bool isInCheck(bool whiteTurn);
    bool hasAnyLegalMove();
    int isMate(bool whiteTurn);
    void swapTurn();
    void setupFromFEN(const std::string &fen);
    std::string getDisambiguator(int fromRow, int fromCol, int toRow, int toCol, std::string piece);
    void updateUciMoveList(std::string move);
    std::string getCurrentFen();
    std::vector<std::vector<std::string>> getBoardFromFEN(std::string fen);
    std::string getStartingFen();

public:
    // Constructor
    ChessBoard();

    // Public methods
    void StartGame(std::string mode);
    std::vector<std::pair<int, int>> GetAllValidMoves(int fromRow, int fromCol);
    bool MovePiece(std::string from, std::string to, std::string promotion);
    int GetMateStatus();
    std::vector<std::vector<std::string>> getBoard();
    bool GetWhiteTurn();
    bool IsValidMove(std::string from, std::string to);
    std::vector<std::pair<std::string, std::string>> GetMoveList();
    void StartGameFromFen(std::string fen);
    int GetStartingMoveNumber();
    bool GetMinSelectedMove0();
    std::vector<std::vector<std::string>> GetBoardAtIndex(int index);
    std::string GetUciMoveAtIndex(int index);
    bool UndoMove();
    void ExportMoves();
    bool IsThreefoldRepetition();
    void CallDraw();
    void printBoardContents(const std::vector<std::vector<std::string>> &board);
    std::unordered_map<std::string, std::string> GetPieceMapAtIndex(int index);
};

#endif