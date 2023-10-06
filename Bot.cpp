#include <bits/stdc++.h>
#include <cstdlib>
#include "Bot.h"

const std::string Bot::BOT_NAME = "IPlayChessLikeZuckerberg";
extern PlaySide getEngineSide();

void Bot::recordMove(Move* move, PlaySide sideToMove) {
  /* You might find it useful to also separately
   * record last move in another custom field */
  /* get source and destination in btb square formal */
  int from = parseSquare(move->getSource());
  int to = parseSquare(move->getDestination());
  Piece replacement;
  if (move->isPromotion()) {
    replacement = move->getReplacement().value();
  } else { replacement = EMPTY; }
  Piece capture = get_piece(to, opposite(sideToMove));
  Piece moved = get_piece(from, sideToMove);
  /* promotion case */
  if (move->isPromotion()) {
    /* move piece */
    reset_bit(&(pieces[sideToMove][moved]), from);
    set_bit(&(pieces[sideToMove][replacement]), to);
    if (capture != EMPTY) {
      reset_bit(&(pieces[opposite(sideToMove)][capture]), to);
    }
    /* replace pawn */
    reset_bit(&(pieces[sideToMove][moved]), to);
  /* castling case */
  } else if (is_castle(from, to, sideToMove)) {
    /* move the king */
    reset_bit(&(pieces[sideToMove][KING]), from);
    set_bit(&(pieces[sideToMove][KING]), to);
    /* replace the rook */
    if (to < from) {
      /* queenside */
      reset_bit (&(pieces[sideToMove][ROOK]), from - 4);
      set_bit (&(pieces[sideToMove][ROOK]), from - 1);
    } else {
      /* kingside */
      reset_bit (&(pieces[sideToMove][ROOK]), from + 3);
      set_bit (&(pieces[sideToMove][ROOK]), from + 1);
    }
  /* en passant case */
  } else if (is_enpassant(from, to, sideToMove)) {
    /* remove captured pawn */
    if (to < from) {reset_bit(&(pieces[opposite(sideToMove)][PAWN]), to + 8);}
    if (to > from) {reset_bit(&(pieces[opposite(sideToMove)][PAWN]), to - 8);}
    reset_bit(&(pieces[sideToMove][moved]), from);
    set_bit(&(pieces[sideToMove][moved]), to);
  /* normal move */
  } else {
    reset_bit(&(pieces[sideToMove][moved]), from);
    set_bit(&(pieces[sideToMove][moved]), to);
    if (capture != EMPTY) {
      reset_bit(&(pieces[opposite(sideToMove)][capture]), to);
    }
  }
  /* move is done */
  update_general();
  /* there are still castling options
   * available, update the conditions */
  if (conds.can_castle) {
    update_condtions(moved, to, from, sideToMove);
  }
  /* update the move stack */
  MoveHistory latest = create(from, to, moved, capture, replacement);
  moveStack.push(latest);
}

