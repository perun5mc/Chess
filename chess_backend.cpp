#include "chess_backend.h"

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
#include <fstream>
#include <filesystem>
#include "fen_generators.h"
#include "chess_utils.h"
#include "chess_types.h"
#include "chess_identifiers.h"

using namespace std;

void printPieceMapList(const std::vector<std::unordered_map<std::string, std::string>> &pieceMapList)
{
    for (size_t i = 0; i < pieceMapList.size(); ++i)
    {
        std::cout << "Map " << i << ":" << std::endl;

        for (const auto &pair : pieceMapList[i])
        {
            std::cout << "  " << pair.first << " : " << pair.second << std::endl;
        }

        if (i < pieceMapList.size() - 1)
        {
            std::cout << std::endl; // Add spacing between maps
        }
    }
}

ChessBoard::ChessBoard()
{
    board = vector<vector<string>>(8, vector<string>(8, " "));
}

bool ChessBoard::GetWhiteTurn()
{
    return whiteTurn;
}

void ChessBoard::displayBoard()
{
    cout << "\n    a   b   c   d   e   f   g   h\n";
    cout << "  ┌───┬───┬───┬───┬───┬───┬───┬───┐\n";

    for (int row = 0; row < 8; row++)
    {
        cout << 8 - row << " │";
        for (int col = 0; col < 8; col++)
        {
            string piece = board[row][col];
            char pieceChar = piece[0];
            if (pieceChar == ' ')
            {
                cout << "   │";
            }
            else
            {
                cout << " " << unicodePieces[pieceChar] << " │";
            }
        }
        cout << " " << 8 - row << "\n";

        if (row < 7)
        {
            cout << "  ├───┼───┼───┼───┼───┼───┼───┼───┤\n";
        }
    }

    cout << "  └───┴───┴───┴───┴───┴───┴───┴───┘\n";
    cout << "    a   b   c   d   e   f   g   h\n\n";
}

string ChessBoard::getStartingFen()
{
    if (prevFENList.size() < 1)
    {
        return "";
    }
    return prevFENList[0];
}

void ChessBoard::ExportMoves()
{
    time_t currentTime = time(nullptr);
    string fileName = to_string(currentTime);
    string folder = "logs";
    string path = folder + "/" + fileName + ".txt";

    if (!filesystem::exists(folder))
    {
        filesystem::create_directory(folder);
    }

    ofstream file;
    file.open(path);
    string startingFen = getStartingFen();
    bool isWhiteTurn = true;
    if (startingFen[startingFen.find(' ') + 1] == 'b')
        isWhiteTurn = false;
    string moves = "";
    int i = 1;
    for (const auto &[start, end] : GetMoveList())
    {

        if (i == 1 && !isWhiteTurn)
        {
            moves += to_string(i) + start + " " + end + "\n";
        }
        else
        {
            moves += to_string(i) + ". " + start + " " + end + "\n";
        }
        i += 1;
    }
    i = 1;
    file << "[FEN \"" + startingFen + "\"]" + "\n\n" + moves;
    file.close();
}

void ChessBoard::swapTurn()
{
    whiteTurn = !whiteTurn;
}

