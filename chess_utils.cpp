#include "chess_utils.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include "chess_types.h"

using namespace std;

bool isSquareAttacked(const vector<vector<string>> &board, int row, int col, bool byWhite)
{
    // pawn
    int pawnDir = byWhite ? 1 : -1;
    if (row + pawnDir >= 0 && row + pawnDir < 8)
    {
        if (col - 1 >= 0)
        {
            string piece = board[row + pawnDir][col - 1];
            string whitePawn = "P";
            string blackPawn = "p";
            if ((byWhite && piece == whitePawn) || (!byWhite && piece == blackPawn))
                return true;
        }
        if (col + 1 < 8)
        {
            string piece = board[row + pawnDir][col + 1];
            string whitePawn = "P";
            string blackPawn = "p";
            if ((byWhite && piece == whitePawn) || (!byWhite && piece == blackPawn))
                return true;
        }
    }

    // knight
    int knightMoves[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    for (int (&move)[2] : knightMoves)
    {
        int newRow = row + move[0];
        int newCol = col + move[1];
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8)
        {
            string piece = board[newRow][newCol];
            string knight = byWhite ? "N" : "n";
            if (piece == knight)
                return true;
        }
    }

    // rook
    int rookDirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int (&dir)[2] : rookDirs)
    {
        int r = row + dir[0];
        int c = col + dir[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            string piece = board[r][c];
            if (piece != " ")
            {
                string rook = byWhite ? "R" : "r";
                string queen = byWhite ? "Q" : "q";
                if (piece == rook || piece == queen)
                    return true;
                break;
            }
            r += dir[0];
            c += dir[1];
        }
    }

    // bishop
    int bishopDirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    for (int (&dir)[2] : bishopDirs)
    {
        int r = row + dir[0];
        int c = col + dir[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            string piece = board[r][c];
            if (piece != " ")
            {
                string bishop = byWhite ? "B" : "b";
                string queen = byWhite ? "Q" : "q";
                if (piece == bishop || piece == queen)
                    return true;
                break;
            }
            r += dir[0];
            c += dir[1];
        }
    }

    // king
    int kingDirs[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
    for (int (&dir)[2] : kingDirs)
    {
        int r = row + dir[0];
        int c = col + dir[1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            string piece = board[r][c];
            string king = byWhite ? "K" : "k";
            if (piece == king)
                return true;
        }
    }

    return false;
}

