#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <iostream>

enum ChessPiece
{
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class BitBoard
{
    public:
        BitBoard()
            : _data(0) { }
        BitBoard(uint64_t data)
            : _data(data) { }
        
        uint64_t data() const { return _data; }
        void setData(uint64_t data) { _data = data; }

        template <typename Func>
        void forEachSetBit(Func func) const {
            if (_data != 0) {
                uint64_t tempData = _data;
                while(tempData) {
                    int index = bitScanForward(tempData);
                    func(index);
                    tempData &= tempData - 1; // Clear the least significant bit
                }
            }
        }

        BitBoard& operator|=(const uint64_t other) {
            _data |= other;
            return *this;
        }
        BitBoard operator<<(const int shift) const {
            return BitBoard(_data << shift);
        }
        BitBoard operator>>(const int shift) const {
            return BitBoard(_data >> shift);
        }

        void PrintBitboard() {
            std::cout << "\n a b c d e f g h\n";
            for (int rank = 7; rank >= 0; --rank) {
                std::cout << rank + 1 << " ";
                for (int file = 0; file < 8; ++file) {
                    int square = rank * 8 + file;
                    if (_data & (1ULL << square)) {
                        std::cout << "X ";
                    } else {
                        std::cout << ". ";
                    }
                }
                std::cout << (rank + 1) << "\n";
                std::cout << std::flush;
            }
        }

        inline int bitScanForward(uint64_t bb) const {
#if defined(_MSC_VER) && !defined(__clang__)
            unsigned long index;
            _BitScanForward64(&index, bb);
            return index;
#else
            return __builtin_ffsll(bb) - 1;
#endif
        };

    private:
        uint64_t _data;
};

struct BitMove {
    int from;
    int to;
    int piece;

    BitMove(int from, int to, int piece)
        : from(from), to(to), piece(piece) { }
    
    BitMove() : from(0), to(0), piece(NoPiece) { }

    bool operator==(const BitMove& other) const {
        return from == other.from && to == other.to && piece == other.piece;
    }
};