Bot::Bot(){
  /* start bitboards */
  for(int side = BLACK; side <= WHITE; side++){
    general_board[side] = 0ULL;
    for(int pice = PAWN; pice <= KING; pice++){
      pieces[side][pice] = 0ULL;
    }
  }

  /* set white pawns */
  for (int square = a2; square <= h2; ++square) {
    set_bit (&(pieces[WHITE][PAWN]), square);
  }
  /* set black pawns */
  for (int square = a7; square <= h7; ++square) {
    set_bit (&(pieces[BLACK][PAWN]), square);
  }
  /* set white rooks */
  set_bit(&(pieces[WHITE][ROOK]), a1);
  set_bit(&(pieces[WHITE][ROOK]), h1);
  /* set black rooks */
  set_bit(&(pieces[BLACK][ROOK]), a8);
  set_bit(&(pieces[BLACK][ROOK]), h8);

  /* set white knights */
  set_bit(&(pieces[WHITE][KNIGHT]), b1);
  set_bit(&(pieces[WHITE][KNIGHT]), g1);
  /* set black knights */
  set_bit(&(pieces[BLACK][KNIGHT]), b8);
  set_bit(&(pieces[BLACK][KNIGHT]), g8);

  /* set white bishops */
  set_bit(&(pieces[WHITE][BISHOP]), c1);
  set_bit(&(pieces[WHITE][BISHOP]), f1);
  /* set black bishops */
  set_bit(&(pieces[BLACK][BISHOP]), c8);
  set_bit(&(pieces[BLACK][BISHOP]), f8);

  /* set queens */
  set_bit(&(pieces[WHITE][QUEEN]), d1);
  set_bit(&(pieces[BLACK][QUEEN]), d8);

  /* set kings */
  set_bit(&(pieces[WHITE][KING]), e1);
  set_bit(&(pieces[BLACK][KING]), e8);

  /* set general boards */
  for (int side = BLACK; side <= WHITE; side++) {
    for (int pice = PAWN; pice <= KING; pice++) {
      general_board[side] |= pieces[side][pice];
    }
  }
}

/** For Checking & Constructing Moves **/
bool Bot::is_legal(int from, int to, PlaySide side) {
  /* initialize parameters */
  //bool side = getEngineSide();
  /* move can't be done */
  if (!can_move(from, to, side)) {
    return false;
  }
  /* check if move is normal */
  if (is_castle(from, to, side)) {
    return is_legal_castle(from, to, side);
  }
  /* change the board and back up the old move */
  Bitboard old_pieces[2][6]; 
  Bitboard old_general_board[2];
  backup_board(old_pieces, old_general_board);

  /* find the from and to pieces */
  for (int i = 0; i < 6; i++) {
    if (((1ULL << from) & pieces[side][i])) {
      /* empty the starting square */
      reset_bit (&(pieces[side][i]), from);
      /* place the piece on the destination square */
      set_bit (&(pieces[side][i]), to);
    }
    if (((1ULL << to) & pieces[opposite(side)][i])) {
      /* empty the destinatrion square if necessary */
      reset_bit (&(pieces[opposite(side)][i]), to);
    }
  }
  /* rebuild general boards */
  update_general();
  /* look for the last's move legality */
  /* for each possible square */
  for (int start = 0; start < 64; start ++) {
    /* if an opponent piece is found */
    if (get_bit(general_board[opposite(side)], start)) {
      /* check if it attacks the king */
      if (can_move(start, scan_bit(pieces[side][KING]), opposite(side))) {
        /* the move is illegal, revert to the old board */
        copy_board(old_pieces, old_general_board);
        return false;
      }
    }
  }
  /* the move is legal, revert to the old board */
  copy_board(old_pieces, old_general_board);
  return true;
}

bool Bot::is_legal_castle(int from, int to, PlaySide side) {
  /* cannot castle out of check */
  if (is_check(side)) {
    return false;
  }
  /* change the board and back up the old move */
  Bitboard old_pieces[2][6]; 
  Bitboard old_general_board[2];
  backup_board(old_pieces, old_general_board);
  /* place the rook and king in the final positions */
  int mid_castle_square;
  /* queenside castle */
  if (to < from) {
    /* replace the rook */
    reset_bit (&(pieces[side][ROOK]), from - 4);
    set_bit (&(pieces[side][ROOK]), from - 1);
    /* replace the king */
    reset_bit (&(pieces[side][KING]), from);
    set_bit (&(pieces[side][KING]), to);
    mid_castle_square = from - 1;
  }
  /* kingside castle */
  else {
    /* replace the rook */
    reset_bit (&(pieces[side][ROOK]), from + 3);
    set_bit (&(pieces[side][ROOK]), from + 1);
    /* replace the king */
    reset_bit (&(pieces[side][KING]), from);
    set_bit (&(pieces[side][KING]), to);
    mid_castle_square = from + 1;
  }
  /* update the general boards */
  update_general();
  /* look for the last's move legality */
  /* for each possible square */
  for (int start = 0; start < 64; start ++) {
    /* if an opponent piece is found */
    if (get_bit(general_board[opposite(side)], start)) {
      /* check if it attacks the king and the intermediary square */
      if (can_move(start, scan_bit(pieces[side][KING]), opposite(side)) ||
          can_move(start, mid_castle_square, opposite(side))) {
        /* the castle is illegal, revert to the old board */
        copy_board(old_pieces, old_general_board);
        return false;
      }
    }
  }
  /* the move is legal, revert to the old board */
  copy_board(old_pieces, old_general_board);
  return true;
}

