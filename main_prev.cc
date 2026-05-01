#include "raylib.h"
#include "chess_backend.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "chess_themes.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using namespace std;
namespace fs = filesystem;

Color LIGHT_SQUARE = {238, 238, 210, 255};
Color DARK_SQUARE = {118, 150, 86, 255};
Color HIGHLIGHT_COLOR = {250, 250, 0, 127};
const Color PANEL_COLOR = {50, 50, 60, 255};
const Color PANEL_BORDER = {80, 80, 90, 255};

enum class ScreenState
{
    MAIN_MENU,
    CHESS_BOARD,
    FEN_INPUT
};

const int MIN_SCREEN_WIDTH = 800;
const int MIN_SCREEN_HEIGHT = 600;
const int BOARD_SIZE = 8;

int screenWidth = 900;
int screenHeight = 700;
ScreenState currentScreen = ScreenState::MAIN_MENU;

unordered_map<string, Texture2D> pieceTextures;
vector<string> pieceSets;

string currentPieceSet = "neo";
bool showPieceSetDropdown = false;
int selectedPieceSetIndex = 0;

string currentBoardSet = "classic";
bool showBoardsDropdown = false;

bool boardFlipped = false;

char fenInput[256] = "";

int squareSize = 0;
int boardWidth = 0;
int boardHeight = 0;
int panelWidth = 0;
int totalWidth = 0; // board + panel + spacing
int boardOffsetX = 0;
int boardOffsetY = 0;
int panelOffsetX = 0;
int panelOffsetY = 0;
float textureScale = 0.9f;

bool showAreYouSureMenu = false;

int selectedMoveIndex = -1;
int moveListScroll = 0;

bool animating = false;
float currentAnimationProgress = 0.0f;
bool reverseAnimation = false;
float animationSpeed = 6.0f;
bool undidMove = false;
string undoneMove = "";
vector<vector<string>> undoneBoard;

int GetLastMoveIndex(const vector<pair<string, string>> &list)
{
    if (list.empty() || (int)list.size() < 1)
    {
        return -1;
    }

    int lastMoveIndex;
    if (list.back().second.empty())
    {
        lastMoveIndex = (list.size() * 2) - 2;
    }
    else
    {
        lastMoveIndex = (list.size() * 2) - 1;
    }

    return lastMoveIndex;
}

bool IsLastMoveSelected(const vector<pair<string, string>> &list)
{
    if (list.empty() || (int)list.size() < 1)
    {
        if (selectedMoveIndex == -1)
            return true;

        return false;
    }

    int lastMoveIndex = GetLastMoveIndex(list);

    return (selectedMoveIndex == lastMoveIndex);
}

void selectLastMove(vector<pair<string, string>> list, bool animate = true, bool reverse = false)
{
    animating = animate;
    reverseAnimation = reverse;
    currentAnimationProgress = 0;

    if (list.size() < 1)
    {
        selectedMoveIndex = -1;
        return;
    }

    int removeIfLastEmpty = 0;
    if (list[list.size() - 1].second.empty())
    {
        removeIfLastEmpty = 1;
    }
    selectedMoveIndex = (list.size() * 2) - 1 - removeIfLastEmpty;
}

void scrollToBottom()
{
    moveListScroll = -INFINITY;
}

vector<string> whitePromotionPieces = {"Q", "N", "R", "B"};
vector<string> blackPromotionPieces = {"b", "r", "n", "q"};

struct PromotionInfo
{
    string sourceSquare;
    string targetSquare;
    int squareRow;
    int squareCol;
    bool isWhite;

    PromotionInfo(const string &src, const string &tgt, int row, int col, bool white)
    {
        sourceSquare = src;
        targetSquare = tgt;
        squareRow = row;
        squareCol = col;
        isWhite = white;
    }
};
bool isPromoting = false;
PromotionInfo *promotionInfo = nullptr;

int mateStatus = 0;
bool mateStatusWindowClosed = false;

ChessBoard *chessGame;

string selectedSquare;
bool isDragging;

void ChangeMoveSelection(int by)
{
    if (by == 0)
        return;

    if ((selectedMoveIndex > -1 && by < 0) || (selectedMoveIndex < GetLastMoveIndex(chessGame->GetMoveList()) && by > 0))
    {
        selectedMoveIndex += by;
        selectedSquare = "";
        isDragging = false;
        animating = true;
        currentAnimationProgress = 0;
        if (by < 0)
        {
            reverseAnimation = true;
        }
        else
        {
            reverseAnimation = false;
        }
    }
}

void calculateDimensions()
{
    const int SPACING = 10;
    const int MARGIN = 40;

    int availableWidth = screenWidth - MARGIN * 2;
    int availableHeight = screenHeight - MARGIN * 2;

    int maxBoardWidthFromWidth = (availableWidth - SPACING) / 1.4f;

    int maxBoardHeightFromHeight = availableHeight;

    int maxSquareFromWidth = maxBoardWidthFromWidth / BOARD_SIZE;
    int maxSquareFromHeight = maxBoardHeightFromHeight / BOARD_SIZE;

    squareSize = min(maxSquareFromWidth, maxSquareFromHeight);

    squareSize = max(squareSize, 40);

    boardWidth = squareSize * BOARD_SIZE;
    boardHeight = squareSize * BOARD_SIZE;

    panelWidth = max(200.0f, min(300.0f, boardWidth * 0.4f));

    totalWidth = boardWidth + SPACING + panelWidth;

    boardOffsetX = (screenWidth - totalWidth) / 2;
    boardOffsetY = (screenHeight - boardHeight) / 2;

    panelOffsetX = boardOffsetX + boardWidth + SPACING;
    panelOffsetY = boardOffsetY;
}

vector<string> discoverPieceSets()
{
    vector<string> sets;

    if (!fs::exists("./img"))
    {
        return sets;
    }

    for (const auto &entry : fs::directory_iterator("./img"))
    {
        if (entry.is_directory())
        {
            string setName = entry.path().filename().string();

            vector<string> requiredFiles = {"wK.png", "wQ.png", "wR.png", "wB.png", "wN.png", "wP.png"};
            bool hasFiles = true;

            for (const auto &file : requiredFiles)
            {
                string filePath = "./img/" + setName + "/" + file;
                if (!fs::exists(filePath))
                {
                    hasFiles = false;
                    break;
                }
            }

            if (hasFiles)
            {
                sets.push_back(setName);
            }
        }
    }

    sort(sets.begin(), sets.end());

    return sets;
}

void unloadTextures()
{
    for (auto &pair : pieceTextures)
    {
        UnloadTexture(pair.second);
    }
    pieceTextures.clear();
}

void loadPieceTextures()
{
    unloadTextures();

    vector<string> pieces = {"K", "Q", "R", "B", "N", "P", "k", "q", "r", "b", "n", "p"};

    for (const auto &piece : pieces)
    {
        char pieceChar = piece[0];
        bool white = (pieceChar >= 'A' && pieceChar <= 'Z');
        string color = white ? "w" : "b";

        string pieceType;
        switch (toupper(pieceChar))
        {
        case 'K':
            pieceType = "K";
            break;
        case 'Q':
            pieceType = "Q";
            break;
        case 'R':
            pieceType = "R";
            break;
        case 'B':
            pieceType = "B";
            break;
        case 'N':
            pieceType = "N";
            break;
        case 'P':
            pieceType = "P";
            break;
        default:
            continue;
        }

        string filename = "./img/" + currentPieceSet + "/" + color + pieceType + ".png";

        if (FileExists(filename.c_str()))
        {
            Texture2D texture = LoadTexture(filename.c_str());
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
            pieceTextures[piece] = texture;
        }
        else
        {
            Image placeholder = GenImageColor(64, 64, white ? WHITE : BLACK);
            Texture2D texture = LoadTextureFromImage(placeholder);
            UnloadImage(placeholder);
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
            pieceTextures[piece] = texture;
        }
    }
}

