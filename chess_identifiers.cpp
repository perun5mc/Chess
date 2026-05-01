#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cctype>
#include <iostream>

using namespace std;

unordered_map<string, string> fenToPieceMap(const string &fen)
{
    unordered_map<string, string> pieceMap;

    // Extract just the board position from FEN (first field)
    string boardPart = fen.substr(0, fen.find(' '));

    // Initialize counters for each piece type
    int whitePawnCount = 0;
    int whiteKnightCount = 0;
    int whiteBishopCount = 0;
    int whiteRookCount = 0;
    int whiteQueenCount = 0;
    int whiteKingCount = 0;

    int blackPawnCount = 0;
    int blackKnightCount = 0;
    int blackBishopCount = 0;
    int blackRookCount = 0;
    int blackQueenCount = 0;
    int blackKingCount = 0;

    // Parse the FEN board
    int row = 8; // Start at row 8 (top in FEN)
    int col = 0; // Column a = 0, b = 1, etc.

    for (char c : boardPart)
    {
        if (c == '/')
        {
            row--;
            col = 0;
            continue;
        }

        if (isdigit(c))
        {
            col += (c - '0');
            continue;
        }

        // Calculate square name (e.g., "e4")
        string square = string(1, 'a' + col) + to_string(row);

        // Assign based on piece type
        switch (c)
        {
        case 'P':
            whitePawnCount++;
            pieceMap["white_pawn_" + to_string(whitePawnCount)] = square;
            break;
        case 'N':
            whiteKnightCount++;
            pieceMap["white_knight_" + to_string(whiteKnightCount)] = square;
            break;
        case 'B':
            whiteBishopCount++;
            pieceMap["white_bishop_" + to_string(whiteBishopCount)] = square;
            break;
        case 'R':
            whiteRookCount++;
            pieceMap["white_rook_" + to_string(whiteRookCount)] = square;
            break;
        case 'Q':
            whiteQueenCount++;
            pieceMap["white_queen_" + to_string(whiteQueenCount)] = square;
            break;
        case 'K':
            whiteKingCount++;
            pieceMap["white_king_" + to_string(whiteKingCount)] = square;
            break;
        case 'p':
            blackPawnCount++;
            pieceMap["black_pawn_" + to_string(blackPawnCount)] = square;
            break;
        case 'n':
            blackKnightCount++;
            pieceMap["black_knight_" + to_string(blackKnightCount)] = square;
            break;
        case 'b':
            blackBishopCount++;
            pieceMap["black_bishop_" + to_string(blackBishopCount)] = square;
            break;
        case 'r':
            blackRookCount++;
            pieceMap["black_rook_" + to_string(blackRookCount)] = square;
            break;
        case 'q':
            blackQueenCount++;
            pieceMap["black_queen_" + to_string(blackQueenCount)] = square;
            break;
        case 'k':
            blackKingCount++;
            pieceMap["black_king_" + to_string(blackKingCount)] = square;
            break;
        }

        col++;
    }

    return pieceMap;
}