bool Bot::can_move(int from, int to, PlaySide side) {
  /* set parameters */
  int diff = 0;
  int piece = EMPTY;
  int dline = abs((to / 8) - (from / 8));
  int dfile = abs((to % 8) - (from % 8));
  Bitboard BTB_to = 1ULL << to;
  Bitboard BTB_from = 1ULL << from;
  /* base cases */ 
  if(to < 0 || to > 63){
    return false;
  }
  if(from < 0 || from > 63){
    return false;
  }
  if(to == from){
    return false;
  }
  /* look for the pice */
  for (int i = 0; i < 6; ++i) {
    if ((BTB_from & pieces[side][i])) {
      piece = i;
    }
  }
  /* cannot move from empty square */
  if (piece == EMPTY) {
    return false;
  }
  /* the pieces from the 'to' and 'from'
    squares are the same color */
  for (int p = 0; p <= KING; p++) {
    if ((BTB_to & pieces[side][p])) {
      return false;
    }
  }

    
  switch (piece) {

    case PAWN:
    if(side == WHITE)
      diff -= 8;
    if(side == BLACK)
      diff += 8;
    /* illegal squares */
    if ((to - from) != diff && (to - from) != (diff + diff) &&
        (to - from) != (diff + 1) && (to - from) != (diff - 1)) {
      return 0;
    }
    /* move one square forward */
    if ((to - from) == diff) {
      /* occupied square */
      if (get_bit(general_board[opposite(side)] | general_board[side], to)) {
        return false;
      }
    }
    /* move two squares forward */
    if ((to - from) == (diff + diff)) {
      /* occupied squares(s) */
      if (get_bit(general_board[opposite(side)] | general_board[side], to - diff) ||
          get_bit(general_board[opposite(side)] | general_board[side], to)) {
        return false;
      }
      /* condition to move from 2-nd rank */
      if (side == WHITE && (from / 8) != 6) {
        return false;
      }
      if (side == BLACK && (from / 8) != 1) {
        return false;
      }
    }
    /* caputure left */
    if ((to - from) == (diff - 1)) {
      /* edge of the board */
      if ((to % 8) == 7) {
        return false;
      }
      /* empty square */
      bool last_move_cond = false;
      if  (!moveStack.empty()){
        last_move_cond = (moveStack.top().from == (from - 1 + diff * 2)
                          && moveStack.top().to == (from - 1)
                          && moveStack.top().moved == PAWN);
      }
      if (get_bit(general_board[opposite(side)], to) != 1) {
        /* en passant */
        //bool last_move_cond => last_move.from == (from - 1 - diff * 2) &&  last_move.to == (from - 1)
        if (get_bit(pieces[opposite(side)][PAWN], from - 1) != 1) {
          return false;
        }
        if (!last_move_cond) return false;
      }
    }
    /* capture right */
    if ((to - from) == (diff + 1)) {
      /* edge of the board */
      if ((to % 8) == 0) {
        return false;
      }
      /* empty square */
      bool last_move_cond = false;
      if  (!moveStack.empty()){
        last_move_cond = (moveStack.top().from == (from + 1 + diff * 2)
                          && moveStack.top().to == (from + 1)
                          && moveStack.top().moved == PAWN);
      }
      //bool last_move_cond => last_move.from == (from + 1 - diff * 2) &&  last_move.to == (from + 1)
      if (get_bit(general_board[opposite(side)], to) != 1) {
        /* en passant */
        if (get_bit(pieces[opposite(side)][PAWN], from + 1) != 1) {
          return false;
        }
        if (!last_move_cond) return false;
      }
    }
    /* legal move */
    break;


    case ROOK:
    /* rule out illegal moves */
    if ((from % 8 != to % 8) && (from / 8 != to / 8)) {
      return false;
    }
    /* vertical move */
    if (from % 8 == to % 8) {
      int offset = 0, from_cpy = from;
      if (to > from) (offset += 8); else (offset -= 8);
      from_cpy += offset;
      while (from_cpy != to) {
        if (get_bit(general_board[side] | general_board[opposite(side)], from_cpy)) {
          return false;
        }
        from_cpy += offset;
      }
    }
    /* horizontal move */
    if (from / 8 == to / 8) {
      int offset = 0, from_cpy = from;
      if (to > from) (offset += 1); else (offset -= 1);
      from_cpy += offset;
      while (from_cpy != to) {
        if (get_bit(general_board[side] | general_board[opposite(side)], from_cpy)) {
          return false;
        }
        from_cpy += offset;
      }
    }
    /* legal move */
    break;


    case KNIGHT:
    /* all cases where the move is correct */
    if (!((dline == 2 && dfile == 1) || (dline == 1 && dfile == 2))) {
      return false;
    }
    /* legal move */
    break;


    case BISHOP:
    /* make sure pice is positioned diagonally */
    if (dline != dfile) {
      return false;
    } else {
    /* get the increments */
      int bshp_y = ((to / 8) - (from / 8)) / dline;
      int bshp_x = ((to % 8) - (from % 8)) / dfile;
      for (int i = 1; i < dfile; i++) {
        if (get_bit(general_board[side] | general_board[opposite(side)],
          (from + i * bshp_x + 8 * i * bshp_y))) {
          return false;
        }
      }
    }
    /* legal move */
    break;


    case QUEEN:
    /* move on the same row */
    if (from / 8 == to / 8) {
      diff = (from > to) ? -1 : 1;
      /* row search for obstacles */
      for (int i = from + diff; i != to; i += diff) {
        if (get_bit(general_board[side] | general_board[opposite(side)], i)) {
          return false;
        }
      }
    /* move on the same file */
    } else if (from % 8 == to % 8) {
      diff = (from > to) ? -8 : 8;
      /* file search for obstacles */
      for (int i = from + diff; i != to; i += diff) {
        if (get_bit(general_board[side] | general_board[opposite(side)], i)) {
          return false;
        }
      }
    /* diagonal move */
    } else {
      diff = (from / 8 > to / 8) ? -8 : 8;
      diff += (from % 8 > to % 8) ? -1 : 1;
      /* search diagonally for obstacles */
      for (int i = from + diff; i != to; i += diff) {
        if (get_bit(general_board[side] | general_board[opposite(side)], i)) {
          return false;
        }
      }
    }
    /* legal move */
    break;


    case KING:
    /* exclude illegal perimeter */
    if (dline > 1 || dfile > 1) {
      /* exclude castling */
      if  (!((from == e1 && side == WHITE) || (from == e8 && side == BLACK))) {
        return false;
      }
      /* white castling */
      if (from == e1) {
        /* invalid destination */
        if (to != g1 && to != c1) { return false; }
        /* kingside */
        if (to == g1) {
          /* check for condition */
          if (!conds.castle_cond_ks_white) { return false; }
          /* check if squares are clear */
          if (get_bit((general_board[side] | general_board[opposite(side)]),f1) ||
              get_bit((general_board[side] | general_board[opposite(side)]),g1)) {
            return false;
          }
        }
        /* queenside */
        if (to == c1) {
          /* check for condition */
          if (!conds.castle_cond_qs_white) { return false; }
          /* check if squares are clear */
          if (get_bit((general_board[side] | general_board[opposite(side)]),d1) ||
              get_bit((general_board[side] | general_board[opposite(side)]),c1) ||
              get_bit((general_board[side] | general_board[opposite(side)]),b1)) {
            return false;
          }
        }
      }
      if (from == e8) {
        /* invalid destination */
        if (to != g8 && to != c8) { return false; }
        /* kingside */
        if (to == g8) {
          /* check for condition */
          if(!conds.castle_cond_ks_black) { return false; }
          /* check if squares are clear */
          if (get_bit((general_board[side] | general_board[opposite(side)]),f8) ||
              get_bit((general_board[side] | general_board[opposite(side)]),g8)) {
            return false;
          }
        }
        /* queenside */
        if (to == c8) {
          /* check for condition */
          if(!conds.castle_cond_qs_black) { return false; }
          /* check if squares are clear */
          if (get_bit((general_board[side] | general_board[opposite(side)]),d8) ||
              get_bit((general_board[side] | general_board[opposite(side)]),c8) ||
              get_bit((general_board[side] | general_board[opposite(side)]),b8)) {
            return false;
          }
        }
      }
    }
    /* legal move */
    break;
    default :
    /* No piece on the square */
    break;
  }
  return true;
}

