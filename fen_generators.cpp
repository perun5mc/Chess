#include "fen_generators.h"
#include "chess_utils.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <random>
#include <iostream>

using namespace std;

namespace FenGenerators
{
    mt19937 rng(random_device{}());

    string generateFischerFEN()
    {
        vector<char> backRank(8, ' ');
        vector<int> squares;

        for (int i = 0; i < 8; i++)
            squares.push_back(i);

        // bishops on opposite colors
        int darkBishop = rng() % 4 * 2;
        int lightBishop = rng() % 4 * 2 + 1;

        backRank[darkBishop] = 'B';
        backRank[lightBishop] = 'B';

        squares.erase(remove(squares.begin(), squares.end(), darkBishop), squares.end());
        squares.erase(remove(squares.begin(), squares.end(), lightBishop), squares.end());

        // queen
        int queenPos = squares[rng() % squares.size()];
        backRank[queenPos] = 'Q';
        squares.erase(remove(squares.begin(), squares.end(), queenPos), squares.end());

        // knights
        int knight1Pos = squares[rng() % squares.size()];
        backRank[knight1Pos] = 'N';
        squares.erase(remove(squares.begin(), squares.end(), knight1Pos), squares.end());

        int knight2Pos = squares[rng() % squares.size()];
        backRank[knight2Pos] = 'N';
        squares.erase(remove(squares.begin(), squares.end(), knight2Pos), squares.end());

        // rook, king and rook
        sort(squares.begin(), squares.end());
        int kingPos = squares[1];
        backRank[kingPos] = 'K';
        backRank[squares[0]] = 'R';
        backRank[squares[2]] = 'R';

        string fenRank = "";
        int emptyCount = 0;

        for (int i = 0; i < 8; i++)
        {
            if (backRank[i] == ' ')
            {
                emptyCount++;
            }
            else
            {
                if (emptyCount > 0)
                {
                    fenRank += to_string(emptyCount);
                    emptyCount = 0;
                }
                fenRank += backRank[i];
            }
        }

        if (emptyCount > 0)
        {
            fenRank += to_string(emptyCount);
        }

        string blackRank = fenRank;
        transform(blackRank.begin(), blackRank.end(), blackRank.begin(), ::tolower);

        string fen = blackRank + "/pppppppp/8/8/8/8/PPPPPPPP/" + fenRank;

        string castling = "KQkq";

        if (castling.empty())
            castling = "-";

        fen += " w " + castling + " - 0 1";

        cout << fen << endl;

        return fen;
    }

    string generateRandomFEN()
    {
        vector<char> whitePieces = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
        vector<char> blackPieces = {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'};

        vector<char> piecesToPlace;

        piecesToPlace.push_back('P');
        piecesToPlace.push_back('N');
        piecesToPlace.push_back('B');
        piecesToPlace.push_back('R');
        piecesToPlace.push_back('Q');
        piecesToPlace.push_back('K');

        piecesToPlace.push_back('p');
        piecesToPlace.push_back('n');
        piecesToPlace.push_back('b');
        piecesToPlace.push_back('r');
        piecesToPlace.push_back('q');
        piecesToPlace.push_back('k');

        shuffle(piecesToPlace.begin(), piecesToPlace.end(), rng);

        bool isBoardValid = false;
        vector<vector<string>> validBoard;

        while (!isBoardValid)
        {
            vector<vector<string>> tempBoard(8, vector<string>(8, " "));
            int kingsPos[2][2];

            for (char piece : piecesToPlace)
            {
                bool placed = false;

                while (!placed)
                {
                    int row = rng() % 8;
                    int col = rng() % 8;

                    if (tempBoard[row][col] == " ")
                    {
                        bool isValid = true;

                        string pieceStr(1, piece);

                        if (piece == 'P' || piece == 'p')
                        {
                            if (row == 0 || row == 7)
                            {
                                isValid = false;
                            }
                        }

                        if (piece == 'K' || piece == 'k')
                        {
                            for (int dr = -1; dr <= 1; dr++)
                            {
                                for (int dc = -1; dc <= 1; dc++)
                                {
                                    if (dr == 0 && dc == 0)
                                        continue;

                                    int nr = row + dr;
                                    int nc = col + dc;

                                    if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8)
                                    {
                                        string adjacentPiece = tempBoard[nr][nc];
                                        if ((piece == 'K' && adjacentPiece == "k") ||
                                            (piece == 'k' && adjacentPiece == "K"))
                                        {
                                            isValid = false;
                                        }
                                    }
                                }
                            }

                            if (isValid)
                            {
                                if (piece == 'K')
                                {
                                    kingsPos[0][0] = row;
                                    kingsPos[0][1] = col;
                                }
                                else
                                {
                                    kingsPos[1][0] = row;
                                    kingsPos[1][1] = col;
                                }
                            }
                        }

                        if (isValid)
                        {
                            tempBoard[row][col] = pieceStr;
                            placed = true;
                        }
                    }
                }
            }

            if (!isSquareAttacked(tempBoard, kingsPos[0][0], kingsPos[0][1], false) &&
                !isSquareAttacked(tempBoard, kingsPos[1][0], kingsPos[1][1], true))
            {
                isBoardValid = true;
                validBoard = tempBoard;
            }
        }

        string fen = "";
        for (int row = 0; row < 8; row++)
        {
            int emptyCount = 0;
            for (int col = 0; col < 8; col++)
            {
                if (validBoard[row][col] == " ")
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
                    fen += validBoard[row][col];
                }
            }
            if (emptyCount > 0)
            {
                fen += to_string(emptyCount);
            }
            if (row < 7)
            {
                fen += '/';
            }
        }

        fen += " w - - 0 1";

        return fen;
    }
}