bool isValidMove(const vector<vector<string>> &board, int fromRow, int fromCol, int toRow, int toCol, bool whiteTurn, CastlingRights castlingRights, vector<string> uciMoveList)
{
    bool valid = false;

    if (fromRow < 0 || fromRow > 7 || fromCol < 0 || fromCol > 7 ||
        toRow < 0 || toRow > 7 || toCol < 0 || toCol > 7)
    {
        return false;
    }

    if (fromRow == toRow && fromCol == toCol)
    {
        return false;
    }

    string piece = board[fromRow][fromCol];
    if (piece == " ")
    {
        return false;
    }

    int rowDiff = toRow - fromRow;
    int colDiff = toCol - fromCol;

    string targetPiece = board[toRow][toCol];
    if (!targetPiece.empty() && ((whiteTurn && isupper(targetPiece[0])) || (!whiteTurn && islower(targetPiece[0]))))
    {
        if (!(whiteTurn && piece == "K" && targetPiece == "R") && !(!whiteTurn && piece == "k" && targetPiece == "r"))
        {
            return false;
        }
    }

    bool isCastlingKingside = false;
    bool isCastlingQueenside = false;

    if (piece == "P" && whiteTurn)
    {
        if (colDiff == 0)
        {
            if (rowDiff == -1 && board[toRow][toCol] == " ")
                valid = true;
            if (fromRow == 6 && rowDiff == -2 && board[toRow][toCol] == " " && board[5][toCol] == " ")
                valid = true;
        }
        else if (abs(colDiff) == 1 && rowDiff == -1)
        {
            if (!targetPiece.empty() && islower(targetPiece[0]))
            {
                valid = true;
            }
            // en passant
            if (!valid && uciMoveList.size() > 0)
            {
                string lastMove = uciMoveList.back();
                if (lastMove.length() == 4)
                {
                    int lastFromCol = lastMove[0] - 'a';
                    int lastFromRow = 8 - (lastMove[1] - '0');
                    int lastToCol = lastMove[2] - 'a';
                    int lastToRow = 8 - (lastMove[3] - '0');

                    if (lastFromRow == 1 && lastToRow == 3 && lastFromCol == lastToCol && board[lastToRow][lastToCol] == "p")
                    {
                        int enPassantRow = lastToRow - 1;
                        int enPassantCol = lastToCol;

                        if (toRow == enPassantRow && toCol == enPassantCol)
                        {
                            valid = true;
                        }
                    }
                }
            }
        }
        if (!valid)
            return false;
    }
    else if (piece == "p" && !whiteTurn)
    {
        if (colDiff == 0)
        {
            if (rowDiff == 1 && board[toRow][toCol] == " ")
                valid = true;
            if (fromRow == 1 && rowDiff == 2 && board[toRow][toCol] == " " && board[2][toCol] == " ")
                valid = true;
        }
        else if (abs(colDiff) == 1 && rowDiff == 1)
        {
            if (!targetPiece.empty() && isupper(targetPiece[0]))
            {
                valid = true;
            }

            // en passant
            if (!valid && uciMoveList.size() > 0)
            {
                string lastMove = uciMoveList.back();
                if (lastMove.length() == 4)
                {
                    int lastFromCol = lastMove[0] - 'a';
                    int lastFromRow = 8 - (lastMove[1] - '0');
                    int lastToCol = lastMove[2] - 'a';
                    int lastToRow = 8 - (lastMove[3] - '0');

                    if (lastFromRow == 6 && lastToRow == 4 && lastFromCol == lastToCol && board[lastToRow][lastToCol] == "P")
                    {
                        int enPassantRow = lastToRow + 1;
                        int enPassantCol = lastToCol;

                        if (toRow == enPassantRow && toCol == enPassantCol)
                        {
                            valid = true;
                        }
                    }
                }
            }
        }
        if (!valid)
            return false;
    }
    else if ((piece == "R" && whiteTurn) || (piece == "r" && !whiteTurn))
    {
        if (rowDiff == 0 || colDiff == 0)
        {
            valid = true;
            if (rowDiff == 0)
            {
                int step = (colDiff > 0) ? 1 : -1;
                for (int c = fromCol + step; c != toCol; c += step)
                {
                    if (board[fromRow][c] != " ")
                    {
                        return false;
                    }
                }
            }
            else
            {
                int step = (rowDiff > 0) ? 1 : -1;
                for (int r = fromRow + step; r != toRow; r += step)
                {
                    if (board[r][fromCol] != " ")
                    {
                        return false;
                    }
                }
            }

            valid = true;
        }
    }
    else if ((piece == "N" && whiteTurn) || (piece == "n" && !whiteTurn))
    {
        if ((abs(rowDiff) == 2 && abs(colDiff) == 1) ||
            (abs(rowDiff) == 1 && abs(colDiff) == 2))
            valid = true;
        else
            return false;
    }
    else if ((piece == "B" && whiteTurn) || (piece == "b" && !whiteTurn))
    {
        if (abs(rowDiff) == abs(colDiff))
        {
            int rowStep = (rowDiff > 0) ? 1 : -1;
            int colStep = (colDiff > 0) ? 1 : -1;

            int r = fromRow + rowStep;
            int c = fromCol + colStep;

            while (r != toRow && c != toCol)
            {
                if (board[r][c] != " ")
                {
                    return false;
                }
                r += rowStep;
                c += colStep;
            }

            valid = true;
        }
    }
    else if ((piece == "Q" && whiteTurn) || (piece == "q" && !whiteTurn))
    {
        if ((rowDiff == 0 || colDiff == 0) ||
            (abs(rowDiff) == abs(colDiff)))
        {
            if (rowDiff == 0)
            {
                int step = (colDiff > 0) ? 1 : -1;
                for (int c = fromCol + step; c != toCol; c += step)
                {
                    if (board[fromRow][c] != " ")
                    {
                        return false;
                    }
                }
            }
            else if (colDiff == 0)
            {
                int step = (rowDiff > 0) ? 1 : -1;
                for (int r = fromRow + step; r != toRow; r += step)
                {
                    if (board[r][fromCol] != " ")
                    {
                        return false;
                    }
                }
            }
            else
            {
                int rowStep = (rowDiff > 0) ? 1 : -1;
                int colStep = (colDiff > 0) ? 1 : -1;

                int r = fromRow + rowStep;
                int c = fromCol + colStep;

                while (r != toRow && c != toCol)
                {
                    if (board[r][c] != " ")
                    {
                        return false;
                    }
                    r += rowStep;
                    c += colStep;
                }
            }

            valid = true;
        }
    }
    else if ((piece == "K" && whiteTurn) || (piece == "k" && !whiteTurn))
    {
        int kingTargetRow = whiteTurn ? 7 : 0;
        if ((((piece[0] == 'K' && targetPiece[0] == 'R' && whiteTurn) || (piece[0] == 'k' && targetPiece[0] == 'r' && !whiteTurn)) && rowDiff == 0 && fromRow == kingTargetRow) || (rowDiff == 0 && (toCol == 2 || toCol == 6) && abs(colDiff) == 2 && fromRow == kingTargetRow))
        {
            // castle queenside
            if (colDiff <= -1 && castlingRights.canCastleQueenside)
            {
                int kingTargetCol = 2;
                for (int i = min(castlingRights.kingStartCol, kingTargetCol); i <= max(castlingRights.kingStartCol, kingTargetCol); i++)
                {
                    if (isSquareAttacked(board, kingTargetRow, i, !whiteTurn))
                    {
                        return false;
                    }
                }
                if (kingTargetCol != castlingRights.kingStartCol)
                {
                    if (kingTargetCol > castlingRights.kingStartCol)
                    {
                        for (int i = castlingRights.kingStartCol + 1; i <= kingTargetCol; i++)
                        {
                            if (!(board[kingTargetRow][i].empty() || board[kingTargetRow][i] == " " || (castlingRights.rookQStartCol == i && tolower(board[kingTargetRow][i][0]) == 'r')))
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        for (int i = castlingRights.kingStartCol - 1; i >= kingTargetCol; i--)
                        {
                            if (!(board[kingTargetRow][i].empty() || board[kingTargetRow][i] == " " || (castlingRights.rookQStartCol == i && tolower(board[kingTargetRow][i][0]) == 'r')))
                            {
                                return false;
                            }
                        }
                    }
                }
                for (int i = castlingRights.rookQStartCol + 1; i <= kingTargetCol + 1; i++)
                {
                    if (!(board[kingTargetRow][i].empty() || board[kingTargetRow][i] == " " || tolower(board[kingTargetRow][i][0]) == 'k'))
                    {
                        return false;
                    }
                }
                isCastlingQueenside = true;
            }
            // castle kingside
            else if (colDiff >= 1 && castlingRights.canCastleKingside)
            {
                int kingTargetCol = 6;
                for (int i = min(castlingRights.kingStartCol, kingTargetCol); i <= max(castlingRights.kingStartCol, kingTargetCol); i++)
                {
                    if (isSquareAttacked(board, kingTargetRow, i, !whiteTurn))
                    {
                        return false;
                    }
                }
                if (kingTargetCol != castlingRights.kingStartCol)
                {
                    if (kingTargetCol > castlingRights.kingStartCol)
                    {
                        for (int i = castlingRights.kingStartCol + 1; i <= kingTargetCol; i++)
                        {
                            if (!(board[kingTargetRow][i].empty() || board[kingTargetRow][i] == " " || (castlingRights.rookKStartCol == i && tolower(board[kingTargetRow][i][0]) == 'r')))
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        for (int i = castlingRights.kingStartCol - 1; i >= kingTargetCol; i--)
                        {
                            if (!(board[kingTargetRow][i].empty() || board[kingTargetRow][i] == " " || (castlingRights.rookKStartCol == i && tolower(board[kingTargetRow][i][0]) == 'r')))
                            {
                                return false;
                            }
                        }
                    }
                }
                for (int i = castlingRights.rookKStartCol - 1; i >= kingTargetCol - 1; i--)
                {
                    if (!(board[kingTargetRow][i].empty() || board[kingTargetRow][i] == " " || tolower(board[kingTargetRow][i][0]) == 'k'))
                    {
                        return false;
                    }
                }
                isCastlingKingside = true;
            }
            else
            {
                return false;
            }

            valid = true;
        }
        else if (abs(rowDiff) <= 1 && abs(colDiff) <= 1)
        {
            if (!targetPiece.empty() && ((whiteTurn && isupper(targetPiece[0])) || (!whiteTurn && islower(targetPiece[0]))))
            {
                return false;
            }
            else
            {
                valid = true;
            }
        }
    }

    if (!valid)
        return false;

    vector<vector<string>> tempBoard = board;

    if (!isCastlingKingside && !isCastlingQueenside)
    {
        tempBoard[toRow][toCol] = tempBoard[fromRow][fromCol];
        tempBoard[fromRow][fromCol] = " ";
    }
    else
    {
        int kingTargetRow = whiteTurn ? 7 : 0;
        string kingPiece = whiteTurn ? "K" : "k";
        string rookPiece = whiteTurn ? "R" : "r";
        int kingCol = castlingRights.kingStartCol;
        if (isCastlingKingside)
        {
            if (castlingRights.rookKStartCol > 0 && kingCol > 0)
            {
                int rookCol = castlingRights.rookKStartCol;
                tempBoard[kingTargetRow][rookCol] = " ";
                tempBoard[kingTargetRow][kingCol] = " ";

                tempBoard[kingTargetRow][6] = kingPiece;
                tempBoard[kingTargetRow][5] = rookPiece;
            }
        }
        else if (isCastlingQueenside)
        {
            if (castlingRights.rookQStartCol > 0 && kingCol > 0)
            {
                int rookCol = castlingRights.rookQStartCol;
                tempBoard[kingTargetRow][rookCol] = " ";
                tempBoard[kingTargetRow][kingCol] = " ";

                tempBoard[kingTargetRow][2] = kingPiece;
                tempBoard[kingTargetRow][3] = rookPiece;
            }
        }
    }

    char kingToFind = whiteTurn ? 'K' : 'k';
    int kingRow = -1, kingCol = -1;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            string piece = tempBoard[i][j];
            if (!piece.empty() && piece[0] == kingToFind)
            {
                kingRow = i;
                kingCol = j;
                break;
            }
        }
        if (kingRow != -1)
            break;
    }

    if (kingRow == -1)
    {
        cout << "INVALID POSITION DETECTED!" << endl;
        return false;
    }

    bool kingInCheck = isSquareAttacked(tempBoard, kingRow, kingCol, !whiteTurn);
    if (kingInCheck)
    {
        return false;
    }

    return valid;
}