void ChessBoard::setupFromFEN(const string &fen)
{

    cout << fen << endl;
    for (int i = 0; i < 8; i++)
    {
        fill(board[i].begin(), board[i].end(), " ");
    }

    vector<string> fenParts;
    size_t start = 0, end = fen.find(' ');

    while (end != string::npos)
    {
        fenParts.push_back(fen.substr(start, end - start));
        start = end + 1;
        end = fen.find(' ', start);
    }
    fenParts.push_back(fen.substr(start));

    if (fenParts.size() < 1)
        return;

    string boardFEN = fenParts[0];
    int row = 0, col = 0;

    for (char c : boardFEN)
    {
        if (c == '/')
        {
            row++;
            col = 0;
        }
        else if (isdigit(c))
        {
            col += c - '0';
        }
        else if (isalpha(c))
        {
            if (row < 8 && col < 8)
            {
                string piece = string(1, c);
                board[row][col] = piece;

                if (piece == "K")
                {
                    ChessBoard::whiteCastling.kingStartCol = col;
                }
                else if (piece == "k")
                {
                    ChessBoard::blackCastling.kingStartCol = col;
                }
                else if (piece == "R")
                {
                    if (ChessBoard::whiteCastling.rookQStartCol == -1 || col < ChessBoard::whiteCastling.rookQStartCol)
                    {
                        ChessBoard::whiteCastling.rookQStartCol = col;
                    }

                    if (ChessBoard::whiteCastling.rookKStartCol == -1 || col > ChessBoard::whiteCastling.rookKStartCol)
                    {
                        ChessBoard::whiteCastling.rookKStartCol = col;
                    }
                }
                else if (piece == "r")
                {
                    if (ChessBoard::blackCastling.rookQStartCol == -1 || col < ChessBoard::blackCastling.rookQStartCol)
                    {
                        ChessBoard::blackCastling.rookQStartCol = col;
                    }

                    if (ChessBoard::blackCastling.rookKStartCol == -1 || col > ChessBoard::blackCastling.rookKStartCol)
                    {
                        ChessBoard::blackCastling.rookKStartCol = col;
                    }
                }
            }
            col++;
        }

        if (row >= 8)
            break;
    }

    if (fenParts.size() >= 2)
    {
        string turn = fenParts[1];
        if (turn == "b" || turn == "B")
        {
            if (prevFENList.empty())
            {
                moveList.push_back({"...", ""});
                minSelectedMove0 = true;
            }
            whiteTurn = false;
        }
        else
        {
            whiteTurn = true;
        }
    }

    if (fenParts.size() >= 3)
    {
        string castling = fenParts[2];

        for (char c : castling)
        {
            switch (c)
            {
            case 'K':
                ChessBoard::whiteCastling.canCastleKingside = true;
                break;
            case 'Q':
                ChessBoard::whiteCastling.canCastleQueenside = true;
                break;
            case 'k':
                ChessBoard::blackCastling.canCastleKingside = true;
                break;
            case 'q':
                ChessBoard::blackCastling.canCastleQueenside = true;
                break;
            case '-':
                break;
            }
        }
    }

    if (fenParts.size() >= 4 && prevFENList.empty())
    {
        string enPassant = fenParts[3];
        if (enPassant != "-" && enPassant.length() == 2)
        {
            char col = enPassant[0];
            char row = enPassant[1];

            string turn = fenParts[1];
            if (tolower(turn[0]) == 'w')
            {
                string newUciMove = string(1, col) + string(1, (row + 1)) + string(1, col) + string(1, (row - 1));
                updateUciMoveList(newUciMove);
                startEnPassant = true;
            }
            else if (tolower(turn[0]) == 'b')
            {
                string newUciMove = string(1, col) + string(1, (row - 1)) + string(1, col) + string(1, (row + 1));
                updateUciMoveList(newUciMove);
                startEnPassant = true;
            }
        }
    }

    if (fenParts.size() >= 5)
    {
        int halfMoveCount = stoi(fenParts[4]);
        if (halfMoveCount >= 0)
        {
            halfMoves = halfMoveCount;
        }
    }

    if (fenParts.size() >= 6)
    {
        int fullMoveCount = stoi(fenParts[5]);
        if (fullMoveCount >= 1)
        {
            completeMoves = fullMoveCount;
            if (prevFENList.empty())
            {
                startingCompleteMoves = fullMoveCount;
            }
        }
    }

    if (prevFENList.empty())
    {
        prevFENList.push_back(fen);
    }

    if (pieceMapList.empty())
    {
        pieceMapList.push_back(fenToPieceMap(fen));
    }
}