unordered_map<string, string> updateFenToPieceMap(
    const unordered_map<string, string> &pieceMap,
    const string &uciMove)
{

    unordered_map<string, string> newMap = pieceMap;

    if (uciMove.length() < 4)
        return newMap; // Invalid UCI move

    // Parse the UCI move
    string fromSquare = uciMove.substr(0, 2);
    string toSquare = uciMove.substr(2, 2);
    char promotionPiece = (uciMove.length() >= 5) ? uciMove[4] : '\0';

    // Find which piece is at the from square
    string movingPieceKey;
    string movingPieceType;
    string movingPieceColor;

    for (const auto &[key, square] : pieceMap)
    {
        if (square == fromSquare)
        {
            movingPieceKey = key;

            // Extract piece type and color from the key (e.g., "white_pawn1" -> "pawn", "white")
            size_t underscorePos = key.find('_');
            size_t secondUnderscorePos = key.find('_', underscorePos + 1);

            if (underscorePos != string::npos && secondUnderscorePos != string::npos)
            {
                movingPieceColor = key.substr(0, underscorePos);
                movingPieceType = key.substr(underscorePos + 1, secondUnderscorePos - underscorePos - 1);
            }
            break;
        }
    }

    if (movingPieceKey.empty())
        return newMap; // No piece found at from square

    // Check if this is a capture (piece at destination square)
    string capturedPieceKey;
    for (const auto &[key, square] : pieceMap)
    {
        if (square == toSquare)
        {
            capturedPieceKey = key;
            break;
        }
    }

    // Handle capture
    if (!capturedPieceKey.empty())
    {
        newMap[capturedPieceKey] = "-";
    }

    // Handle special cases
    bool kingsideCastle = false;
    bool queensideCastle = false;
    if (movingPieceType == "king")
    {
        // Check if this might be castling (king moved)
        bool isCastling = false;
        string rookFromSquare;
        string rookToSquare;
        string rookKey;

        // Case 1: King moved 2 squares (traditional castling)
        if (abs(toSquare[0] - fromSquare[0]) == 2)
        {
            isCastling = true;

            // Determine which rook to move based on direction
            if (toSquare[0] > fromSquare[0])
            { // Kingside (right)
                // Find the rook on the same rank, to the right of the king's original square
                for (const auto &[key, square] : pieceMap)
                {
                    if (square != "-" && square[1] == fromSquare[1] && square[0] > fromSquare[0])
                    {
                        // Check if it's a rook of the same color
                        if (key.find(movingPieceColor + "_rook") == 0)
                        {
                            rookKey = key;
                            rookFromSquare = square;
                            break;
                        }
                    }
                }
                // Kingside: rook moves to f1/f8
                rookToSquare = string(1, 'f') + fromSquare[1];
                kingsideCastle = true;
            }
            else
            { // Queenside (left)
                // Find the rook on the same rank, to the left of the king's original square
                for (const auto &[key, square] : pieceMap)
                {
                    if (square != "-" && square[1] == fromSquare[1] && square[0] < fromSquare[0])
                    {
                        // Check if it's a rook of the same color
                        if (key.find(movingPieceColor + "_rook") == 0)
                        {
                            rookKey = key;
                            rookFromSquare = square;
                            break;
                        }
                    }
                }
                // Queenside: rook moves to d1/d8
                rookToSquare = string(1, 'd') + fromSquare[1];
                queensideCastle = true;
            }
        }
        // Case 2: King moved to a square occupied by a rook (Fischer castling)
        else
        {
            // Check if there's a rook at the destination square
            for (const auto &[key, square] : pieceMap)
            {
                if (square == toSquare)
                {
                    if (key.find(movingPieceColor + "_rook") == 0)
                    {
                        isCastling = true;
                        rookKey = key;
                        rookFromSquare = square;

                        // Determine where the rook should go
                        if (toSquare[0] > fromSquare[0])
                        { // Moved right (kingside)
                            rookToSquare = string(1, 'f') + fromSquare[1];
                            kingsideCastle = true;
                        }
                        else
                        { // Moved left (queenside)
                            rookToSquare = string(1, 'd') + fromSquare[1];
                            queensideCastle = true;
                        }

                        // Remove the rook from the destination since king will occupy it
                        newMap[rookKey] = "-";
                        break;
                    }
                }
            }
        }

        if (isCastling && !rookKey.empty())
        {
            newMap[rookKey] = rookToSquare;
        }
    }
    else if (movingPieceType == "pawn" && uciMove.length() >= 5)
    {
        // Promotion
        string newPieceColor = movingPieceColor;
        string newPieceType;

        cout << promotionPiece << endl;

        // Determine new piece type from promotion character
        switch (tolower(promotionPiece))
        {
        case 'n':
            newPieceType = "knight";
            break;
        case 'b':
            newPieceType = "bishop";
            break;
        case 'r':
            newPieceType = "rook";
            break;
        case 'q':
            newPieceType = "queen";
            break;
        default:
            newPieceType = "queen"; // Default to queen if invalid
        }

        // Find an available ID for the new piece
        int id = 1;
        string newPieceKey;
        while (true)
        {
            newPieceKey = newPieceColor + "_" + newPieceType + to_string(id);
            if (newMap.find(newPieceKey) == newMap.end() || newMap[newPieceKey] == "-")
            {
                break;
            }
            id++;
        }

        // Set the pawn square to empty and place the new piece
        newMap[movingPieceKey] = "-";
        newMap[newPieceKey] = toSquare;

        return newMap;
    }
    else if (movingPieceType == "pawn" && fromSquare[0] != toSquare[0] && capturedPieceKey.empty())
    {
        string enPassantSquare = string(1, toSquare[0]) + fromSquare[1];

        // Find and remove the captured pawn
        for (auto &[key, square] : newMap)
        {
            if (square == enPassantSquare)
            {
                newMap[key] = "-";
                break;
            }
        }
    }

    if (!queensideCastle && !kingsideCastle)
    {
        newMap[movingPieceKey] = toSquare;
    }
    else
    {
        if (movingPieceColor == "white")
        {
            if (kingsideCastle)
            {
                newMap[movingPieceKey] = "g1";
            }
            else
            {
                newMap[movingPieceKey] = "c1";
            }
        }
        else
        {
            if (kingsideCastle)
            {
                newMap[movingPieceKey] = "g8";
            }
            else
            {
                newMap[movingPieceKey] = "c8";
            }
        }
    }

    return newMap;
}

