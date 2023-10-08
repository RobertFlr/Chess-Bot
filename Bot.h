#ifndef BOT_H
#define BOT_H
#include <bits/stdc++.h>
#include <chrono>
#include <vector>
#include <stack>

#include "Move.h"
#include "PlaySide.h"
#include "Bitboard.h"

class Bot {

 private:
  static const std::string BOT_NAME;

 public:
  Bitboard pieces[2][6]; 
  Bitboard general_board[2];

  struct MoveHistory {
    int from, to;
    Piece moved, captured, replacement;
  };
  std::stack<MoveHistory> moveStack = {};

  struct Conditions {
    bool castle_cond_ks_white = true;
    bool castle_cond_qs_white = true;
    bool castle_cond_ks_black = true;
    bool castle_cond_qs_black = true;
    bool can_castle = true;
  };
  Conditions conds;

  /* Main Engine Functions */
  Bot();
  
  Move* calculateNextMove();

  void recordMove(Move* move, PlaySide sideToMove);
  
  std::vector<std::pair<int, Move*>> getMovePool(PlaySide side);

  /* creates a moves stack */
  static MoveHistory create(int from, int to, Piece moved, Piece captured, Piece replacement) {
    MoveHistory latest;
    latest.to = to;
    latest.from = from;
    latest.moved = moved;
    latest.captured = captured;
    latest.replacement = replacement;
    return latest;
  }
  /* returns opposite side */
  PlaySide opposite(PlaySide side) {
    return (side == WHITE) ? BLACK : WHITE;
  }


/* Board Evaluation */
/*__________________*/

  /* checks if the basic move can be done */
  bool can_move(int from, int to, PlaySide side);

  /* checks if castling is legal from every perspective */
  bool is_legal_castle (int from, int to, PlaySide side);

  /* checks high level rules for normal moves */
  bool is_legal(int from, int to, PlaySide side);

  /* checks if move is has a castle input, doesn't have to be legal */
  bool is_castle(int from, int to, PlaySide side);

  /* checks if move is has en passant input, doesn't have to be legal */
  bool is_enpassant(int from, int to, PlaySide side);

  /* checks if a move has promotion input */
  bool is_promotion(int from, int to, PlaySide side);

  /* chess specific helper functions */
  bool is_check(PlaySide side);

  /* get specific piece type */
  Piece get_piece(int square, PlaySide side);


/* Board Transformation */
/*______________________*/

  /* copy the pamarameters to the main board */
  void copy_board(Bitboard(*src_pieces)[6], Bitboard* src_general_board);

  /* backs up the main board into the function's parameteres */
  void backup_board(Bitboard(*old_pieces)[6], Bitboard* old_general_board);

  /* backs up the main board's conditions */
  void backup_conds(Conditions* backup_conds);

  /* update the conditions based on the parameters */
  void copy_conds(Conditions src_conds);

  /* updates the castling requirements after each move */
  void update_condtions(Piece moved, int to, int from, PlaySide sideToMove);

  /* joins the individual single piece boards */
  void update_general();

  /* print board in human readable format */
  void printBoardHR();
  
/* Board Evaluation */
/*__________________*/

  /* performs minimax search */
  int minimax(PlaySide side, int depth, int alpha, int beta);

  /* get evaluation score */
  int evaluate(PlaySide side);

  /* sum of all values */
  int count_material(PlaySide side);

  /* sum of all piece type values */
  int count(int piece, PlaySide side);

  /* get value of each piece */
  int getPieceValue(Piece p);

  static std::string getBotName();
};
#endif