void drawPossibleMoves(string selectedSquare)
{
    if (selectedSquare.empty())
        return;

    int selectedRow = 8 - (selectedSquare[1] - '0');
    int selectedCol = tolower(selectedSquare[0]) - 'a';

    vector<pair<int, int>> validMoves = chessGame->GetAllValidMoves(selectedRow, selectedCol);

    for (const pair<int, int> &move : validMoves)
    {
        int toRow = move.first;
        int toCol = move.second;

        int visualRow = boardFlipped ? (BOARD_SIZE - 1 - toRow) : toRow;
        int visualCol = boardFlipped ? (BOARD_SIZE - 1 - toCol) : toCol;

        float centerX = boardOffsetX + visualCol * squareSize + squareSize / 2.0f;
        float centerY = boardOffsetY + visualRow * squareSize + squareSize / 2.0f;

        bool isCapture = chessGame->getBoard()[toRow][toCol] != " ";

        Color dotColor = {0, 0, 0, 75};

        Vector2 mousePos = GetMousePosition();

        int col = (mousePos.x - boardOffsetX) / squareSize;
        int row = (mousePos.y - boardOffsetY) / squareSize;

        if (col == visualCol && row == visualRow)
        {
            Rectangle rect = {
                boardOffsetX + visualCol * squareSize,
                boardOffsetY + visualRow * squareSize,
                squareSize,
                squareSize};

            float roundness = 0.2f;
            float thickness = 2.5f;
            int segments = 0;

            DrawRectangle(boardOffsetX + visualCol * squareSize,
                          boardOffsetY + visualRow * squareSize,
                          squareSize, squareSize, dotColor);
        }
        else
        {
            if (isCapture)
            {
                float outerRadius = squareSize * 0.50f;
                float innerRadius = squareSize * 0.40f;
                Vector2 pos = {centerX, centerY};

                DrawRing(pos, innerRadius, outerRadius, 0, 360, 72, dotColor);
            }
            else
            {
                float radius = squareSize * 0.15f;
                DrawCircle(centerX, centerY, radius, dotColor);
            }
        }
    }
}

bool MovePiece(string from, string to, string promotion, bool animate = true)
{
    bool success = chessGame->MovePiece(from, to, promotion);
    mateStatus = chessGame->GetMateStatus();

    if (success)
    {
        scrollToBottom();
        selectLastMove(chessGame->GetMoveList(), animate, false);
    }

    return success;
}