vector<pair<int, int>> getAllValidMoves(const vector<vector<string>> &board,
                                        int fromRow, int fromCol, bool whiteTurn, CastlingRights castlingRights, vector<string> uciMoveList)
{
    vector<pair<int, int>> validMoves;

    if (fromRow < 0 || fromRow > 7 || fromCol < 0 || fromCol > 7)
    {
        return validMoves;
    }

    const string &piece = board[fromRow][fromCol];
    if (piece.empty() || piece == " ")
    {
        return validMoves;
    }

    bool isWhitePiece = !piece.empty() && isupper(piece[0]);
    if ((whiteTurn && !isWhitePiece) || (!whiteTurn && isWhitePiece))
    {
        return validMoves;
    }

    for (int toRow = 0; toRow < 8; toRow++)
    {
        for (int toCol = 0; toCol < 8; toCol++)
        {
            if (fromRow == toRow && fromCol == toCol)
            {
                continue;
            }

            const string &targetPiece = board[toRow][toCol];
            if (!targetPiece.empty() && targetPiece != " ")
            {
                bool isTargetWhite = isupper(targetPiece[0]);
                if ((whiteTurn && isTargetWhite) || (!whiteTurn && !isTargetWhite))
                {
                    if (!(whiteTurn && piece == "K" && targetPiece == "R") && !(!whiteTurn && piece == "k" && targetPiece == "r"))
                    {
                        continue;
                    }
                }
            }

            if (isValidMove(board, fromRow, fromCol, toRow, toCol, whiteTurn, castlingRights, uciMoveList))
            {
                validMoves.push_back(make_pair(toRow, toCol));
            }
        }
    }

    return validMoves;
}