string ChessBoard::getDisambiguator(int fromRow, int fromCol, int toRow, int toCol, string piece)
{
    if (piece == "P" || piece == "p" || piece == " ")
        return "";

    char pieceType = toupper(piece[0]);
    vector<pair<int, int>> possibleOrigins;

    CastlingRights castlingRights = whiteTurn ? whiteCastling : blackCastling;

    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            if (board[r][c] != " " && toupper(board[r][c][0]) == pieceType)
            {
                if (r == fromRow && c == fromCol)
                    continue;

                if (isValidMove(board, r, c, toRow, toCol, whiteTurn, castlingRights, uciMoveList))
                {
                    possibleOrigins.push_back({r, c});
                }
            }
        }
    }

    if (possibleOrigins.empty())
        return "";

    bool sameFile = false;
    bool sameRank = false;

    for (const auto &origin : possibleOrigins)
    {
        if (origin.second == fromCol)
            sameFile = true;
        if (origin.first == fromRow)
            sameRank = true;
    }

    if (!sameFile)
    {
        return string(1, char('a' + fromCol));
    }
    else if (!sameRank)
    {
        return string(1, char('8' - fromRow));
    }
    else
    {
        string result = "";
        result += char('a' + fromCol);
        result += char('8' - fromRow);
        return result;
    }
}

string ChessBoard::getBasicMoveNotation(int fromRow, int fromCol, int toRow, int toCol, string promotion, bool enPassant)
{
    string piece = board[fromRow][fromCol];
    string targetPiece = board[toRow][toCol];

    string pieceSymbol = "";
    if (toupper(piece[0]) == 'R')
        pieceSymbol = "R";
    else if (toupper(piece[0]) == 'N')
        pieceSymbol = "N";
    else if (toupper(piece[0]) == 'B')
        pieceSymbol = "B";
    else if (toupper(piece[0]) == 'Q')
        pieceSymbol = "Q";
    else if (toupper(piece[0]) == 'K')
        pieceSymbol = "K";

    string disambiguator = getDisambiguator(fromRow, fromCol, toRow, toCol, piece);

    string capture = (targetPiece.empty() || targetPiece == " ") ? "" : "x";

    if (toupper(piece[0]) == 'P' && (!capture.empty() || enPassant))
    {
        capture = "x";
        pieceSymbol = char('a' + fromCol);
    }

    string dest = "";
    dest += char('a' + toCol);
    dest += char('8' - toRow);

    string promo = "";
    if (!promotion.empty())
    {
        promo = "=";
        if (toupper(promotion[0]) == 'Q')
            promo += "Q";
        else if (toupper(promotion[0]) == 'R')
            promo += "R";
        else if (toupper(promotion[0]) == 'B')
            promo += "B";
        else if (toupper(promotion[0]) == 'N')
            promo += "N";
    }

    string notation = pieceSymbol + disambiguator + capture + dest + promo;
    return notation;
}

void ChessBoard::updateUciMoveList(string move)
{
    uciMoveList.push_back(move);
}

void ChessBoard::updateMoveList(string move, bool whiteTurn)
{
    if (whiteTurn)
    {
        moveList.push_back({move, ""});
    }
    else if (moveList.size() >= 1)
    {
        moveList[moveList.size() - 1].second = move;
    }
}