bool Bot::is_castle(int from, int to, PlaySide side) {
  /* checks if the king is the moved piece */
  if (!get_bit(pieces[side][KING], from)) {
    return false;
  }
  if ((from == e1 && (to == g1 || to == c1)) ||
      (from == e8 && (to == g8 || to == c8))){
    return true;
  }
  return false;
}

bool Bot::is_enpassant(int from, int to, PlaySide side) {
     /* source square is a pawn */
  if (!get_bit(pieces[side][PAWN], from) ||
     /* destination square is empty */
      get_bit(general_board[opposite(side)], to) ||
     /* the piece move diagonally */
     (to % 8 == from % 8)) {
    return false;
  }
  return true;
}

bool Bot::is_check(PlaySide side) {
  /* for each possible square */
  for (int start = 0; start < 64; start ++) {
    /* if an opponent piece is found */
    if (get_bit(general_board[opposite(side)], start)) {
      /* check if it attacks the king */
      if (can_move(start, scan_bit(pieces[side][KING]), opposite(side))) {
        return true;
      }
    }
  }
  return false;
}

bool Bot::is_promotion(int from, int to, PlaySide side) {
  /* chech is piece is a pawn */
  if (pieces[side][PAWN] & (1ULL << from)) {
    if ((side == BLACK) && (to / 8 == 7)) return true;
    if ((side == WHITE) && (to / 8 == 0)) return true;
  }
  return false;
}