void handlePromotionClick()
{
    if (!isPromoting || promotionInfo == nullptr)
    {
        isPromoting = false;
        promotionInfo = nullptr;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    {
        delete promotionInfo;
        promotionInfo = nullptr;
        isPromoting = false;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mousePos = GetMousePosition();

        vector<string> pieces;
        bool isWhite = promotionInfo->isWhite;

        if (isWhite)
        {
            pieces = whitePromotionPieces;
        }
        else
        {
            pieces = blackPromotionPieces;
        }

        for (int i = 0; i < pieces.size(); i++)
        {
            int pieceRow = promotionInfo->squareRow;
            int pieceCol = promotionInfo->squareCol;

            int rowOffset = isWhite ? (i) : (i - 3);

            int targetRow = pieceRow + rowOffset;

            int visualRow = boardFlipped ? (BOARD_SIZE - 1 - targetRow) : targetRow;
            int visualCol = boardFlipped ? (BOARD_SIZE - 1 - pieceCol) : pieceCol;

            if (visualRow < 0 || visualRow >= BOARD_SIZE)
            {
                continue;
            }

            int x = boardOffsetX + visualCol * squareSize;
            int y = boardOffsetY + visualRow * squareSize;

            if (mousePos.x >= x && mousePos.x < x + squareSize &&
                mousePos.y >= y && mousePos.y < y + squareSize)
            {
                string promotionPiece = pieces[i];

                MovePiece(promotionInfo->sourceSquare, promotionInfo->targetSquare, promotionPiece);

                delete promotionInfo;
                promotionInfo = nullptr;
                isPromoting = false;
                return;
            }
        }

        delete promotionInfo;
        promotionInfo = nullptr;
        isPromoting = false;
        return;
    }
}

void clickDetection()
{
    if (mateStatus > 0 || showAreYouSureMenu || !IsLastMoveSelected(chessGame->GetMoveList()))
        return;

    if (isPromoting)
    {
        handlePromotionClick();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isDragging && !selectedSquare.empty()))
    {
        Vector2 mousePos = GetMousePosition();

        if (mousePos.x >= boardOffsetX &&
            mousePos.x < boardOffsetX + boardWidth &&
            mousePos.y >= boardOffsetY &&
            mousePos.y < boardOffsetY + boardHeight)
        {
            int col = (mousePos.x - boardOffsetX) / squareSize;
            int row = (mousePos.y - boardOffsetY) / squareSize;

            int visualRow = boardFlipped ? (BOARD_SIZE - 1 - row) : row;
            int visualCol = boardFlipped ? (BOARD_SIZE - 1 - col) : col;

            col = max(0, min(visualCol, BOARD_SIZE - 1));
            row = max(0, min(visualRow, BOARD_SIZE - 1));

            char file = 'a' + col;
            int rank = BOARD_SIZE - row;

            string newSquare = file + to_string(rank);

            cout << "[" << row << "," << col << "] ";
            cout << "(" << file << rank << ")" << endl;

            string piece = chessGame->getBoard()[visualRow][visualCol];
            bool whiteTurn = chessGame->GetWhiteTurn();
            bool isUpper = !piece.empty() && isupper(piece[0]);

            bool pieceBelongsToCurrentPlayer = (!piece.empty() && piece != " ") &&
                                               ((isUpper && whiteTurn) || (!isUpper && !whiteTurn));

            if (selectedSquare.empty())
            {
                if (pieceBelongsToCurrentPlayer)
                {
                    selectedSquare = newSquare;
                    isDragging = true;
                }
            }
            else
            {
                if (pieceBelongsToCurrentPlayer)
                {
                    int selectedCol = tolower(selectedSquare[0]) - 'a';
                    int selectedRow = 8 - (selectedSquare[1] - '0');
                    string selectedPiece = chessGame->getBoard()[selectedRow][selectedCol];
                    if (tolower(piece[0]) == 'r' && tolower(selectedPiece[0]) == 'k')
                    {
                        if (!MovePiece(selectedSquare, newSquare, ""))
                        {
                            if (!isDragging)
                            {
                                selectedSquare = newSquare;
                                isDragging = true;
                            }
                            else if (selectedSquare != newSquare)
                            {
                                selectedSquare = "";
                            }
                        }
                        else
                        {
                            selectedSquare = "";
                        }
                    }
                    else
                    {
                        if (!isDragging)
                        {
                            selectedSquare = newSquare;
                            isDragging = true;
                        }
                        else if (selectedSquare != newSquare)
                        {
                            selectedSquare = "";
                        }
                    }
                }
                else
                {
                    int selectedCol = tolower(selectedSquare[0]) - 'a';
                    int selectedRow = 8 - (selectedSquare[1] - '0');
                    string selectedPiece = chessGame->getBoard()[selectedRow][selectedCol];

                    if ((selectedPiece == "P" && rank == 8) ||
                        (selectedPiece == "p" && rank == 1))
                    {
                        bool isMoveValid = chessGame->IsValidMove(selectedSquare, newSquare);
                        if (isMoveValid)
                        {
                            isPromoting = true;
                            promotionInfo = new PromotionInfo(selectedSquare, newSquare,
                                                              row, col, selectedPiece == "P");
                        }
                    }
                    else
                    {
                        MovePiece(selectedSquare, newSquare, "", !isDragging);
                    }
                    selectedSquare = "";
                }
            }
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isDragging)
    {
        isDragging = false;
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !selectedSquare.empty())
    {
        selectedSquare = "";
        isDragging = false;
    }
}

void drawPromotionMenu()
{
    if (!isPromoting || promotionInfo == nullptr)
        return;

    DrawRectangle(0, 0, screenWidth, screenHeight, {0, 0, 0, 160});

    vector<string> pieces;
    bool isWhite = chessGame->GetWhiteTurn();
    bool flipOrder = (boardFlipped && isWhite) || (!boardFlipped && !isWhite);

    if (isWhite)
    {
        pieces = whitePromotionPieces;
    }
    else
    {
        pieces = blackPromotionPieces;
    }

    for (int i = 0; i < pieces.size(); i++)
    {
        int pieceRow = promotionInfo->squareRow;
        int pieceCol = promotionInfo->squareCol;

        int rowOffset = isWhite ? (i) : (i - 3);

        int targetRow = pieceRow + rowOffset;

        int visualRow = boardFlipped ? (BOARD_SIZE - 1 - targetRow) : targetRow;
        int visualCol = boardFlipped ? (BOARD_SIZE - 1 - pieceCol) : pieceCol;

        if (visualRow < 0 || visualRow >= BOARD_SIZE)
        {
            continue;
        }

        int x = boardOffsetX + visualCol * squareSize;
        int y = boardOffsetY + visualRow * squareSize;

        Color lightSquare = {150, 140, 50, 255};
        Color darkSquare = {130, 120, 20, 255};
        Color squareColor = ((targetRow + pieceCol) % 2 == 0) ? lightSquare : darkSquare;

        DrawRectangle(x, y, squareSize, squareSize, squareColor);

        string piece = pieces[i];
        if (piece != " " && pieceTextures.find(piece) != pieceTextures.end())
        {
            Texture2D texture = pieceTextures[piece];

            float scale = (float)squareSize / max(texture.width, texture.height) * 0.8f;
            int width = texture.width * scale;
            int height = texture.height * scale;
            int pieceX = x + (squareSize - width) / 2;
            int pieceY = y + (squareSize - height) / 2;

            DrawTextureEx(texture, Vector2{(float)pieceX, (float)pieceY}, 0.0f, scale, WHITE);
        }

        Vector2 mousePos = GetMousePosition();

        int col = (mousePos.x - boardOffsetX) / squareSize;
        int row = (mousePos.y - boardOffsetY) / squareSize;

        if (col == visualCol && row == visualRow)
        {
            Rectangle rect = {x, y, squareSize, squareSize};

            DrawRectangleLinesEx(rect, 4.0f, {255, 255, 255, 128});
        }
    }
}

float EaseInOutCubic(float t)
{
    return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

Vector2 Vector2Lerp(Vector2 from, Vector2 to, float progress)
{
    Vector2 result;
    result.x = from.x + (to.x - from.x) * progress;
    result.y = from.y + (to.y - from.y) * progress;
    return result;
}

void drawChessBoard()
{
    int selectedRow = -1;
    int selectedCol = -1;

    bool isLastMoveSelected = IsLastMoveSelected(chessGame->GetMoveList());

    if (!selectedSquare.empty())
    {
        selectedRow = 8 - (selectedSquare[1] - '0');
        selectedCol = tolower(selectedSquare[0]) - 'a';

        selectedRow = boardFlipped ? (BOARD_SIZE - 1 - selectedRow) : selectedRow;
        selectedCol = boardFlipped ? (BOARD_SIZE - 1 - selectedCol) : selectedCol;
    }

    vector<vector<string>> board;
    if (isLastMoveSelected)
    {
        board = chessGame->getBoard();
    }
    else
    {
        board = chessGame->GetBoardAtIndex(selectedMoveIndex);
    }

    string highlightedMove = chessGame->GetUciMoveAtIndex(selectedMoveIndex);

    int highlightFromCol = -1;
    int highlightFromRow = -1;
    int highlightToCol = -1;
    int highlightToRow = -1;

    if (highlightedMove.length() >= 4)
    {
        highlightFromCol = tolower(highlightedMove[0]) - 'a';
        highlightFromRow = 8 - (highlightedMove[1] - '0');
        highlightToCol = tolower(highlightedMove[2]) - 'a';
        highlightToRow = 8 - (highlightedMove[3] - '0');

        highlightFromCol = boardFlipped ? (BOARD_SIZE - 1 - highlightFromCol) : highlightFromCol;
        highlightFromRow = boardFlipped ? (BOARD_SIZE - 1 - highlightFromRow) : highlightFromRow;
        highlightToRow = boardFlipped ? (BOARD_SIZE - 1 - highlightToRow) : highlightToRow;
        highlightToCol = boardFlipped ? (BOARD_SIZE - 1 - highlightToCol) : highlightToCol;
    }

    Texture2D draggingTexture;
    float draggingPieceScale;
    int draggingPieceWidth;
    int draggingPieceHeight;

    bool draggingParametersAssigned = false;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Color squareColor = ((row + col) % 2 == 0) ? LIGHT_SQUARE : DARK_SQUARE;

            int visualRow = boardFlipped ? (BOARD_SIZE - 1 - row) : row;
            int visualCol = boardFlipped ? (BOARD_SIZE - 1 - col) : col;

            DrawRectangle(boardOffsetX + visualCol * squareSize,
                          boardOffsetY + visualRow * squareSize,
                          squareSize, squareSize, squareColor);

            if (visualRow == selectedRow && visualCol == selectedCol && isLastMoveSelected)
            {
                DrawRectangle(boardOffsetX + visualCol * squareSize,
                              boardOffsetY + visualRow * squareSize,
                              squareSize, squareSize, HIGHLIGHT_COLOR);
            }

            if ((visualRow == highlightFromRow && visualCol == highlightFromCol) || (visualRow == highlightToRow && visualCol == highlightToCol))
            {
                DrawRectangle(boardOffsetX + visualCol * squareSize,
                              boardOffsetY + visualRow * squareSize,
                              squareSize, squareSize, HIGHLIGHT_COLOR);
            }
        }
    }

    if (!selectedSquare.empty() && !isPromoting && isLastMoveSelected)
    {
        drawPossibleMoves(selectedSquare);
    }

    int moveToAnimateIndex = reverseAnimation ? selectedMoveIndex + 1 : selectedMoveIndex;
    string moveToAnimate = undidMove ? undoneMove : chessGame->GetUciMoveAtIndex(moveToAnimateIndex);

    int animFromCol;
    int animFromRow;
    int animToCol;
    int animToRow;
    int animVisualFromCol;
    int animVisualFromRow;
    int animVisualToCol;
    int animVisualToRow;
    string animPiece;
    if (animating && moveToAnimate.length() >= 4)
    {
        animFromCol = tolower(moveToAnimate[0]) - 'a';
        animFromRow = 8 - (moveToAnimate[1] - '0');
        animToCol = tolower(moveToAnimate[2]) - 'a';
        animToRow = 8 - (moveToAnimate[3] - '0');

        animVisualFromCol = boardFlipped ? (BOARD_SIZE - 1 - animFromCol) : animFromCol;
        animVisualFromRow = boardFlipped ? (BOARD_SIZE - 1 - animFromRow) : animFromRow;
        animVisualToCol = boardFlipped ? (BOARD_SIZE - 1 - animToCol) : animToCol;
        animVisualToRow = boardFlipped ? (BOARD_SIZE - 1 - animToRow) : animToRow;

        animPiece = undidMove ? undoneBoard[animToRow][animToCol] : chessGame->GetBoardAtIndex(moveToAnimateIndex)[animToRow][animToCol];
    }

    Texture2D animTexture;
    int animTextureWidth;
    int animTextureHeight;
    float animTextureScale;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            int visualRow = boardFlipped ? (BOARD_SIZE - 1 - row) : row;
            int visualCol = boardFlipped ? (BOARD_SIZE - 1 - col) : col;

            string piece = board[row][col];
            if (piece != " " && pieceTextures.find(piece) != pieceTextures.end())
            {
                Texture2D texture = pieceTextures[piece];

                float scale = (float)squareSize / max(texture.width, texture.height) * textureScale;
                int width = texture.width * scale;
                int height = texture.height * scale;
                int x = boardOffsetX + visualCol * squareSize + (squareSize - width) / 2;
                int y = boardOffsetY + visualRow * squareSize + (squareSize - height) / 2;

                Vector2 position = {(float)x, (float)y};
                if (!animating || (!(visualRow == animVisualFromRow && visualCol == animVisualFromCol && piece == animPiece) && !(visualCol == animVisualToCol && visualRow == animVisualToRow && piece == animPiece)))
                {

                    if (isDragging && visualRow == selectedRow && visualCol == selectedCol)
                    {
                        DrawTextureEx(texture, position, 0.0f, scale, {255, 255, 255, 128});
                        draggingTexture = texture;
                        draggingPieceScale = scale;
                        draggingPieceWidth = width;
                        draggingPieceHeight = height;
                        draggingParametersAssigned = true;
                    }
                    else
                    {
                        DrawTextureEx(texture, position, 0.0f, scale, WHITE);
                    }
                }
                else
                {
                    animTexture = texture;
                    animTextureWidth = width;
                    animTextureHeight = height;
                    animTextureScale = scale;
                }
            }
        }
    }

    int actualFromRow = reverseAnimation ? animVisualToRow : animVisualFromRow;
    int actualFromCol = reverseAnimation ? animVisualToCol : animVisualFromCol;
    int actualToRow = reverseAnimation ? animVisualFromRow : animVisualToRow;
    int actualToCol = reverseAnimation ? animVisualFromCol : animVisualToCol;

    int fromX = boardOffsetX + actualFromCol * squareSize + (squareSize - animTextureWidth) / 2;
    int fromY = boardOffsetY + actualFromRow * squareSize + (squareSize - animTextureHeight) / 2;
    int toX = boardOffsetX + actualToCol * squareSize + (squareSize - animTextureWidth) / 2;
    int toY = boardOffsetY + actualToRow * squareSize + (squareSize - animTextureHeight) / 2;

    Vector2 fromPosition = {(float)fromX, (float)fromY};
    Vector2 toPosition = {(float)toX, (float)toY};

    Vector2 currentPosition = Vector2Lerp(fromPosition, toPosition, EaseInOutCubic(currentAnimationProgress));
    DrawTextureEx(animTexture, currentPosition, 0.0f, animTextureScale, WHITE);

    currentAnimationProgress += GetFrameTime() * animationSpeed;
    if (currentAnimationProgress >= 1)
    {
        currentAnimationProgress = 0;
        animating = false;
        reverseAnimation = false;
        undidMove = false;
        undoneMove = "";
        undoneBoard.clear();
    }

    if (isDragging && draggingParametersAssigned)
    {
        Vector2 mousePos = GetMousePosition();

        int x = mousePos.x - draggingPieceWidth / 2;
        int y = mousePos.y - draggingPieceHeight / 2;

        Vector2 position = {(float)x, (float)y};
        DrawTextureEx(draggingTexture, position, 0.0f, draggingPieceScale, WHITE);
    }

    if (isPromoting && isLastMoveSelected)
    {
        drawPromotionMenu();
    }
}

