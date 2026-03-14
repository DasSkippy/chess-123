#include "Chess.h"
#include "Game.h"
#include "Bitboard.h"
#include "../Application.h"
#include <limits>
#include <cmath>

namespace {
// Evaluation is small (material count), so this is plenty for alpha-beta bounds.
constexpr int kSearchInf = 1000000;

inline int bitColor(const Bit* bit)
{
    // gameTag uses bit 7 (128) as the "black" flag in this project.
    return (bit && (bit->gameTag() & 128)) ? BLACK : WHITE;
}

inline bool isColor(const Bit* bit, int color)
{
    return bit && bitColor(bit) == color;
}

inline bool isOpponent(const Bit* bit, int color)
{
    return bit && bitColor(bit) == -color;
}

// BitHolder::setBit deletes any existing bit. For search we need to be able to
// capture and then undo without freeing the captured piece.
Bit* makeMoveForSearch(const Move& move)
{
    BitHolder* src = move.src;
    BitHolder* dst = move.dst;
    Bit* piece = move.piece;

    if (!src || !dst || !piece) return nullptr;

    Bit* captured = dst->bit();
    if (captured) {
        // Detach so dst->setBit(piece) won't delete it.
        captured->setParent(nullptr);
    }

    dst->setBit(piece);
    src->setBit(nullptr);

    piece->setParent(dst);
    piece->setPosition(dst->getPosition());

    return captured;
}

void undoMoveForSearch(const Move& move, Bit* captured)
{
    BitHolder* src = move.src;
    BitHolder* dst = move.dst;
    Bit* piece = move.piece;

    if (!src || !dst || !piece) return;

    // Order matters: setting src first updates piece->parent so dst->setBit
    // won't delete the moving piece.
    src->setBit(piece);
    dst->setBit(captured);

    piece->setParent(src);
    piece->setPosition(src->getPosition());

    if (captured) {
        captured->setPosition(dst->getPosition());
    }
}

inline bool onBoard(int x, int y)
{
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

static void addRayMoves(std::vector<Move>& moves, Grid* grid, ChessSquare* src, Bit* piece, int color, int dx, int dy)
{
    const int sx = src->getColumn();
    const int sy = src->getRow();

    for (int x = sx + dx, y = sy + dy; onBoard(x, y); x += dx, y += dy) {
        ChessSquare* dst = grid->getSquare(x, y);
        Bit* dstBit = dst->bit();
        if (!dstBit) {
            moves.emplace_back(src, dst, piece);
            continue;
        }
        if (isOpponent(dstBit, color)) {
            moves.emplace_back(src, dst, piece);
        }
        break;
    }
}

static std::vector<Move> generateAllMovesForColor(Grid* grid, int color)
{
    std::vector<Move> moves;
    moves.reserve(64);

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            ChessSquare* src = grid->getSquare(x, y);
            Bit* piece = src->bit();
            if (!isColor(piece, color)) continue;

            const int pieceType = piece->gameTag() & 127;
            switch (pieceType) {
                case Pawn: {
                    const int dir = (color == WHITE) ? 1 : -1;
                    const int startRow = (color == WHITE) ? 1 : 6;

                    const int oneY = y + dir;
                    if (onBoard(x, oneY)) {
                        ChessSquare* one = grid->getSquare(x, oneY);
                        if (one->empty()) {
                            moves.emplace_back(src, one, piece);

                            const int twoY = y + 2 * dir;
                            if (y == startRow && onBoard(x, twoY)) {
                                ChessSquare* two = grid->getSquare(x, twoY);
                                if (two->empty()) {
                                    moves.emplace_back(src, two, piece);
                                }
                            }
                        }
                    }

                    const int capY = y + dir;
                    if (onBoard(x - 1, capY)) {
                        ChessSquare* dst = grid->getSquare(x - 1, capY);
                        if (isOpponent(dst->bit(), color)) {
                            moves.emplace_back(src, dst, piece);
                        }
                    }
                    if (onBoard(x + 1, capY)) {
                        ChessSquare* dst = grid->getSquare(x + 1, capY);
                        if (isOpponent(dst->bit(), color)) {
                            moves.emplace_back(src, dst, piece);
                        }
                    }
                    break;
                }
                case Knight: {
                    static const int kOffsets[8][2] = {
                        { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 },
                        { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 }
                    };
                    for (const auto& off : kOffsets) {
                        const int nx = x + off[0];
                        const int ny = y + off[1];
                        if (!onBoard(nx, ny)) continue;
                        ChessSquare* dst = grid->getSquare(nx, ny);
                        Bit* dstBit = dst->bit();
                        if (!dstBit || isOpponent(dstBit, color)) {
                            moves.emplace_back(src, dst, piece);
                        }
                    }
                    break;
                }
                case King: {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            const int nx = x + dx;
                            const int ny = y + dy;
                            if (!onBoard(nx, ny)) continue;
                            ChessSquare* dst = grid->getSquare(nx, ny);
                            Bit* dstBit = dst->bit();
                            if (!dstBit || isOpponent(dstBit, color)) {
                                moves.emplace_back(src, dst, piece);
                            }
                        }
                    }
                    break;
                }
                case Rook: {
                    addRayMoves(moves, grid, src, piece, color, 1, 0);
                    addRayMoves(moves, grid, src, piece, color, -1, 0);
                    addRayMoves(moves, grid, src, piece, color, 0, 1);
                    addRayMoves(moves, grid, src, piece, color, 0, -1);
                    break;
                }
                case Bishop: {
                    addRayMoves(moves, grid, src, piece, color, 1, 1);
                    addRayMoves(moves, grid, src, piece, color, 1, -1);
                    addRayMoves(moves, grid, src, piece, color, -1, 1);
                    addRayMoves(moves, grid, src, piece, color, -1, -1);
                    break;
                }
                case Queen: {
                    addRayMoves(moves, grid, src, piece, color, 1, 0);
                    addRayMoves(moves, grid, src, piece, color, -1, 0);
                    addRayMoves(moves, grid, src, piece, color, 0, 1);
                    addRayMoves(moves, grid, src, piece, color, 0, -1);
                    addRayMoves(moves, grid, src, piece, color, 1, 1);
                    addRayMoves(moves, grid, src, piece, color, 1, -1);
                    addRayMoves(moves, grid, src, piece, color, -1, 1);
                    addRayMoves(moves, grid, src, piece, color, -1, -1);
                    break;
                }
                default:
                    break;
            }
        }
    }

    return moves;
}
} // namespace

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