pair<int, int> squareToCoordinates(const string &square)
{
    if (square == "-")
        return {-1, -1};
    int file = square[0] - 'a';
    int rank = square[1] - '1';
    return {file, rank};
}

int calculateDistance(const string &square1, const string &square2)
{
    if (square1 == "-" || square2 == "-")
        return numeric_limits<int>::max();

    auto [x1, y1] = squareToCoordinates(square1);
    auto [x2, y2] = squareToCoordinates(square2);

    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);

    cout << "DISTANCE: " << max(dx, dy) * 2 - min(dx, dy) << endl;

    return max(dx, dy) * 2 - min(dx, dy);
}

std::unordered_map<std::string, std::string> fillSpawnedPieces(
    const std::unordered_map<std::string, std::string> &currentMap,
    const std::unordered_map<std::string, std::string> &newMap)
{

    std::unordered_map<std::string, std::string> filledCurrentMap = currentMap;

    // Step 1: Identify pieces that are moving (exist in both maps but at different positions)
    std::vector<std::string> movingPieces;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> movingPiecesByType;

    for (const auto &[pieceId, currentPos] : currentMap)
    {
        if (currentPos == "-")
            continue;

        auto it = newMap.find(pieceId);
        if (it != newMap.end() && it->second != "-" && it->second != currentPos)
        {
            movingPieces.push_back(pieceId);

            // Extract piece type from ID (e.g., "white_pawn1" -> "white_pawn")
            size_t lastUnderscorePos = pieceId.find_last_of('_');
            std::string pieceType = pieceId.substr(0, lastUnderscorePos);

            // Store both the piece ID and its CURRENT position (not destination!)
            movingPiecesByType[pieceType].push_back({pieceId, currentPos});
        }
    }

    // Step 2: Find spawned pieces (in newMap but not in currentMap)
    std::vector<std::pair<std::string, std::string>> spawnedPieces;

    for (const auto &[pieceId, newPos] : newMap)
    {
        if (newPos == "-")
            continue;

        auto it = currentMap.find(pieceId);
        if (it == currentMap.end() || it->second == "-")
        {
            spawnedPieces.push_back({pieceId, newPos});
        }
    }

    // Step 3: For each spawned piece, find the closest moving piece of the same type
    for (const auto &[spawnedId, spawnedDest] : spawnedPieces)
    {
        // Extract piece type from spawned ID
        size_t lastUnderscorePos = spawnedId.find_last_of('_');
        std::string pieceType = spawnedId.substr(0, lastUnderscorePos);

        // Find moving pieces of the same type
        auto it = movingPiecesByType.find(pieceType);

        if (it != movingPiecesByType.end() && !it->second.empty())
        {
            // Find the closest moving piece of this type
            std::string closestMovingPiece;
            std::string closestMovingStart;
            int minDistance = std::numeric_limits<int>::max();

            for (const auto &[movingId, movingStart] : it->second)
            {
                // Calculate distance between moving piece's START position and spawned piece's DESTINATION
                int distance = calculateDistance(movingStart, spawnedDest);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestMovingPiece = movingId;
                    closestMovingStart = movingStart;
                }
            }

            if (!closestMovingPiece.empty())
            {
                // Set spawned piece's current position to the moving piece's START position
                filledCurrentMap[spawnedId] = closestMovingStart;
            }
            else
            {
                // Fallback: if no moving pieces of this type, spawn at destination
                filledCurrentMap[spawnedId] = spawnedDest;
            }
        }
        else
        {
            // No moving pieces of this type, spawn at destination
            filledCurrentMap[spawnedId] = spawnedDest;
        }
    }

    return filledCurrentMap;
}