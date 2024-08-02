/* move.c - by Vaino Hassinen */

#include <stdio.h>

#include "chess.h"

unsigned int const hash[WHITE_VIRGIN_KING+MAN_OFSET+1][8*8]
#include "hash.h"
;

#define TTUPDATE(_F,_F_MAN,_T,_T_MAN) \
  sit_p->save.tthash ^= hash[_F_MAN+MAN_OFSET][_F] ^ hash[_T_MAN+MAN_OFSET][_T]
#define PUPDATE(_F,_F_MAN,_T,_T_MAN) \
  sit_p->save.phash ^= hash[_F_MAN+MAN_OFSET][_F] ^ hash[_T_MAN+MAN_OFSET][_T]

int do_move(
  int themove,
  int ply
)
{
  int from = GET_FROM(themove);
  int to = GET_TO(themove);
  int man;
  int taken;

  sit_push();

  man = sit_p->save.board[from];
  sit_p->save.board[from] = EMPTY;

  if(sit_p->save.enpassant >= 0) {
    sit_p->save.tthash ^= hash[ENPASSANTMAN+MAN_OFSET][sit_p->save.enpassant];
  }
  sit_p->save.enpassant = -1;

  taken = sit_p->save.board[to];

  switch(man) {
  case BLACK_VIRGIN_KING:
    set_white_color();

    sit_p->save.black_king_place = to;

    man = BLACK_KING;
    sit_p->save.material_balance += VALUE_BLACK_KING-VALUE_BLACK_VIRGIN_KING;

    switch(GET_SPECIAL(themove)) {
    case SPECIAL_S_CASTLING:
      PUPDATE(E8,BLACK_VIRGIN_KING,G8,BLACK_KING);
      TTUPDATE(E8,BLACK_VIRGIN_KING,G8,BLACK_KING);
      TTUPDATE(H8,BLACK_VIRGIN_ROOK,F8,BLACK_ROOK);
      sit_p->save.material_balance += VALUE_BLACK_ROOK-VALUE_BLACK_VIRGIN_ROOK;
      sit_p->save.board[H8] = EMPTY;
      sit_p->save.board[F8] = BLACK_ROOK;
      sit_p->save.bonus -= 3*VALUE_CASTLING;
      break;

    case SPECIAL_L_CASTLING:
      PUPDATE(E8,BLACK_VIRGIN_KING,C8,BLACK_KING);
      TTUPDATE(E8,BLACK_VIRGIN_KING,C8,BLACK_KING);
      TTUPDATE(A8,BLACK_VIRGIN_ROOK,D8,BLACK_ROOK);
      sit_p->save.material_balance += VALUE_BLACK_ROOK-VALUE_BLACK_VIRGIN_ROOK;
      sit_p->save.board[A8] = EMPTY;
      sit_p->save.board[D8] = BLACK_ROOK;
      sit_p->save.bonus -= 3*VALUE_CASTLING;
      break;

    default:
      PUPDATE(E8,BLACK_VIRGIN_KING,to,BLACK_KING);
      TTUPDATE(E8,BLACK_VIRGIN_KING,to,BLACK_KING);
      break;
    }

    if(sit_p->save.board[A8] == BLACK_VIRGIN_ROOK) {
      TTUPDATE(A8,BLACK_VIRGIN_ROOK,A8,BLACK_ROOK);
      sit_p->save.board[A8] = BLACK_ROOK;
      sit_p->save.material_balance += VALUE_BLACK_ROOK-VALUE_BLACK_VIRGIN_ROOK;
      sit_p->save.bonus += VALUE_CASTLING;
    }
    if(sit_p->save.board[H8] == BLACK_VIRGIN_ROOK) {
      TTUPDATE(H8,BLACK_VIRGIN_ROOK,H8,BLACK_ROOK);
      sit_p->save.board[H8] = BLACK_ROOK;
      sit_p->save.material_balance += VALUE_BLACK_ROOK-VALUE_BLACK_VIRGIN_ROOK;
      sit_p->save.bonus += 2*VALUE_CASTLING;
    }
    break;

  case BLACK_VIRGIN_ROOK:
    set_white_color();

    man = BLACK_ROOK;
    sit_p->save.material_balance += VALUE_BLACK_ROOK-VALUE_BLACK_VIRGIN_ROOK;

    if(from == H8) {
      TTUPDATE(H8,BLACK_VIRGIN_ROOK,to,BLACK_ROOK);
      sit_p->save.bonus += 2*VALUE_CASTLING;
    } else {
      TTUPDATE(A8,BLACK_VIRGIN_ROOK,to,BLACK_ROOK);
      sit_p->save.bonus += VALUE_CASTLING;
    }

    if(sit_p->save.board[A8] != BLACK_VIRGIN_ROOK &&
       sit_p->save.board[H8] != BLACK_VIRGIN_ROOK)
    {
      PUPDATE(E8,BLACK_VIRGIN_KING,E8,BLACK_KING);
      TTUPDATE(E8,BLACK_VIRGIN_KING,E8,BLACK_KING);
      sit_p->save.board[E8] = BLACK_KING;
      sit_p->save.material_balance += VALUE_BLACK_KING-VALUE_BLACK_VIRGIN_KING;
    }
    break;

  case BLACK_PAWN:
    set_white_color();

    switch(GET_SPECIAL(themove)) {
    case SPECIAL_DOUBLE:
      register_pawn_moved(2);

      PUPDATE(from,BLACK_PAWN,to,BLACK_PAWN);
      TTUPDATE(from,BLACK_PAWN,to,BLACK_PAWN);
      sit_p->save.enpassant = (from+to) >> 1;
      sit_p->save.tthash ^=hash[ENPASSANTMAN+MAN_OFSET][sit_p->save.enpassant];
      break;
    case SPECIAL_ENPASSANT:
      register_pawn_moved(1);

      PUPDATE(from,BLACK_PAWN,to,BLACK_PAWN);
      TTUPDATE(from,BLACK_PAWN,to,BLACK_PAWN);
      taken = WHITE_PAWN;
      sit_p->save.board[to+ROW_DELTA] = EMPTY;
      break;
    case SPECIAL_QUEEN:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[BLACK_PAWN+MAN_OFSET][from];
      TTUPDATE(from,BLACK_PAWN,to,BLACK_QUEEN);
      man = BLACK_QUEEN;
      sit_p->save.material_balance += VALUE_BLACK_QUEEN-VALUE_BLACK_PAWN;
      break;
    case SPECIAL_ROOK:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[BLACK_PAWN+MAN_OFSET][from];
      TTUPDATE(from,BLACK_PAWN,to,BLACK_ROOK);
      man = BLACK_ROOK;
      sit_p->save.material_balance += VALUE_BLACK_ROOK-VALUE_BLACK_PAWN;
      break;
    case SPECIAL_BISHOP:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[BLACK_PAWN+MAN_OFSET][from];
      TTUPDATE(from,BLACK_PAWN,to,BLACK_BISHOP);
      man = BLACK_BISHOP;
      sit_p->save.material_balance += VALUE_BLACK_BISHOP-VALUE_BLACK_PAWN;
      break;
    case SPECIAL_KNIGHT:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[BLACK_PAWN+MAN_OFSET][from];
      TTUPDATE(from,BLACK_PAWN,to,BLACK_KNIGHT);
      man = BLACK_KNIGHT;
      sit_p->save.material_balance += VALUE_BLACK_KNIGHT-VALUE_BLACK_PAWN;
      break;
    default:
      register_pawn_moved(1);

      PUPDATE(from,BLACK_PAWN,to,BLACK_PAWN);
      TTUPDATE(from,BLACK_PAWN,to,BLACK_PAWN);
      break;
    }
    break;

  case WHITE_VIRGIN_KING:
    set_black_color();

    sit_p->save.white_king_place = to;

    man = WHITE_KING;
    sit_p->save.material_balance += VALUE_WHITE_KING-VALUE_WHITE_VIRGIN_KING;

    switch(GET_SPECIAL(themove)) {
    case SPECIAL_S_CASTLING:
      PUPDATE(E1,WHITE_VIRGIN_KING,G1,WHITE_KING);
      TTUPDATE(E1,WHITE_VIRGIN_KING,G1,WHITE_KING);
      TTUPDATE(H1,WHITE_VIRGIN_ROOK,F1,WHITE_ROOK);
      sit_p->save.material_balance += VALUE_WHITE_ROOK-VALUE_WHITE_VIRGIN_ROOK;
      sit_p->save.board[H1] = EMPTY;
      sit_p->save.board[F1] = WHITE_ROOK;
      sit_p->save.bonus += 3*VALUE_CASTLING;
      break;

    case SPECIAL_L_CASTLING:
      PUPDATE(E1,WHITE_VIRGIN_KING,C1,WHITE_KING);
      TTUPDATE(E1,WHITE_VIRGIN_KING,C1,WHITE_KING);
      TTUPDATE(A1,WHITE_VIRGIN_ROOK,D1,WHITE_ROOK);
      sit_p->save.material_balance += VALUE_WHITE_ROOK-VALUE_WHITE_VIRGIN_ROOK;
      sit_p->save.board[A1] = EMPTY;
      sit_p->save.board[D1] = WHITE_ROOK;
      sit_p->save.bonus += 3*VALUE_CASTLING;
      break;

    default:
      PUPDATE(E1,WHITE_VIRGIN_KING,to,WHITE_KING);
      TTUPDATE(E1,WHITE_VIRGIN_KING,to,WHITE_KING);
      break;
    }

    if(sit_p->save.board[A1] == WHITE_VIRGIN_ROOK) {
      TTUPDATE(A1,WHITE_VIRGIN_ROOK,A1,WHITE_ROOK);
      sit_p->save.board[A1] = WHITE_ROOK;
      sit_p->save.material_balance += VALUE_WHITE_ROOK-VALUE_WHITE_VIRGIN_ROOK;
      sit_p->save.bonus -= VALUE_CASTLING;
    }
    if(sit_p->save.board[H1] == WHITE_VIRGIN_ROOK) {
      TTUPDATE(H1,WHITE_VIRGIN_ROOK,H1,WHITE_ROOK);
      sit_p->save.board[H1] = WHITE_ROOK;
      sit_p->save.material_balance += VALUE_WHITE_ROOK-VALUE_WHITE_VIRGIN_ROOK;
      sit_p->save.bonus -= 2*VALUE_CASTLING;
    }
    break;

  case WHITE_VIRGIN_ROOK:
    set_black_color();

    man = WHITE_ROOK;
    sit_p->save.material_balance += VALUE_WHITE_ROOK-VALUE_WHITE_VIRGIN_ROOK;

    if(from == H1) {
      TTUPDATE(H1,WHITE_VIRGIN_ROOK,to,WHITE_ROOK);
      sit_p->save.bonus -= 2*VALUE_CASTLING;
    } else {
      TTUPDATE(A1,WHITE_VIRGIN_ROOK,to,WHITE_ROOK);
      sit_p->save.bonus -= VALUE_CASTLING;
    }

    if(sit_p->save.board[A1] != WHITE_VIRGIN_ROOK &&
       sit_p->save.board[H1] != WHITE_VIRGIN_ROOK)
    {
      PUPDATE(E1,WHITE_VIRGIN_KING,E1,WHITE_KING);
      TTUPDATE(E1,WHITE_VIRGIN_KING,E1,WHITE_KING);
      sit_p->save.board[E1] = WHITE_KING;
      sit_p->save.material_balance += VALUE_WHITE_KING-VALUE_WHITE_VIRGIN_KING;
    }
    break;

  case WHITE_PAWN:
    set_black_color();

    switch(GET_SPECIAL(themove)) {
    case SPECIAL_DOUBLE:
      register_pawn_moved(2);

      PUPDATE(from,WHITE_PAWN,to,WHITE_PAWN);
      TTUPDATE(from,WHITE_PAWN,to,WHITE_PAWN);
      sit_p->save.enpassant = (from+to) >> 1;
      sit_p->save.tthash ^=hash[ENPASSANTMAN+MAN_OFSET][sit_p->save.enpassant];
      break;
    case SPECIAL_ENPASSANT:
      register_pawn_moved(1);

      PUPDATE(from,WHITE_PAWN,to,WHITE_PAWN);
      TTUPDATE(from,WHITE_PAWN,to,WHITE_PAWN);
      taken = BLACK_PAWN;
      sit_p->save.board[to-ROW_DELTA] = EMPTY;
      break;
    case SPECIAL_QUEEN:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[WHITE_PAWN+MAN_OFSET][from];
      TTUPDATE(from,WHITE_PAWN,to,WHITE_QUEEN);
      man = WHITE_QUEEN;
      sit_p->save.material_balance += VALUE_WHITE_QUEEN-VALUE_WHITE_PAWN;
      break;
    case SPECIAL_ROOK:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[WHITE_PAWN+MAN_OFSET][from];
      TTUPDATE(from,WHITE_PAWN,to,WHITE_ROOK);
      man = WHITE_ROOK;
      sit_p->save.material_balance += VALUE_WHITE_ROOK-VALUE_WHITE_PAWN;
      break;
    case SPECIAL_BISHOP:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[WHITE_PAWN+MAN_OFSET][from];
      TTUPDATE(from,WHITE_PAWN,to,WHITE_BISHOP);
      man = WHITE_BISHOP;
      sit_p->save.material_balance += VALUE_WHITE_BISHOP-VALUE_WHITE_PAWN;
      break;
    case SPECIAL_KNIGHT:
      register_pawn_moved(1);

      sit_p->save.phash ^= hash[WHITE_PAWN+MAN_OFSET][from];
      TTUPDATE(from,WHITE_PAWN,to,WHITE_KNIGHT);
      man = WHITE_KNIGHT;
      sit_p->save.material_balance += VALUE_WHITE_KNIGHT-VALUE_WHITE_PAWN;
      break;
    default:
      register_pawn_moved(1);

      PUPDATE(from,WHITE_PAWN,to,WHITE_PAWN);
      TTUPDATE(from,WHITE_PAWN,to,WHITE_PAWN);
      break;
    }
    break;

  case BLACK_KING:
    set_white_color();

    PUPDATE(from,BLACK_KING,to,BLACK_KING);
    TTUPDATE(from,BLACK_KING,to,BLACK_KING);
    sit_p->save.black_king_place = to;
    break;

  case BLACK_QUEEN:
    set_white_color();

    TTUPDATE(from,BLACK_QUEEN,to,BLACK_QUEEN);
    break;

  case BLACK_ROOK:
    set_white_color();

    TTUPDATE(from,BLACK_ROOK,to,BLACK_ROOK);
    break;

  case BLACK_BISHOP:
    set_white_color();

    TTUPDATE(from,BLACK_BISHOP,to,BLACK_BISHOP);
    break;

  case BLACK_KNIGHT:
    set_white_color();

    TTUPDATE(from,BLACK_KNIGHT,to,BLACK_KNIGHT);
    break;

  case WHITE_KNIGHT:
    set_black_color();

    TTUPDATE(from,WHITE_KNIGHT,to,WHITE_KNIGHT);
    break;

  case WHITE_BISHOP:
    set_black_color();

    TTUPDATE(from,WHITE_BISHOP,to,WHITE_BISHOP);
    break;

  case WHITE_ROOK:
    set_black_color();

    TTUPDATE(from,WHITE_ROOK,to,WHITE_ROOK);
    break;

  case WHITE_QUEEN:
    set_black_color();

    TTUPDATE(from,WHITE_QUEEN,to,WHITE_QUEEN);
    break;

  case WHITE_KING:
    set_black_color();

    PUPDATE(from,WHITE_KING,to,WHITE_KING);
    TTUPDATE(from,WHITE_KING,to,WHITE_KING);
    sit_p->save.white_king_place = to;
    break;

  case EMPTY:
    printf("Moving nothing?\n");
    set_hash();
    return 0;

  default:
    printf("Strange man (%d) moving\n",man);
    set_hash();
    return 0;
  }

  sit_p->save.board[to] = man;

  switch(taken) {
  case BLACK_VIRGIN_KING:
    register_pice_taken();

    sit_p->save.tthash ^= hash[BLACK_VIRGIN_KING+MAN_OFSET][to];
    sit_p->save.phash ^= hash[BLACK_VIRGIN_KING+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_BLACK_VIRGIN_KING;
    sit_p->save.bonus -= ply;
    sit_p->save.black_king_place = -1;
    set_hash();
    return 2;

  case BLACK_KING:
    register_pice_taken();

    sit_p->save.tthash ^= hash[BLACK_KING+MAN_OFSET][to];
    sit_p->save.phash ^= hash[BLACK_KING+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_BLACK_KING;
    sit_p->save.bonus -= ply;
    sit_p->save.black_king_place = -1;
    set_hash();
    return 2;

  case BLACK_QUEEN:       
    register_pice_taken();

    sit_p->save.tthash ^= hash[BLACK_QUEEN+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_BLACK_QUEEN;
    sit_p->save.bonus -= ply;
    set_hash();
    return 1;

  case BLACK_VIRGIN_ROOK: 
    register_pice_taken();

    sit_p->save.material_balance -= VALUE_BLACK_VIRGIN_ROOK;
    sit_p->save.bonus -= ply;

    if(to == H8) {
      sit_p->save.tthash ^= hash[BLACK_VIRGIN_ROOK+MAN_OFSET][H8];
      sit_p->save.bonus += 2*VALUE_CASTLING;
    } else {
      sit_p->save.tthash ^= hash[BLACK_VIRGIN_ROOK+MAN_OFSET][A8];
      sit_p->save.bonus += VALUE_CASTLING;
    }

    if(sit_p->save.board[A8] != BLACK_VIRGIN_ROOK &&
       sit_p->save.board[H8] != BLACK_VIRGIN_ROOK)
    {
      PUPDATE(E8,BLACK_VIRGIN_KING,E8,BLACK_KING);
      TTUPDATE(E8,BLACK_VIRGIN_KING,E8,BLACK_KING);
      sit_p->save.board[E8] = BLACK_KING;
      sit_p->save.material_balance += VALUE_BLACK_KING-VALUE_BLACK_VIRGIN_KING;
    }
    set_hash();
    return 1;

  case BLACK_ROOK:        
    register_pice_taken();

    sit_p->save.tthash ^= hash[BLACK_ROOK+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_BLACK_ROOK;
    sit_p->save.bonus -= ply;
    set_hash();
    return 1;

  case BLACK_BISHOP:      
    register_pice_taken();

    sit_p->save.tthash ^= hash[BLACK_BISHOP+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_BLACK_BISHOP;
    sit_p->save.bonus -= ply;
    set_hash();
    return 1;

  case BLACK_KNIGHT:      
    register_pice_taken();

    sit_p->save.tthash ^= hash[BLACK_KNIGHT+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_BLACK_KNIGHT;
    sit_p->save.bonus -= ply;
    set_hash();
    return 1;

  case BLACK_PAWN:        
    register_pawn_taken();

    if(themove & ENPASSANT_OR_MASK) {
      sit_p->save.phash ^= hash[BLACK_PAWN+MAN_OFSET][to-ROW_DELTA];
      sit_p->save.tthash ^= hash[BLACK_PAWN+MAN_OFSET][to-ROW_DELTA];
    } else {
      sit_p->save.phash ^= hash[BLACK_PAWN+MAN_OFSET][to];
      sit_p->save.tthash ^= hash[BLACK_PAWN+MAN_OFSET][to];
    }
    sit_p->save.material_balance -= VALUE_BLACK_PAWN;
    sit_p->save.bonus -= ply;
    set_hash();
    return 1;

  case WHITE_PAWN:        
    register_pawn_taken();

    if(themove & ENPASSANT_OR_MASK) {
      sit_p->save.phash ^= hash[WHITE_PAWN+MAN_OFSET][to+ROW_DELTA];
      sit_p->save.tthash ^= hash[WHITE_PAWN+MAN_OFSET][to+ROW_DELTA];
    } else {
      sit_p->save.phash ^= hash[WHITE_PAWN+MAN_OFSET][to];
      sit_p->save.tthash ^= hash[WHITE_PAWN+MAN_OFSET][to];
    }
    sit_p->save.material_balance -= VALUE_WHITE_PAWN;
    sit_p->save.bonus += ply;
    set_hash();
    return 1;

  case WHITE_KNIGHT:      
    register_pice_taken();

    sit_p->save.tthash ^= hash[WHITE_KNIGHT+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_WHITE_KNIGHT;
    sit_p->save.bonus += ply;
    set_hash();
    return 1;

  case WHITE_BISHOP:      
    register_pice_taken();

    sit_p->save.tthash ^= hash[WHITE_BISHOP+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_WHITE_BISHOP;
    sit_p->save.bonus += ply;
    set_hash();
    return 1;

  case WHITE_ROOK:        
    register_pice_taken();

    sit_p->save.tthash ^= hash[WHITE_ROOK+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_WHITE_ROOK;
    sit_p->save.bonus += ply;
    set_hash();
    return 1;

  case WHITE_VIRGIN_ROOK: 
    register_pice_taken();

    sit_p->save.material_balance -= VALUE_WHITE_VIRGIN_ROOK;
    sit_p->save.bonus += ply;

    if(to == H1) {
      sit_p->save.tthash ^= hash[WHITE_VIRGIN_ROOK+MAN_OFSET][H1];
      sit_p->save.bonus -= 2*VALUE_CASTLING;
    } else {
      sit_p->save.tthash ^= hash[WHITE_VIRGIN_ROOK+MAN_OFSET][A1];
      sit_p->save.bonus -= VALUE_CASTLING;
    }

    if(sit_p->save.board[A1] != WHITE_VIRGIN_ROOK &&
       sit_p->save.board[H1] != WHITE_VIRGIN_ROOK)
    {
      PUPDATE(E1,WHITE_VIRGIN_KING,E1,WHITE_KING);
      TTUPDATE(E1,WHITE_VIRGIN_KING,E1,WHITE_KING);
      sit_p->save.board[E1] = WHITE_KING;
      sit_p->save.material_balance += VALUE_WHITE_KING-VALUE_WHITE_VIRGIN_KING;
    }
    set_hash();
    return 1;

  case WHITE_QUEEN:       
    register_pice_taken();

    sit_p->save.tthash ^= hash[WHITE_QUEEN+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_WHITE_QUEEN;
    sit_p->save.bonus += ply;
    set_hash();
    return 1;

  case WHITE_KING:
    register_pice_taken();

    sit_p->save.phash ^= hash[WHITE_KING+MAN_OFSET][to];
    sit_p->save.tthash ^= hash[WHITE_KING+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_WHITE_KING;
    sit_p->save.bonus += ply;
    sit_p->save.white_king_place = -1;
    set_hash();
    return 2;

  case WHITE_VIRGIN_KING:
    register_pice_taken();

    sit_p->save.phash ^= hash[WHITE_VIRGIN_KING+MAN_OFSET][to];
    sit_p->save.tthash ^= hash[WHITE_VIRGIN_KING+MAN_OFSET][to];
    sit_p->save.material_balance -= VALUE_WHITE_VIRGIN_KING;
    sit_p->save.bonus += ply;
    sit_p->save.white_king_place = -1;
    set_hash();
    return 2;

  case EMPTY:
    set_hash();
    return 1;

  default:
    printf("Strange man (%d) taken\n",man);
    set_hash();
    return 0;
  }
}

#ifdef STANDALONE

main()
{
  int playing_white;
  init_tools();
  init_moves();

  set_board();

  playing_white = 1;

  dump_board(playing_white);
  generate_moves(playing_white);

  while(1) {
    int *mp;
    int themove = get_move();

    for(mp = sit_p->moves; *mp && *mp != themove; mp++);

    if(*mp) {
      do_move(themove,0);
      playing_white = !playing_white;
      dump_board(playing_white);
      generate_moves(playing_white);
    } else {
      display_moves();
    }
  }
}

#endif /* STANDALONE */