string ChessBoard::getCurrentFen()
{
    string fen = "";

    for (int row = 0; row < 8; row++)
    {
        int emptyCount = 0;

        for (int col = 0; col < 8; col++)
        {
            string piece = board[row][col];

            if (piece == " ")
            {
                emptyCount++;
            }
            else
            {
                if (emptyCount > 0)
                {
                    fen += to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += piece;
            }
        }

        if (emptyCount > 0)
        {
            fen += to_string(emptyCount);
        }

        if (row < 7)
        {
            fen += "/";
        }
    }

    fen += " ";

    fen += whiteTurn ? "w" : "b";
    fen += " ";

    string castling = "";

    if (whiteCastling.canCastleKingside)
        castling += "K";
    if (whiteCastling.canCastleQueenside)
        castling += "Q";
    if (blackCastling.canCastleKingside)
        castling += "k";
    if (blackCastling.canCastleQueenside)
        castling += "q";

    if (castling.empty())
    {
        fen += "-";
    }
    else
    {
        fen += castling;
    }

    fen += " ";

    string enPassant = "-";

    if (uciMoveList.size() > 0)
    {
        string lastMove = uciMoveList.back();
        if (lastMove.length() == 4)
        {
            char fromRank = lastMove[1];
            char toFile = lastMove[2];
            char toRank = lastMove[3];

            int fromRow = 8 - (fromRank - '0');
            int toRow = 8 - (toRank - '0');

            string pieceMoved = board[toRow][toFile - 'a'];

            if ((pieceMoved == "P" || pieceMoved == "p") && abs(fromRow - toRow) == 2)
            {
                int possibleFirst = toFile - 'a' + 1;
                int possibleSecond = toFile - 'a' - 1;
                bool correct = false;

                if (pieceMoved == "P")
                {
                    if (possibleFirst >= 0 && possibleFirst <= 7)
                    {
                        if (board[toRow][possibleFirst] == "p")
                        {
                            correct = true;
                        }
                    }

                    if (possibleSecond >= 0 && possibleSecond <= 7)
                    {
                        if (board[toRow][possibleSecond] == "p")
                        {
                            correct = true;
                        }
                    }
                }
                else if (pieceMoved == "p")
                {
                    if (possibleFirst >= 0 && possibleFirst <= 7)
                    {
                        if (board[toRow][possibleFirst] == "P")
                        {
                            correct = true;
                        }
                    }

                    if (possibleSecond >= 0 && possibleSecond <= 7)
                    {
                        if (board[toRow][possibleSecond] == "P")
                        {
                            correct = true;
                        }
                    }
                }

                if (correct)
                {
                    int epRank = toRank + 1;
                    enPassant = string(1, toFile) + string(1, epRank);
                }
            }
        }
    }

    fen += enPassant;
    fen += " ";

    fen += to_string(halfMoves);
    fen += " ";

    fen += to_string(completeMoves);

    return fen;
}

bool ChessBoard::MovePiece(string from, string to, string promotion)
{
    if (from.length() != 2 || to.length() != 2 || mateStatus > 0)
        return false;

    int fromCol = tolower(from[0]) - 'a';
    int fromRow = 8 - (from[1] - '0');
    int toCol = tolower(to[0]) - 'a';
    int toRow = 8 - (to[1] - '0');

    CastlingRights castlingRights = whiteTurn ? ChessBoard::whiteCastling : ChessBoard::blackCastling;

    bool isValid = isValidMove(board, fromRow, fromCol, toRow, toCol, whiteTurn, castlingRights, uciMoveList);
    if (isValid)
    {
        string movePGN = "";

        string piece = board[fromRow][fromCol];
        string targetPiece = board[toRow][toCol];

        int rowDiff = toRow - fromRow;
        int colDiff = toCol - fromCol;

        halfMoves++;

        if (piece == "K")
        {
            ChessBoard::whiteCastling.canCastleKingside = false;
            ChessBoard::whiteCastling.canCastleQueenside = false;
        }
        else if (piece == "k")
        {
            ChessBoard::blackCastling.canCastleKingside = false;
            ChessBoard::blackCastling.canCastleQueenside = false;
        }
        else if (piece == "R")
        {
            if (fromCol == ChessBoard::whiteCastling.rookKStartCol)
            {
                ChessBoard::whiteCastling.canCastleKingside = false;
            }
            if (fromCol == ChessBoard::whiteCastling.rookQStartCol)
            {
                ChessBoard::whiteCastling.canCastleQueenside = false;
            }
        }
        else if (piece == "r")
        {
            if (fromCol == ChessBoard::blackCastling.rookKStartCol)
            {
                ChessBoard::blackCastling.canCastleKingside = false;
            }
            if (fromCol == ChessBoard::blackCastling.rookQStartCol)
            {
                ChessBoard::blackCastling.canCastleQueenside = false;
            }
        }

        if ((whiteTurn && piece == "P" && toRow == 0) || (!whiteTurn && piece == "p" && toRow == 7))
        {
            string prom = promotion;

            if ((prom == "q" && !whiteTurn) || (prom == "b" && !whiteTurn) || (prom == "n" && !whiteTurn) || (prom == "r" && !whiteTurn) || (prom == "Q" && whiteTurn) || (prom == "B" && whiteTurn) || (prom == "N" && whiteTurn) || (prom == "R" && whiteTurn))
            {
                movePGN = getBasicMoveNotation(fromRow, fromCol, toRow, toCol, prom, false);
                board[toRow][toCol] = prom;
                board[fromRow][fromCol] = " ";
                halfMoves = 0;
            }
            else
            {
                halfMoves--;
                return false;
            }
        }
        else if (whiteTurn && piece == "P" && abs(colDiff) == 1 && (targetPiece.empty() || targetPiece == " "))
        {
            movePGN = getBasicMoveNotation(fromRow, fromCol, toRow, toCol, "", true);
            board[toRow][toCol] = board[fromRow][fromCol];
            board[fromRow][fromCol] = " ";
            board[toRow + 1][toCol] = " ";
            halfMoves = 0;
        }
        else if (!whiteTurn && piece == "p" && abs(colDiff) == 1 && (targetPiece.empty() || targetPiece == " "))
        {
            movePGN = getBasicMoveNotation(fromRow, fromCol, toRow, toCol, "", true);
            board[toRow][toCol] = board[fromRow][fromCol];
            board[fromRow][fromCol] = " ";
            board[toRow - 1][toCol] = " ";
            halfMoves = 0;
        }
        else if ((((piece[0] == 'K' && targetPiece[0] == 'R' && whiteTurn) || (piece[0] == 'k' && targetPiece[0] == 'r' && !whiteTurn)) && rowDiff == 0) || (((piece[0] == 'k' && !whiteTurn) || (piece[0] == 'K' && whiteTurn)) && (rowDiff == 0 && (toCol == 2 || toCol == 6) && abs(colDiff) == 2)))
        {
            bool isCastlingKingside = false;
            bool isCastlingQueenside = false;
            if (colDiff >= 1)
                isCastlingKingside = true;
            else if (colDiff <= -1)
                isCastlingQueenside = true;

            int kingTargetRow = whiteTurn ? 7 : 0;
            string kingPiece = whiteTurn ? "K" : "k";
            string rookPiece = whiteTurn ? "R" : "r";
            int kingCol = castlingRights.kingStartCol;
            if (isCastlingKingside)
            {
                int rookCol = castlingRights.rookKStartCol;
                board[kingTargetRow][rookCol] = " ";
                board[kingTargetRow][kingCol] = " ";

                board[kingTargetRow][6] = kingPiece;
                board[kingTargetRow][5] = rookPiece;

                movePGN = "O-O";
            }
            else if (isCastlingQueenside)
            {
                int rookCol = castlingRights.rookQStartCol;
                board[kingTargetRow][rookCol] = " ";
                board[kingTargetRow][kingCol] = " ";

                board[kingTargetRow][2] = kingPiece;
                board[kingTargetRow][3] = rookPiece;

                movePGN = "O-O-O";
            }
        }
        else
        {
            movePGN = getBasicMoveNotation(fromRow, fromCol, toRow, toCol, "", false);

            if ((!targetPiece.empty() && targetPiece != " ") || tolower(piece[0]) == 'p')
            {
                halfMoves = 0;
            }

            board[toRow][toCol] = board[fromRow][fromCol];
            board[fromRow][fromCol] = " ";
        }

        if (!whiteTurn)
        {
            completeMoves++;
        }

        swapTurn();

        string curFen = getCurrentFen();
        prevFENList.push_back(curFen);

        mateStatus = isMate(whiteTurn);

        string uciMove = from + to + promotion;

        if (movePGN == "O-O-O")
        {
            if (uciMove == "e1a1")
                uciMove = "e1c1";
            if (uciMove == "e8a8")
                uciMove = "e8c8";
        }
        else if (movePGN == "O-O")
        {
            if (uciMove == "e1h1")
                uciMove = "e1g1";
            if (uciMove == "e8h8")
                uciMove = "e8g8";
        }

        if (isInCheck(whiteTurn))
        {
            if (isMate(whiteTurn) == 1)
            {
                movePGN += "#";
            }
            else
            {
                movePGN += "+";
            }
        }

        updateMoveList(movePGN, !whiteTurn);

        updateUciMoveList(uciMove);

        unordered_map<string, string> newMap = updateFenToPieceMap(pieceMapList.back(), uciMove);
        pieceMapList.push_back(newMap);

        // printPieceMapList(pieceMapList);

        // cout << "MOVE PGN:" << movePGN << endl;
        // cout << "UCI MOVE: " << uciMove << endl;

        return true;
    }
    return false;
}

bool ChessBoard::isInCheck(bool whiteTurn)
{
    char kingToFind = whiteTurn ? 'K' : 'k';
    int kingRow = -1, kingCol = -1;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (!board[row][col].empty() && board[row][col][0] == kingToFind)
            {
                kingRow = row;
                kingCol = col;
                break;
            }
        }
        if (kingRow != -1)
            break;
    }

    if (kingRow == -1)
        return false;

    return isSquareAttacked(board, kingRow, kingCol, !whiteTurn);
}

