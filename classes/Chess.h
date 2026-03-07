#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;
constexpr int WHITE = 1;
constexpr int BLACK = -1;

struct Move
{
    BitHolder* src;
    BitHolder* dst;
    Bit* piece;

    Move(BitHolder* s, BitHolder* d, Bit* p) : src(s), dst(d), piece(p) {}
};

enum AllBitBoards
{
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    WHITE_ALL_PIECES,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    bool canPawnMove(Bit &bit, BitHolder &src, BitHolder &dst);
    bool canKnightMove(Bit &bit, BitHolder &src, BitHolder &dst);
    bool canKingMove(Bit &bit, BitHolder &src, BitHolder &dst);
    bool canRookMove(Bit &bit, BitHolder &src, BitHolder &dst);
    bool canBishopMove(Bit &bit, BitHolder &src, BitHolder &dst);
    bool canQueenMove(Bit &bit, BitHolder &src, BitHolder &dst);

    std::vector<Move> generateAllMoves();

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    Grid* _grid;
};