void Chess::endTurn()
{
    // End-of-move bookkeeping: detect winner/draw before advancing turns or triggering AI.
    if (_winner) {
        return;
    }

    if (Player* winner = checkForWinner()) {
        _winner = winner;
        const char* side = (winner->playerNumber() == 0) ? "White" : "Black";
        std::cout << "Game over: " << side << " wins";
        if (winner->isAIPlayer()) std::cout << " (AI)";
        std::cout << std::endl;
        // Notify the app/UI, but keep the final board state visible.
        ClassGame::EndOfTurn();
        return;
    }

    if (checkForDraw()) {
        std::cout << "Game over: draw" << std::endl;
        // Notify the app/UI, but keep the final board state visible.
        ClassGame::EndOfTurn();
        return;
    }

    Game::endTurn();
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

    setAIPlayer(1);

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
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (square->bit() && square->bit()->friendly()) {
            Bit& bit = *square->bit();
            _grid->forEachSquare([&](ChessSquare* dstSquare, int dstX, int dstY) {
                if (canBitMoveFromTo(bit, *square, *dstSquare)) {
                    moves.push_back(Move(square, dstSquare, &bit));
                }
            });
        }
    });
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
    bool whiteKing = false;
    bool blackKing = false;

    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (!bit) return;
        if ((bit->gameTag() & 127) != King) return;

        if (bit->gameTag() & 128) {
            blackKing = true;
        } else {
            whiteKing = true;
        }
    });

    // If a king is missing, the other side wins. (No check/checkmate rules yet.)
    if (!whiteKing && blackKing) return getPlayerAt(1);
    if (!blackKing && whiteKing) return getPlayerAt(0);

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

void Chess::makeMove(const Move& move)
{
    BitHolder* src = move.src;
    BitHolder* dst = move.dst;
    Bit* piece = move.piece;

    if (!piece)
        return;

    dst->setBit(piece);
    src->setBit(nullptr);

    piece->setParent(dst);
    piece->setPosition(dst->getPosition());
}

void Chess::undoMove(const Move& move, Bit* captured)
{
    BitHolder* src = move.src;
    BitHolder* dst = move.dst;
    Bit* piece = move.piece;

    if (!piece)
        return;

    src->setBit(piece);
    dst->setBit(captured);

    piece->setParent(src);
    piece->setPosition(src->getPosition());
}

int Chess::evaluateBoard()
{
    int score = 0;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (square->bit()) {
            int pieceValue = 0;
            int pieceType = square->bit()->gameTag() & 127; // mask out color
            switch (pieceType) {
                case Pawn: pieceValue = 10; break;
                case Knight: pieceValue = 30; break;
                case Bishop: pieceValue = 30; break;
                case Rook: pieceValue = 50; break;
                case Queen: pieceValue = 90; break;
                case King: pieceValue = 900; break;
            }
            if (square->bit()->gameTag() < 128) {
                score += pieceValue; // white pieces are positive
            } else {
                score -= pieceValue; // black pieces are negative
            }
        }
    });
    return score;
}

int Chess::negamax(int depth, int alpha, int beta, int color)
{
    if (depth == 0) {
        return color * evaluateBoard();
    }

    auto moves = generateAllMovesForColor(_grid, color);
    if (moves.empty()) {
        return color * evaluateBoard();
    }

    // Avoid UB from negating INT_MIN; keep bounds within +/- kSearchInf.
    if (alpha < -kSearchInf) alpha = -kSearchInf;
    if (beta > kSearchInf) beta = kSearchInf;

    int maxEval = -kSearchInf;

    for (const auto& move : moves)
    {
        Bit* captured = makeMoveForSearch(move);

        int eval = -negamax(depth - 1, -beta, -alpha, -color);

        undoMoveForSearch(move, captured);

        maxEval = std::max(maxEval, eval);
        alpha = std::max(alpha, eval);
        if (alpha >= beta) {
            break; // beta cut-off
        }
    }

    return maxEval;
    
}

Move Chess::findBestMove(int depth, int color)
{
    Move bestMove;
    int maxEval = -kSearchInf;

    auto moves = generateAllMovesForColor(_grid, color);

    for (const auto& move : moves)
    {
        Bit* captured = makeMoveForSearch(move);

        int eval = -negamax(depth - 1, -kSearchInf, kSearchInf, -color);

        undoMoveForSearch(move, captured);

        if (eval > maxEval) {
            maxEval = eval;
            bestMove = move;
        }
    }

    return bestMove;
}

void Chess::updateAI()
{
    if (_winner) return;
    if (!gameHasAI()) return;

    if (!getCurrentPlayer()->isAIPlayer()) return;

    int color = getCurrentPlayer()->playerNumber() == 0 ? 1 : -1;

    Move best = findBestMove(3, color);

    if (!best.src || !best.dst || !best.piece)
        return;

    makeMove(best);

    endTurn();
}

