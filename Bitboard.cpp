#include <bits/stdc++.h>
#include <optional>
#include <vector>
#include "Bitboard.h"

std::string to_chess_string(int square) {
    std::string res;
    int line = 8 - (square / 8);
    int column = square % 8;
    res.push_back((char)('a' + column));
    res.push_back((char)('0' + line));
    return res;
}

int parseSquare(std::optional<std::string> square) {
    int line = (char)square.value()[1] - '0';
    int column = (char)square.value()[0] - 'a';
    return (8 - line) * 8 + column;
} 

std::vector<int> get_filled_squares(Bitboard B) {
    std::vector<int> res;
    for (int sq = 0; sq < 64; sq++) {
        if (get_bit(B, sq)) {res.push_back(sq);}
    }
    return res;
}

int scan_bit(Bitboard B) {
    int bit;
    for (bit = 1; B != 1ULL << bit; bit++);
    return bit;
}

int get_bit(Bitboard B, int n) {
    if ((B & (1ULL << n)) == 0)
        return 0;
    return 1;
}

void set_bit(Bitboard *B, int n) {
    *B |= (1ULL << n);
}

void reset_bit(Bitboard *B, int n) {
    unsigned long long mask = 0xFFFFFFFFFFFFFFFFULL;
    mask -= (1ULL << n);
    *B &= mask;
}

void print_BTB (Bitboard B) {
    printf ("\n");
    for (int line = 0; line < 8; ++line) {
        printf ("%d ", 8 - line);
        for (int square = 0; square < 8; ++square) {
            int bit = get_bit (B, line * 8 + square);
            printf(" %d",bit); 
        }
        printf ("\n");
    }
    printf("\n   A B C D E F G H\n");
}