void drawBoardCoordinates()
{
    int coordFontSize = max(14, squareSize / 6);

    for (int col = 0; col < BOARD_SIZE; col++)
    {
        char letter = 'a' + col;
        string letterStr = string(1, letter);
        int visualCol = boardFlipped ? (BOARD_SIZE - 1 - col) : col;
        int textWidth = MeasureText(letterStr.c_str(), coordFontSize);
        int x = boardOffsetX + visualCol * squareSize + (squareSize - textWidth) / 2;
        int y = boardOffsetY + boardHeight + 5;
        DrawText(letterStr.c_str(), x, y, coordFontSize, WHITE);
    }

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        int rank = (BOARD_SIZE - row);
        string number = to_string(rank);
        int visualRow = boardFlipped ? (BOARD_SIZE - 1 - row) : row;
        int textWidth = MeasureText(number.c_str(), coordFontSize);
        int x = boardOffsetX - textWidth - 5;
        int y = boardOffsetY + visualRow * squareSize + (squareSize - coordFontSize) / 2;
        DrawText(number.c_str(), x, y, coordFontSize, WHITE);
    }
}

void GoToMainMenu()
{
    if (chessGame != nullptr)
    {
        delete chessGame;
        chessGame = nullptr;
    }

    selectedSquare = "";
    mateStatus = 0;
    isPromoting = false;
    promotionInfo = nullptr;
    mateStatusWindowClosed = false;
    showAreYouSureMenu = false;
    selectedMoveIndex = -1;
    currentScreen = ScreenState::MAIN_MENU;
    chessGame = new ChessBoard();
}

void TryToGoToMainMenu(bool sure)
{
    if (currentScreen == ScreenState::CHESS_BOARD && !sure)
    {
        showAreYouSureMenu = true;
    }
    else
    {
        showAreYouSureMenu = false;
        GoToMainMenu();
    }
}

Rectangle boardDropdownRect = {0};
Vector2 boardDropdownScroll = {0};

Rectangle pieceSetDropdownRect = {0};
Vector2 pieceSetDropdownScroll = {0};
const int DROPDOWN_ITEM_HEIGHT = 35;