void Bot::update_condtions(Piece moved, int to, int from, PlaySide sideToMove) {
  /* move resulted in checked */
  if (is_check(opposite(sideToMove))) {
    if (opposite(sideToMove) == WHITE) {
      conds.castle_cond_ks_white = false;
      conds.castle_cond_qs_white = false;
    } else {
      conds.castle_cond_ks_black = false;
      conds.castle_cond_qs_black = false;
    }
  }
  /* king was moved */
  if (moved == KING) {
    if (sideToMove == WHITE) {
      conds.castle_cond_ks_white = false;
      conds.castle_cond_qs_white = false;
    } else {
      conds.castle_cond_ks_black = false;
      conds.castle_cond_qs_black = false;
    }
  }
  /* either rook was moved */
  if (moved == ROOK) {
    if (sideToMove == WHITE) {
      if (from == a1) {conds.castle_cond_qs_white = false;}
      if (from == h1) {conds.castle_cond_ks_white = false;}
    } else {
      if (from == a8) {conds.castle_cond_qs_black = false;}
      if (from == h8) {conds.castle_cond_ks_black = false;}
    }
  }
  /* either rook was captured */
  if (sideToMove == BLACK) {
    if (to == a1) {conds.castle_cond_qs_white = false;}
    if (to == h1) {conds.castle_cond_ks_white = false;}
  } else {
    if (to == a8) {conds.castle_cond_qs_black = false;}
    if (to == h8) {conds.castle_cond_ks_black = false;}
  }
  /* update general condition */
  conds.can_castle = (conds.castle_cond_ks_white ||
                      conds.castle_cond_qs_white ||
                      conds.castle_cond_ks_black ||
                      conds.castle_cond_qs_black);
  return;
}

