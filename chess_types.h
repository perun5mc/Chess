#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

struct CastlingRights
{
    bool canCastleKingside = false;
    bool canCastleQueenside = false;
    int kingStartCol = -1;
    int rookKStartCol = -1;
    int rookQStartCol = -1;
};

#endif