void drawSidePanel()
{
    DrawRectangle(panelOffsetX, panelOffsetY, panelWidth, boardHeight, PANEL_COLOR);
    DrawRectangleLinesEx(Rectangle{(float)panelOffsetX, (float)panelOffsetY,
                                   (float)panelWidth, (float)boardHeight},
                         2, PANEL_BORDER);

    if (chessGame->GetMinSelectedMove0() && selectedMoveIndex < 0)
    {
        selectedMoveIndex = 0;
    }

    vector<pair<string, string>> moveList = chessGame->GetMoveList();
    int startMoves = chessGame->GetStartingMoveNumber();

    int moveListWidth = panelWidth * 0.9f;
    int moveListHeight = boardHeight * 0.3f;
    int moveListX = panelOffsetX + (panelWidth - moveListWidth) / 2;
    int moveListY = panelOffsetY + boardHeight / 2 - moveListHeight / 2;

    int titleFontSize = max(18, panelWidth / 15);
    string title = "Move List";
    int titleWidth = MeasureText(title.c_str(), titleFontSize);
    DrawText(title.c_str(), panelOffsetX + (panelWidth - titleWidth) / 2,
             moveListY - titleFontSize - 5, titleFontSize, RAYWHITE);

    DrawRectangle(moveListX, moveListY, moveListWidth, moveListHeight, {30, 30, 40, 255});
    DrawRectangleLinesEx(Rectangle{(float)moveListX, (float)moveListY,
                                   (float)moveListWidth, (float)moveListHeight},
                         2, GRAY);

    int moveButtonHeight = max(30, moveListHeight / 15);
    int moveButtonSpacing = 5;
    int moveNumberWidth = MeasureText("999.", 18) + 10;
    int moveButtonWidth = (moveListWidth - moveNumberWidth - 30) / 2;
    int padding = 2;

    int contentHeight = 0;
    if (!moveList.empty())
    {
        contentHeight = moveList.size() * (moveButtonHeight + moveButtonSpacing) - moveButtonSpacing + (padding * 2);
    }

    Rectangle scrollBounds = {(float)moveListX + 5, (float)moveListY + 5,
                              (float)moveListWidth - 10, (float)moveListHeight - 10};

    Rectangle contentRec = {0, 0, (float)moveListWidth - 25, (float)contentHeight};
    Vector2 scroll = {0, (float)moveListScroll};

    int prevBorderWidth = GuiGetStyle(LISTVIEW, BORDER_WIDTH);
    GuiSetStyle(LISTVIEW, BORDER_WIDTH, 0);

    Rectangle view = {0};
    GuiScrollPanel(scrollBounds, NULL, contentRec, &scroll, &view);

    moveListScroll = scroll.y;

    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

    float currentY = scroll.y + padding;

    for (size_t i = 0; i < moveList.size(); i++)
    {
        float buttonY = view.y + currentY;

        if (buttonY + moveButtonHeight >= view.y && buttonY <= view.y + view.height)
        {
            string moveNumber = to_string(i + startMoves) + ".";
            DrawText(moveNumber.c_str(), view.x + 5,
                     buttonY + moveButtonHeight / 2 - 9,
                     18, RAYWHITE);

            Rectangle whiteMoveBtn = {view.x + moveNumberWidth,
                                      buttonY,
                                      (float)moveButtonWidth, (float)moveButtonHeight};

            if (!showPieceSetDropdown && !showBoardsDropdown)
            {
                int prevMoveIndex = selectedMoveIndex;
                if (!moveList[i].first.empty())
                {
                    if (GuiButton(whiteMoveBtn, moveList[i].first.c_str()))
                    {
                        selectedMoveIndex = i * 2;
                    }
                }

                if (!moveList[i].second.empty())
                {
                    Rectangle blackMoveBtn = {view.x + moveNumberWidth + moveButtonWidth + 5,
                                              buttonY,
                                              (float)moveButtonWidth, (float)moveButtonHeight};

                    if (GuiButton(blackMoveBtn, moveList[i].second.c_str()))
                    {
                        selectedMoveIndex = i * 2 + 1;
                    }
                }

                if (selectedMoveIndex - prevMoveIndex != 0)
                {
                    if (selectedMoveIndex - prevMoveIndex == -1)
                    {
                        animating = true;
                        reverseAnimation = true;
                    }
                    else if (selectedMoveIndex - prevMoveIndex == 1)
                    {
                        reverseAnimation = false;
                        animating = true;
                    }
                    undidMove = false;
                    currentAnimationProgress = 0;
                    undoneMove = "";
                    undoneBoard.clear();
                }
            }

            if (selectedMoveIndex / 2 == i && selectedMoveIndex >= 0)
            {
                if (selectedMoveIndex % 2 == 0 && !moveList[i].first.empty())
                {
                    DrawRectangleLinesEx(whiteMoveBtn, 2, YELLOW);
                }
                else if (!moveList[i].second.empty())
                {
                    Rectangle blackMoveBtn = {view.x + moveNumberWidth + moveButtonWidth + 5,
                                              buttonY,
                                              (float)moveButtonWidth, (float)moveButtonHeight};
                    DrawRectangleLinesEx(blackMoveBtn, 2, YELLOW);
                }
            }
        }

        currentY += moveButtonHeight + moveButtonSpacing;
    }
    EndScissorMode();

    int buttonAreaY = moveListY + moveListHeight + 10;
    int buttonWidth = panelWidth * 0.4f;
    int buttonHeight = 35;
    int buttonSpacing = panelWidth * 0.05f;

    int totalButtonsWidth = buttonWidth * 2 + buttonSpacing;
    int buttonsStartX = panelOffsetX + (panelWidth - totalButtonsWidth) / 2;

    Rectangle backBtn = {(float)buttonsStartX,
                         (float)buttonAreaY,
                         (float)buttonWidth, (float)buttonHeight};

    Rectangle nextBtn = {(float)buttonsStartX + buttonWidth + buttonSpacing,
                         (float)buttonAreaY,
                         (float)buttonWidth, (float)buttonHeight};

    if (GuiButton(backBtn, "Back"))
    {
        if (!showBoardsDropdown && !showPieceSetDropdown)
        {
            ChangeMoveSelection(-1);
        }
    }

    if (GuiButton(nextBtn, "Next"))
    {
        if (!showBoardsDropdown && !showPieceSetDropdown)
        {
            ChangeMoveSelection(1);
        }
    }

    buttonWidth = panelWidth * 0.7f;
    buttonHeight = 35;
    buttonsStartX = panelOffsetX + (panelWidth - buttonWidth) / 2;
    Rectangle undoButton = {(float)buttonsStartX,
                            (float)buttonAreaY + 50,
                            (float)buttonWidth, (float)buttonHeight};

    if (GuiButton(undoButton, "Undo move"))
    {
        if (!showBoardsDropdown && !showPieceSetDropdown)
        {
            string lastMove = chessGame->GetUciMoveAtIndex(GetLastMoveIndex(chessGame->GetMoveList()));
            vector<vector<string>> tmpUndoneBoard = chessGame->getBoard();
            if (chessGame->UndoMove())
            {
                undoneBoard = tmpUndoneBoard;
                undoneMove = lastMove;
                undidMove = true;
                scrollToBottom();
                selectLastMove(chessGame->GetMoveList(), true, true);
            };
        }
    }

    int menuButtonWidth = panelWidth * 0.85f;
    int menuButtonHeight = 35;
    int bottomMargin = 20;

    Rectangle menuBtn = {
        (float)panelOffsetX + (panelWidth - menuButtonWidth) / 2,
        (float)panelOffsetY + boardHeight - menuButtonHeight - bottomMargin,
        (float)menuButtonWidth,
        (float)menuButtonHeight};

    if (GuiButton(menuBtn, "Main Menu"))
    {
        if (!showBoardsDropdown && !showPieceSetDropdown)
        {
            if (!(mateStatus > 0 && !mateStatusWindowClosed))
                TryToGoToMainMenu(false);
        }
    }

    if (chessGame->IsThreefoldRepetition())
    {
        buttonWidth = panelWidth * 0.7f;
        buttonHeight = 35;
        buttonsStartX = panelOffsetX + (panelWidth - buttonWidth) / 2;
        buttonAreaY = panelOffsetY + boardHeight * 0.225f - titleFontSize;
        Rectangle drawButton = {(float)buttonsStartX,
                                (float)buttonAreaY,
                                (float)buttonWidth, (float)buttonHeight};

        if (GuiButton(drawButton, "Take draw"))
        {
            if (!showBoardsDropdown && !showPieceSetDropdown)
            {
                chessGame->CallDraw();
                mateStatus = chessGame->GetMateStatus();
            }
        }

        string tieText = "Threefold repetition";
        int tieTextWidth = MeasureText(tieText.c_str(), titleFontSize);
        DrawText(tieText.c_str(), panelOffsetX + (panelWidth - tieTextWidth) / 2, buttonAreaY + 35, titleFontSize, RAYWHITE);
    }

    int dropdownWidth = panelWidth * 0.85f;
    int dropdownHeight = 35;
    int startY = panelOffsetY + dropdownHeight + 20;

    Rectangle dropdownBtn = {(float)panelOffsetX + (panelWidth - dropdownWidth) / 2,
                             (float)startY,
                             (float)dropdownWidth,
                             (float)dropdownHeight};

    if (GuiButton(dropdownBtn, currentBoardSet.c_str()))
    {
        if (!showPieceSetDropdown)
        {
            showBoardsDropdown = !showBoardsDropdown;
        }
    }

    float arrowX = dropdownBtn.x + dropdownBtn.width - 20;
    float arrowY = dropdownBtn.y + dropdownBtn.height / 2;

    if (showBoardsDropdown)
    {
        DrawTriangle(
            {arrowX - 5, arrowY + 5},
            {arrowX + 5, arrowY + 5},
            {arrowX, arrowY - 5},
            LIGHTGRAY);
    }
    else
    {
        DrawTriangle(
            {arrowX, arrowY + 5},
            {arrowX + 5, arrowY - 5},
            {arrowX - 5, arrowY - 5},
            LIGHTGRAY);
    }

    if (showBoardsDropdown)
    {
        int maxDropdownHeight = boardHeight - (dropdownHeight + 75);
        int listHeight = min(maxDropdownHeight,
                             (int)CHESS_THEMES.size() * DROPDOWN_ITEM_HEIGHT + 15);

        boardDropdownRect = {
            dropdownBtn.x,
            dropdownBtn.y + dropdownHeight + 2,
            dropdownBtn.width,
            (float)listHeight};

        DrawRectangleRec(boardDropdownRect, {40, 40, 50, 255});
        DrawRectangleLinesEx(boardDropdownRect, 2, {80, 80, 90, 255});

        int contentHeight = CHESS_THEMES.size() * DROPDOWN_ITEM_HEIGHT + 10;
        Rectangle contentRec = {0, 0, boardDropdownRect.width - 25, (float)contentHeight};

        Rectangle view = {0};
        GuiScrollPanel(boardDropdownRect, NULL, contentRec, &boardDropdownScroll, &view);

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

        float currentY = boardDropdownScroll.y;

        for (const auto &[themeName, colors] : CHESS_THEMES)
        {
            if (currentY + DROPDOWN_ITEM_HEIGHT >= 0 && currentY <= view.height)
            {
                Rectangle itemRect = {
                    view.x + 5,
                    view.y + currentY + 5,
                    view.width,
                    (float)DROPDOWN_ITEM_HEIGHT - 5};

                bool isSelected = (themeName == currentBoardSet);
                bool isHovered = CheckCollisionPointRec(GetMousePosition(), itemRect);

                Color itemColor = {50, 50, 60, 255};
                if (isSelected)
                {
                    itemColor = {70, 100, 150, 255};
                }
                else if (isHovered)
                {
                    itemColor = {60, 60, 70, 255};
                }

                DrawRectangleRec(itemRect, itemColor);

                int textFontSize = min(20, screenHeight / 10);
                // int textFontSize = 16;
                // int titleFontSize = min(60, screenHeight / 10);
                DrawText(themeName.c_str(),
                         itemRect.x + 10,
                         itemRect.y + itemRect.height / 2 - textFontSize / 2,
                         textFontSize, RAYWHITE);

                if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    string newBoard = themeName;
                    if (newBoard != currentBoardSet)
                    {
                        currentBoardSet = newBoard;
                        LIGHT_SQUARE = colors[0];
                        DARK_SQUARE = colors[1];
                        HIGHLIGHT_COLOR = colors[2];
                    }
                }
            }

            currentY += DROPDOWN_ITEM_HEIGHT;
        }

        EndScissorMode();
    }

    if (showBoardsDropdown && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mousePos = GetMousePosition();

        bool clickedInDropdownBtn = CheckCollisionPointRec(mousePos, dropdownBtn);
        bool clickedInDropdownList = showBoardsDropdown &&
                                     CheckCollisionPointRec(mousePos, boardDropdownRect);

        if (!clickedInDropdownBtn && !clickedInDropdownList)
        {
            showBoardsDropdown = false;
        }
    }

    startY = panelOffsetY + 20;

    dropdownBtn = {(float)panelOffsetX + (panelWidth - dropdownWidth) / 2,
                   (float)startY,
                   (float)dropdownWidth,
                   (float)dropdownHeight};

    if (GuiButton(dropdownBtn, currentPieceSet.c_str()))
    {
        showPieceSetDropdown = !showPieceSetDropdown;

        if (pieceSets.empty())
        {
            pieceSets = discoverPieceSets();
            if (pieceSets.empty())
            {
                pieceSets.push_back(currentPieceSet);
            }
        }
    }

    arrowX = dropdownBtn.x + dropdownBtn.width - 20;
    arrowY = dropdownBtn.y + dropdownBtn.height / 2;

    if (showPieceSetDropdown)
    {
        DrawTriangle(
            {arrowX - 5, arrowY + 5},
            {arrowX + 5, arrowY + 5},
            {arrowX, arrowY - 5},
            LIGHTGRAY);
    }
    else
    {
        DrawTriangle(
            {arrowX, arrowY + 5},
            {arrowX + 5, arrowY - 5},
            {arrowX - 5, arrowY - 5},
            LIGHTGRAY);
    }

    if (showPieceSetDropdown && !pieceSets.empty())
    {
        int maxDropdownHeight = boardHeight - (dropdownHeight + 40);
        int listHeight = min(maxDropdownHeight,
                             (int)pieceSets.size() * DROPDOWN_ITEM_HEIGHT + 15);

        pieceSetDropdownRect = {
            dropdownBtn.x,
            dropdownBtn.y + dropdownHeight + 2,
            dropdownBtn.width,
            (float)listHeight};

        DrawRectangleRec(pieceSetDropdownRect, {40, 40, 50, 255});
        DrawRectangleLinesEx(pieceSetDropdownRect, 2, {80, 80, 90, 255});

        int contentHeight = pieceSets.size() * DROPDOWN_ITEM_HEIGHT + 10;
        Rectangle contentRec = {0, 0, pieceSetDropdownRect.width - 25, (float)contentHeight};

        Rectangle view = {0};
        GuiScrollPanel(pieceSetDropdownRect, NULL, contentRec, &pieceSetDropdownScroll, &view);

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

        float currentY = pieceSetDropdownScroll.y;

        for (size_t i = 0; i < pieceSets.size(); i++)
        {
            if (currentY + DROPDOWN_ITEM_HEIGHT >= 0 && currentY <= view.height)
            {
                Rectangle itemRect = {
                    view.x + 5,
                    view.y + currentY + 5,
                    view.width,
                    (float)DROPDOWN_ITEM_HEIGHT - 5};

                bool isSelected = (pieceSets[i] == currentPieceSet);
                bool isHovered = CheckCollisionPointRec(GetMousePosition(), itemRect);

                Color itemColor = {50, 50, 60, 255};
                if (isSelected)
                {
                    itemColor = {70, 100, 150, 255};
                }
                else if (isHovered)
                {
                    itemColor = {60, 60, 70, 255};
                }

                DrawRectangleRec(itemRect, itemColor);

                int textFontSize = min(20, screenHeight / 10);
                // int textFontSize = 16;
                // int titleFontSize = min(60, screenHeight / 10);
                DrawText(pieceSets[i].c_str(),
                         itemRect.x + 10,
                         itemRect.y + itemRect.height / 2 - textFontSize / 2,
                         textFontSize, RAYWHITE);

                if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    string newPieceSet = pieceSets[i];
                    if (newPieceSet != currentPieceSet)
                    {
                        currentPieceSet = newPieceSet;
                        loadPieceTextures();
                    }
                }
            }

            currentY += DROPDOWN_ITEM_HEIGHT;
        }

        EndScissorMode();
    }

    if (showPieceSetDropdown && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mousePos = GetMousePosition();

        bool clickedInDropdownBtn = CheckCollisionPointRec(mousePos, dropdownBtn);
        bool clickedInDropdownList = showPieceSetDropdown &&
                                     CheckCollisionPointRec(mousePos, pieceSetDropdownRect);

        if (!clickedInDropdownBtn && !clickedInDropdownList)
        {
            showPieceSetDropdown = false;
        }
    }

    GuiSetStyle(LISTVIEW, BORDER_WIDTH, prevBorderWidth);
}