Piece Bot::get_piece(int square, PlaySide side) {
  for (int p = 0; p < 6; p++) {
    if (get_bit(pieces[side][p], square)) {
      return (Piece)p;
    }
  }
  return EMPTY;
}

/** For Board Manipulation Opperations **/
void Bot::backup_board(Bitboard(*old_pieces)[6], Bitboard* old_general_board) {
  for (int side = BLACK; side <= WHITE; side++) {
    old_general_board[side] = general_board[side];
    for (int pice = PAWN; pice <= KING; pice++) {
      old_pieces[side][pice] = pieces[side][pice];
    }
  }
}

void Bot::copy_board(Bitboard(*src_pieces)[6], Bitboard* src_general_board) {
  for (int side = BLACK; side <= WHITE; side++) {
    general_board[side] = src_general_board[side];
    for (int pice = PAWN; pice <= KING; pice++) {
      pieces[side][pice] = src_pieces[side][pice];
    }
  }
}

void Bot::backup_conds(Bot::Conditions* backup_conds) {
  backup_conds->castle_cond_ks_black = conds.castle_cond_ks_black;
  backup_conds->castle_cond_ks_white = conds.castle_cond_ks_white;
  backup_conds->castle_cond_qs_black = conds.castle_cond_qs_black;
  backup_conds->castle_cond_qs_white = conds.castle_cond_qs_white;
  backup_conds->can_castle = conds.can_castle;
}

void Bot::copy_conds(Bot::Conditions src_conds) {
  conds.castle_cond_ks_black = src_conds.castle_cond_ks_black;
  conds.castle_cond_ks_white = src_conds.castle_cond_ks_white;
  conds.castle_cond_qs_black = src_conds.castle_cond_qs_black;
  conds.castle_cond_qs_white = src_conds.castle_cond_qs_white;
  conds.can_castle = src_conds.can_castle;
}

void Bot::update_general() {
  for (int side = BLACK; side <= WHITE; side++) {
    general_board[side] = 0ULL;
    for (int pice = PAWN; pice <= KING; pice++) {
      general_board[side] |= pieces[side][pice];
    }
  } 
}

/** For Computing Moves **/
std::vector<Move*> Bot::getMovePool(PlaySide side) {
  std::vector<Move*> moves;
  /* for each possible piece type */
  for (int p = 0; p < 6; p++) {
    /* for all the available spots to move from */
    std::vector<int> filled_squares = get_filled_squares(pieces[side][p]);
    while (!filled_squares.empty()) {
      /* get a square */
      int from = filled_squares.back();
      filled_squares.pop_back();
      /* try every move */
      for (int to = 0; to < 64; to++) {
        /* add legal moves to pool */
        if (is_legal(from, to, side)) {
          std::string source = to_chess_string(from);
          std::string destination = to_chess_string(to);
          if (is_promotion(from, to, side)) {
            for (int replc = ROOK; replc < KING; replc++) {
              Move* new_move = Move::promote(source, destination, (Piece)(replc));
              moves.push_back(new_move);
            }
          } else {
            Move* new_move = Move::moveTo(source, destination);
            moves.push_back(new_move);
          }
        }
      }
    }
  }
  return moves;
}