bool ChessBoard::hasAnyLegalMove()
{
    CastlingRights castlingRights = whiteTurn ? ChessBoard::whiteCastling : ChessBoard::blackCastling;

    for (int fromRow = 0; fromRow < 8; fromRow++)
    {
        for (int fromCol = 0; fromCol < 8; fromCol++)
        {
            string piece = board[fromRow][fromCol];

            if (piece.empty())
                continue;
            bool isWhitePiece = isupper(piece[0]);
            if ((whiteTurn && !isWhitePiece) || (!whiteTurn && isWhitePiece))
            {
                continue;
            }

            for (int toRow = 0; toRow < 8; toRow++)
            {
                for (int toCol = 0; toCol < 8; toCol++)
                {
                    if (fromRow == toRow && fromCol == toCol)
                        continue;

                    if (isValidMove(board, fromRow, fromCol, toRow, toCol, whiteTurn, castlingRights, uciMoveList))
                    {
                        // cout << "Legal move from: (" << fromRow << ", " << fromCol << ")" << " to: " << "(" << toRow << ", " << toCol << ")";
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

string removeLastFourFields(string fen)
{
    int spaceCount = 0;
    size_t lastSpacePos = 0;

    for (size_t i = 0; i < fen.length(); i++)
    {
        if (fen[i] == ' ')
        {
            spaceCount++;
            if (spaceCount == 3)
            {
                lastSpacePos = i;
                break;
            }
        }
    }

    if (lastSpacePos > 0)
    {
        return fen.substr(0, lastSpacePos);
    }

    return fen;
}

bool ChessBoard::IsThreefoldRepetition()
{
    if (prevFENList.empty())
    {
        return false;
    }

    string currentFEN = removeLastFourFields(prevFENList.back());

    int repetitionCount = 0;

    for (int i = 0; i < (int)prevFENList.size() - 1; i++)
    {
        string prevFEN = removeLastFourFields(prevFENList[i]);

        if (currentFEN == prevFEN)
        {
            repetitionCount++;
        }
    }

    return repetitionCount >= 2;
}

int ChessBoard::isMate(bool whiteTurn)
{
    if (mateStatus > 0)
    {
        return mateStatus;
    }

    if (halfMoves >= 100)
    {
        return 3;
    }

    bool hasLegalMove = hasAnyLegalMove();
    if (hasLegalMove)
        return 0;

    bool isCheck = isInCheck(whiteTurn);
    if (isCheck)
        return 1;
    else
        return 2;
}

void ChessBoard::printBoardContents(const vector<vector<string>> &board)
{
    cout << "Board contents (vector):" << endl;
    for (int row = 0; row < 8; row++)
    {
        cout << row << ": [";
        for (int col = 0; col < 8; col++)
        {
            if (board[row][col] == " ")
            {
                cout << "\" \"";
            }
            else
            {
                cout << "\"" << board[row][col] << "\"";
            }

            if (col < 7)
            {
                cout << ", ";
            }
        }
        cout << "]" << endl;
    }
}

vector<vector<string>> ChessBoard::getBoard()
{
    return board;
}

void ChessBoard::StartGame(string mode)
{
    string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    if (mode == "random")
    {
        fen = FenGenerators::generateRandomFEN();
    }
    else if (mode == "fischer")
    {
        fen = FenGenerators::generateFischerFEN();
    }
    setupFromFEN(fen);
}

void ChessBoard::StartGameFromFen(string fen)
{
    setupFromFEN(fen);
}

vector<pair<int, int>> ChessBoard::GetAllValidMoves(int fromRow, int fromCol)
{
    CastlingRights castlingRights = whiteTurn ? ChessBoard::whiteCastling : ChessBoard::blackCastling;
    return getAllValidMoves(board, fromRow, fromCol, whiteTurn, castlingRights, uciMoveList);
}

int ChessBoard::GetMateStatus()
{
    return mateStatus;
}

bool ChessBoard::IsValidMove(string from, string to)
{
    int fromCol = tolower(from[0]) - 'a';
    int fromRow = 8 - (from[1] - '0');
    int toCol = tolower(to[0]) - 'a';
    int toRow = 8 - (to[1] - '0');
    CastlingRights castlingRights = whiteTurn ? ChessBoard::whiteCastling : ChessBoard::blackCastling;
    return isValidMove(board, fromRow, fromCol, toRow, toCol, whiteTurn, castlingRights, uciMoveList);
}

vector<pair<string, string>> ChessBoard::GetMoveList()
{
    return moveList;
}

int ChessBoard::GetStartingMoveNumber()
{
    return startingCompleteMoves;
}

bool ChessBoard::GetMinSelectedMove0()
{
    return minSelectedMove0;
}

vector<vector<string>> ChessBoard::getBoardFromFEN(string fen)
{
    vector<vector<string>> newBoard(8, vector<string>(8, " "));

    vector<string> fenParts;
    size_t start = 0, end = fen.find(' ');

    while (end != string::npos)
    {
        fenParts.push_back(fen.substr(start, end - start));
        start = end + 1;
        end = fen.find(' ', start);
    }
    fenParts.push_back(fen.substr(start));

    if (fenParts.size() < 1)
        return newBoard;

    string boardFEN = fenParts[0];
    int row = 0, col = 0;

    for (char c : boardFEN)
    {
        if (c == '/')
        {
            row++;
            col = 0;
        }
        else if (isdigit(c))
        {
            col += c - '0';
        }
        else if (isalpha(c))
        {
            if (row < 8 && col < 8)
            {
                string piece = string(1, c);
                newBoard[row][col] = piece;
            }
            col++;
        }

        if (row >= 8)
            break;
    }

    return newBoard;
}

std::vector<std::vector<std::string>> ChessBoard::GetBoardAtIndex(int index)
{
    vector<vector<string>> newBoard(8, vector<string>(8, " "));
    if (prevFENList.size() < 1)
        return newBoard;

    int newInd = index;

    if (!minSelectedMove0)
    {
        newInd += 1;
    }

    if (minSelectedMove0 && newInd < 0)
    {
        newInd = 0;
    }

    if ((int)prevFENList.size() <= newInd)
        newInd = 0;

    string fen = prevFENList[newInd];
    newBoard = getBoardFromFEN(fen);
    return newBoard;
}

string ChessBoard::GetUciMoveAtIndex(int index)
{
    if (uciMoveList.empty())
        return "";

    int newInd = index;

    if (startEnPassant)
    {
        newInd += 1;
    }

    if (minSelectedMove0)
    {
        newInd -= 1;
    }

    if ((int)uciMoveList.size() <= newInd)
    {
        return "";
    }

    if (newInd < 0)
        return "";

    return uciMoveList[newInd];
}

bool ChessBoard::UndoMove()
{
    if ((int)prevFENList.size() > 1)
    {
        uciMoveList.pop_back();
        prevFENList.pop_back();
        pieceMapList.pop_back();

        if (moveList.back().second == "" || moveList.back().second.empty())
        {
            moveList.pop_back();
        }
        else
        {
            moveList.back().second = "";
        }

        setupFromFEN(prevFENList.back());

        cout << whiteTurn << endl;

        return true;
    }

    return false;
}

void ChessBoard::CallDraw()
{
    if (IsThreefoldRepetition())
    {
        mateStatus = 4;
    }
}

unordered_map<string, string> ChessBoard::GetPieceMapAtIndex(int index)
{
    unordered_map<string, string> map;
    if ((int)moveList.size() > 0 && moveList[0].first == "...")
    {
        index--;
    }
    if (index < 0 || index >= (int)pieceMapList.size())
    {
        return map;
    }

    map = pieceMapList[index];
    return map;
}