void drawMainMenu()
{
    Color color1 = {30, 30, 40, 255};
    DrawRectangle(0, 0, screenWidth, screenHeight, color1);

    int titleFontSize = min(60, screenHeight / 10);
    string title = "CHESS GAME";
    int titleWidth = MeasureText(title.c_str(), titleFontSize);
    DrawText(title.c_str(), screenWidth / 2 - titleWidth / 2, screenHeight / 6, titleFontSize, GOLD);

    int buttonWidth = max(300, screenWidth * 1 / 2);
    int buttonHeight = min(60, screenHeight / 10);
    int buttonSpacing = 20;
    int startY = screenHeight / 3;

    Rectangle standardBtn = {screenWidth / 2 - buttonWidth / 2, startY, (float)buttonWidth, (float)buttonHeight};
    if (GuiButton(standardBtn, "Standard Position"))
    {
        chessGame->StartGame("standard");
        currentScreen = ScreenState::CHESS_BOARD;
    }

    Rectangle randomBtn = {screenWidth / 2 - buttonWidth / 2, startY + buttonHeight + buttonSpacing,
                           (float)buttonWidth, (float)buttonHeight};
    if (GuiButton(randomBtn, "Random Position"))
    {
        chessGame->StartGame("random");
        currentScreen = ScreenState::CHESS_BOARD;
    }

    Rectangle fischerBtn = {screenWidth / 2 - buttonWidth / 2, startY + (buttonHeight + buttonSpacing) * 2,
                            (float)buttonWidth, (float)buttonHeight};
    if (GuiButton(fischerBtn, "Fischer Random"))
    {
        chessGame->StartGame("fischer");
        currentScreen = ScreenState::CHESS_BOARD;
    }

    Rectangle fenBtn = {screenWidth / 2 - buttonWidth / 2, startY + (buttonHeight + buttonSpacing) * 3,
                        (float)buttonWidth, (float)buttonHeight};
    if (GuiButton(fenBtn, "Custom FEN"))
    {
        currentScreen = ScreenState::FEN_INPUT;
        strcpy(fenInput, "");
    }
}