Move* Bot::calculateNextMove() {
  /* Play move for the side the engine is playing (Hint: getEngineSide())
   * Make sure to record your move in custom structures before returning.
   *
   * Return move that you are willing to submit
   * Move is to be constructed via one of the factory methods declared in Move.h */
  PlaySide side = getEngineSide();
  /* jumpstart minimax */
  std::vector<Move*> moves = getMovePool(side);
  Move* bestMove;
  if (moves.size() == 0) {
    return Move::resign();
  }
  int bestScore = NEGINF;
  int alpha = NEGINF;
  int beta = POSINF;
  /* for each possible move */
  for (unsigned int i = 0; i < moves.size(); i++) {
    Move* move = moves[i];
    /* save the current game state */
    Bitboard pieces_cpy[2][6]; 
    Bitboard general_board_cpy[2];
    Bot::Conditions conds_cpy;
    backup_board(pieces_cpy, general_board_cpy);
    backup_conds(&conds_cpy);
    /* recored move */
    recordMove(move, side);
    /* perform search */
    int eval = -minimax(opposite(side), 3, -beta, -alpha);
    /* undo the move */
    copy_board(pieces_cpy, general_board_cpy);
    copy_conds(conds_cpy);
    update_general();
    moveStack.pop();
    /* enemy move was too good */
    if (eval > bestScore) { bestMove = move; bestScore = eval; }
    if (bestScore > alpha) { alpha = bestScore; }
    if (alpha >= beta) { break; }
  }
  
  print_BTB(general_board[WHITE] | general_board[BLACK]);
  std::cout << "Qside : " << conds.castle_cond_qs_black << "  ";
  std::cout << "Kside : " << conds.castle_cond_ks_black << "\n";
  recordMove(bestMove, side);
  return bestMove;
}


/** For Board Evaluation **/
int Bot::minimax(PlaySide side, int depth, int alpha, int beta) {
  /* exit condition */
  if (depth == 0) {
    return evaluate(side);
  }
  std::vector<Move*> moves = getMovePool(side);
  if (moves.size() == 0) {
    /* checkmate case */
    if (is_check(side)) {
      return NEGINF;
    }
    /* stalemate case */
    return 0;
  }
  int bestScore = NEGINF;
  /* for each possible move */
  for (unsigned int i = 0; i < moves.size(); i++) {
    Move* move = moves[i];
    /* save the current game state */
    Bitboard pieces_cpy[2][6]; 
    Bitboard general_board_cpy[2];
    Bot::Conditions conds_cpy;
    backup_board(pieces_cpy, general_board_cpy);
    backup_conds(&conds_cpy);
    /* recored move */
    recordMove(move, side);
    /* perform search */
    int eval = -minimax(opposite(side), depth - 1, -beta, -alpha);
    /* undo the move */
    copy_board(pieces_cpy, general_board_cpy);
    copy_conds(conds_cpy);
    update_general();
    moveStack.pop();
    /* enemy move was too good */
    if (eval > bestScore) { bestScore = eval; }
    if (bestScore > alpha) { alpha = bestScore; }
    if (alpha > beta) { break; }
  }
  return bestScore;
}

int Bot::evaluate(PlaySide side) {
  int whiteScore = count_material(WHITE);
  int blackScore = count_material(BLACK);
  int score = whiteScore - blackScore;
  score = side ? score : -score;
  return score;
}

int Bot::count_material(PlaySide side) {
  int res = 0;
  res += count(PAWN, side) * pawnVal;
  res += count(KNIGHT, side) * knightVal;
  res += count(BISHOP, side) * bishopVal;
  res += count(ROOK, side) * rookVal;
  res += count(QUEEN, side) * queenVal;
  return res;
}

int Bot::count(int piece, PlaySide side) {
  int res = 0;
  Bitboard B = pieces[side][piece];
  while (B) {
    if (B & 1ULL) { res++; }
    B >>= 1;
  }
  return res;
}

std::string Bot::getBotName() { return Bot::BOT_NAME; }
