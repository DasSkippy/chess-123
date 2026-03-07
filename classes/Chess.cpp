#include "Chess.h"
#include "Game.h"
#include "Bitboard.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    std::cout << "testing" << std::endl;

    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    auto moves = generateAllMoves();
    std::cout << "Generated moves: " << moves.size() << std::endl;  // should be 20 at start

    // _currentPlayer = WHITE;
    // _moves = generateAllMoves
    // startGame();
}

bool Chess::canPawnMove(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare *srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare *dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    int direction = (bit.gameTag() < 128) ? 1 : -1;
    int startRow = (bit.gameTag() < 128) ? 1 : 6;

    int srcCol = srcSquare->getColumn();
    int srcRow = srcSquare->getRow();
    int dstCol = dstSquare->getColumn();
    int dstRow = dstSquare->getRow();

    if (dstCol == srcCol && dstRow == srcRow + direction && dst.empty()) {
        return true;
    }

    if (dstCol == srcCol && dstRow == srcRow + 2 * direction && srcRow == startRow && dst.empty()) {
        ChessSquare *intermediateSquare = _grid->getSquare(srcCol, srcRow + direction);
        if (intermediateSquare && intermediateSquare->empty()) {
            return true;
        }
    }

    if (std::abs(dstCol - srcCol) == 1 && dstRow == srcRow + direction && !dst.empty() && !dst.bit()->friendly()) {
        return true;
    }

    return false;
}

bool Chess::canKnightMove(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare *srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare *dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    int srcCol = srcSquare->getColumn();
    int srcRow = srcSquare->getRow();
    int dstCol = dstSquare->getColumn();
    int dstRow = dstSquare->getRow();

    int colDiff = std::abs(dstCol - srcCol);
    int rowDiff = std::abs(dstRow - srcRow);

    if ((colDiff == 2 && rowDiff == 1) || (colDiff == 1 && rowDiff == 2)) {
        if (dst.empty() || !dst.bit()->friendly()) {
            return true;
        }
    }

    return false;
}

bool Chess::canKingMove(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare *srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare *dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    int srcCol = srcSquare->getColumn();
    int srcRow = srcSquare->getRow();
    int dstCol = dstSquare->getColumn();
    int dstRow = dstSquare->getRow();

    int colDiff = std::abs(dstCol - srcCol);
    int rowDiff = std::abs(dstRow - srcRow);

    if (colDiff <= 1 && rowDiff <= 1) {
        if (dst.empty() || !dst.bit()->friendly()) {
            return true;
        }
    }

    return false;
}

bool Chess::canRookMove(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare *srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare *dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    int srcCol = srcSquare->getColumn();
    int srcRow = srcSquare->getRow();
    int dstCol = dstSquare->getColumn();
    int dstRow = dstSquare->getRow();

    if (srcCol == dstCol || srcRow == dstRow) {
        if (dst.empty() || !dst.bit()->friendly()) {
            if (srcCol == dstCol) {
                int step = (dstRow > srcRow) ? 1 : -1;
                for (int r = srcRow + step; r != dstRow; r += step) {
                    if (!_grid->getSquare(srcCol, r)->empty()) {
                        return false;
                    }
                }
            } else {
                int step = (dstCol > srcCol) ? 1 : -1;
                for (int c = srcCol + step; c != dstCol; c += step) {
                    if (!_grid->getSquare(c, srcRow)->empty()) {
                        return false;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool Chess::canBishopMove(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare *srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare *dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    int srcCol = srcSquare->getColumn();
    int srcRow = srcSquare->getRow();
    int dstCol = dstSquare->getColumn();
    int dstRow = dstSquare->getRow();

    int colDiff = std::abs(dstCol - srcCol);
    int rowDiff = std::abs(dstRow - srcRow);

    if (colDiff == rowDiff) {
        if (dst.empty() || !dst.bit()->friendly()) {
            int colStep = (dstCol > srcCol) ? 1 : -1;
            int rowStep = (dstRow > srcRow) ? 1 : -1;
            for (int c = srcCol + colStep, r = srcRow + rowStep; c != dstCol; c += colStep, r += rowStep) {
                if (!_grid->getSquare(c, r)->empty()) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool Chess::canQueenMove(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Queen can move like a rook or a bishop
    return canRookMove(bit, src, dst) || canBishopMove(bit, src, dst);
}

std::vector<Move> Chess::generateAllMoves()
{
    std::vector<Move> moves;

    for (int x = 0; x < 8; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            BitHolder* src = _grid->getSquare(x, y);
            if (!src || src->empty()) continue;

            Bit* bit = src->bit();

            if (!canBitMoveFrom(*bit, *src)) continue;

            for (int dx = 0; dx < 8; dx++)
            {
                for (int dy = 0; dy < 8; dy++)
                {
                    BitHolder* dst = _grid->getSquare(dx, dy);
                    if (!dst) continue;

                    if (canBitMoveFromTo(*bit, *src, *dst))
                    {
                        moves.emplace_back(src, dst, bit);
                    }
                }
            }
        }
    }

    return moves;
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    //Clear the board
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->setBit(nullptr);
    });

    int row = 7;
    int col = 0;
    for (char c : fen) {
        if (c == '/') {
            row--;
            col = 0;
        } else if (isdigit(c)) {
            col += c - '0';
        } else {
            ChessPiece piece = NoPiece;
            switch (tolower(c)) {
                case 'p': piece = Pawn; break;
                case 'n': piece = Knight; break;
                case 'b': piece = Bishop; break;
                case 'r': piece = Rook; break;
                case 'q': piece = Queen; break;
                case 'k': piece = King; break;
            }
            Bit* bit = PieceForPlayer(isupper(c) ? 0 : 1, piece);
            ChessSquare* square = _grid->getSquare(col, row);
            bit->setPosition(square->getPosition());
            bit->setParent(square);
            bit->setGameTag(isupper(c) ? piece : (piece + 128));
            square->setBit(bit);
            col++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    int pieceType = bit.gameTag() & 127; // mask out color
    if (pieceType == Pawn) {
        return canPawnMove(bit, src, dst);
    }
    if (pieceType == Knight) {
        return canKnightMove(bit, src, dst);
    }
    if (pieceType == King) {
        return canKingMove(bit, src, dst);
    }
    if (pieceType == Rook) {
        return canRookMove(bit, src, dst);
    }
    if (pieceType == Bishop) {
        return canBishopMove(bit, src, dst);
    }
    if (pieceType == Queen) {
        return canQueenMove(bit, src, dst);
    }

    // TODO: implement other pieces
    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}