bool showFenErrorPopup = false;
void drawFENInput()
{
    Color color1 = {30, 30, 40, 255};
    DrawRectangle(0, 0, screenWidth, screenHeight, color1);

    int titleFontSize = min(40, screenHeight / 15);
    string title = "ENTER FEN STRING";
    int titleWidth = MeasureText(title.c_str(), titleFontSize);
    DrawText(title.c_str(), screenWidth / 2 - titleWidth / 2, screenHeight / 4, titleFontSize, GOLD);

    int inputWidth = min(900, screenWidth * 5 / 6);
    int inputHeight = 40;
    Rectangle inputRect = {screenWidth / 2 - inputWidth / 2, screenHeight / 2 - inputHeight / 2,
                           (float)inputWidth, (float)inputHeight};

    Color color2 = {60, 60, 70, 255};
    DrawRectangleRec(inputRect, color2);
    DrawRectangleLinesEx(inputRect, 2, GRAY);

    GuiTextBox(inputRect, fenInput, 256, true);

    int exampleFontSize = min(18, screenHeight / 35);
    string example = "Example: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int exampleWidth = MeasureText(example.c_str(), exampleFontSize);
    DrawText(example.c_str(), screenWidth / 2 - exampleWidth / 2,
             screenHeight / 2 + 50, exampleFontSize, RAYWHITE);

    int buttonWidth = max(200, screenWidth / 6);
    int buttonHeight = max(40, screenHeight / 20);
    int buttonSpacing = 20;

    Rectangle startBtn = {screenWidth / 2 - buttonWidth - buttonSpacing / 2,
                          screenHeight * 3 / 4, (float)buttonWidth, (float)buttonHeight};

    if (GuiButton(startBtn, "Start Game") && !showFenErrorPopup)
    {
        string fenString(fenInput);

        if (!fenString.empty())
        {
            chessGame->StartGameFromFen(fenString);
            currentScreen = ScreenState::CHESS_BOARD;
        }
        else
        {
            showFenErrorPopup = true;
        }
    }

    if (showFenErrorPopup)
    {
        int guiWidth = max(250, screenWidth / 6);
        int guiHeight = max(180, screenHeight / 10);

        Rectangle rec = {
            screenWidth / 2 - (guiWidth / 2),
            screenHeight / 2 - (guiHeight / 2),
            guiWidth,
            guiHeight};

        int result = GuiMessageBox(
            rec,
            "Invalid FEN",
            "FEN can't be empty!",
            "OK");

        if (result == 1 || result == 0)
        {
            showFenErrorPopup = false;
        }
    }

    Rectangle backBtn = {screenWidth / 2 + buttonSpacing / 2,
                         screenHeight * 3 / 4, (float)buttonWidth, (float)buttonHeight};
    if (GuiButton(backBtn, "Back") && !showFenErrorPopup)
    {
        TryToGoToMainMenu(false);
    }
}

void drawChessBoardScreen()
{
    drawChessBoard();
    drawBoardCoordinates();

    drawSidePanel();

    if (showAreYouSureMenu)
    {
        int guiWidth = max(300, screenWidth / 6);
        int guiHeight = max(200, screenHeight / 8);

        Rectangle rec = {
            screenWidth / 2 - (guiWidth / 2),
            screenHeight / 2 - (guiHeight / 2),
            guiWidth,
            guiHeight};

        int result = GuiMessageBox(
            rec,
            "Confirm exit",
            "Are you sure you\n\nwant to exit?",
            "Yes;No");

        if (result == 2 || result == 0)
        {
            showAreYouSureMenu = false;
        }
        else if (result == 1)
        {
            TryToGoToMainMenu(true);
        }
    }
    else if (mateStatus > 0 && !mateStatusWindowClosed)
    {
        int guiWidth = max(350, screenWidth / 6);
        int guiHeight = max(200, screenHeight / 8);

        string message = "Game ended!\n\n";
        if (mateStatus == 1)
        {
            bool whiteWon = !chessGame->GetWhiteTurn();
            message += (whiteWon ? (string) "White" : (string) "Black");
            message += " won by checkmate!";
        }
        else if (mateStatus == 2)
        {
            message += "It's a draw by stalemate!";
        }
        else if (mateStatus == 3)
        {
            message += "It's a draw by 50 moves rule!";
        }
        else if (mateStatus == 4)
        {
            message += "It's a draw by repetition!";
        }
        Rectangle rec = {
            screenWidth / 2 - (guiWidth / 2),
            screenHeight / 2 - (guiHeight / 2),
            guiWidth,
            guiHeight};

        int result = GuiMessageBox(
            rec,
            "Game ended",
            message.c_str(),
            "Export game;Main Menu");

        if (result == 2)
        {
            mateStatusWindowClosed = true;
            TryToGoToMainMenu(true);
        }
        else if (result == 1)
        {
            chessGame->ExportMoves();
            cout << "EXPORT STUFF" << endl;
        }
        else if (result == 0)
        {
            mateStatusWindowClosed = true;
        }
    }
}

void handleWindowResize()
{
    if (IsWindowResized())
    {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        // if (screenWidth * GetWindowScaleDPI().x < MIN_SCREEN_WIDTH)
        // {
        //     screenWidth = MIN_SCREEN_WIDTH;
        //     SetWindowSize(screenWidth, screenHeight);
        // }
        // if (screenHeight * GetWindowScaleDPI().y < MIN_SCREEN_HEIGHT)
        // {
        //     screenHeight = MIN_SCREEN_HEIGHT;
        //     SetWindowSize(screenWidth, screenHeight);
        // }

        calculateDimensions();

        int dynamicFontSize = max(14, min(25, screenHeight / 30));
        GuiSetStyle(DEFAULT, TEXT_SIZE, dynamicFontSize);
    }
}

void SetGuiStyles()
{
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt({50, 50, 50, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt({50, 50, 50, 240}));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt({50, 50, 50, 240}));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt({60, 60, 70, 240}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(RAYWHITE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt({32, 114, 168, 255}));

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt({80, 80, 100, 255}));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, ColorToInt({100, 100, 120, 255}));
    GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
}

int main(int argc, char *argv[])
{
    bool fpsDrawingEnabled = false;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--piece" && i + 1 < argc)
        {
            currentPieceSet = argv[++i];
        }
        else if (arg == "--board" && i + 1 < argc)
        {
            currentBoardSet = argv[++i];
        }
        else if (arg == "--drawFps")
        {
            fpsDrawingEnabled = true;
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: " << argv[0] << " [-piece PIECE_SET] [-board BOARD_THEME]\n";
            std::cout << "Options:\n";
            std::cout << "  --piece PIECE_SET    Set the piece set (e.g., 'classic', 'modern')\n";
            std::cout << "  --board BOARD_THEME  Set the board theme (e.g., 'classic', 'blue', 'forest')\n";
            std::cout << "  --drawFps            Draws the current FPS in the top left corner\n";
            std::cout << "  -h, --help          Show this help message\n";
            exit(0);
        }
    }

    pieceSets = discoverPieceSets();
    if (pieceSets.empty())
    {
        pieceSets.push_back("neo");
    }

    auto it = find(pieceSets.begin(), pieceSets.end(), currentPieceSet);
    if (it != pieceSets.end())
    {
        selectedPieceSetIndex = distance(pieceSets.begin(), it);
    }
    else
    {
        currentPieceSet = pieceSets[0];
        selectedPieceSetIndex = 0;
    }

    auto it2 = CHESS_THEMES.find(currentBoardSet);
    if (it2 != CHESS_THEMES.end())
    {
        LIGHT_SQUARE = CHESS_THEMES[currentBoardSet][0];
        DARK_SQUARE = CHESS_THEMES[currentBoardSet][1];
        HIGHLIGHT_COLOR = CHESS_THEMES[currentBoardSet][2];
    }
    else
    {
        currentBoardSet = "classic";
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_VSYNC_HINT);

    InitWindow(screenWidth, screenHeight, "Chess");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Image icon = LoadImage("icons/icon.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    Vector2 dpiScale = GetWindowScaleDPI();

    SetWindowMinSize(MIN_SCREEN_WIDTH * dpiScale.x, MIN_SCREEN_HEIGHT * dpiScale.y);

    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    int scaledWidth = screenWidth * dpiScale.x;
    int scaledHeight = screenHeight * dpiScale.y;

    SetWindowPosition((monitorWidth - scaledWidth) / 2, (monitorHeight - scaledHeight) / 2);

    SetExitKey(KEY_NULL);

    GuiLoadStyleDefault();

    int initialFontSize = max(14, min(25, screenHeight / 30));
    GuiSetStyle(DEFAULT, TEXT_SIZE, initialFontSize);

    SetGuiStyles();

    calculateDimensions();

    loadPieceTextures();

    GoToMainMenu();

    while (!WindowShouldClose())
    {
        handleWindowResize();

        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (currentScreen == ScreenState::FEN_INPUT)
            {
                TryToGoToMainMenu(false);
            }
            else if (currentScreen == ScreenState::CHESS_BOARD)
            {
                if (!(mateStatus > 0 && !mateStatusWindowClosed))
                {
                    TryToGoToMainMenu(false);
                }
            }
        }

        if (IsKeyPressed(KEY_RIGHT))
        {
            if (currentScreen == ScreenState::CHESS_BOARD)
            {
                ChangeMoveSelection(1);
            }
        }

        if (IsKeyPressed(KEY_LEFT))
        {
            if (currentScreen == ScreenState::CHESS_BOARD)
            {
                ChangeMoveSelection(-1);
            }
        }

        if (currentScreen == ScreenState::CHESS_BOARD)
        {
            clickDetection();
        }

        if (IsKeyPressed(KEY_R) && currentScreen == ScreenState::CHESS_BOARD)
        {
            boardFlipped = !boardFlipped;
        }

        BeginDrawing();
        Color bgColor = {20, 20, 25, 255};
        ClearBackground(bgColor);

        switch (currentScreen)
        {
        case ScreenState::MAIN_MENU:
            drawMainMenu();
            break;
        case ScreenState::CHESS_BOARD:
            drawChessBoardScreen();
            break;
        case ScreenState::FEN_INPUT:
            drawFENInput();
            break;
        }

        if (fpsDrawingEnabled)
        {
            DrawFPS(10, 10);
        }

        EndDrawing();
    }

    if (promotionInfo != nullptr)
    {
        delete promotionInfo;
        promotionInfo = nullptr;
    }
    unloadTextures();
    CloseWindow();

    return 0;
}
