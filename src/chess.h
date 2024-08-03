/* chess.h -- by Vaino Hassinen */

#ifndef CHESS_H
#define CHESS_H

#define MAX_PLY   (64)
#define MAX_MOVES (2*128)

#define TTBITS	      18
#define TTSIZE	      (1<<TTBITS)
#define TTMASK	      (TTSIZE-1)

#define PBITS	      13
#define	PSIZE	      (1<<PBITS)
#define PMASK	      (PSIZE-1)

#define CLEARED	            0
#define RECURSION           1
#define ESTIMATE            2
#define EXACT	            3
#define PRECISIONMASK       3

#define MAKE_DEPTH_PRECISION(_PLY,_PRECISION) (((_PLY) << 2) | (_PRECISION))
#define GET_PRECISION(_DP)                    ((_DP) & PRECISIONMASK)
#define GET_DEPTH(_DP)                        ((_DP) >> 2)

#define DEPTH_PRECISION_RECURSION MAKE_DEPTH_PRECISION(2*MAX_PLY,RECURSION)

#define MAX_VALUE ( 10000000)
#define MIN_VALUE (-10000000)

#define BLACK_VIRGIN_KING -20
#define BLACK_KING 	  -19
#define BLACK_QUEEN 	  -18
#define BLACK_VIRGIN_ROOK -11
#define BLACK_ROOK 	  -10
#define BLACK_BISHOP 	   -7
#define BLACK_KNIGHT 	   -6
#define BLACK_PAWN 	   -2
#define EMPTY 		    0
#define WHITE_PAWN 	    2
#define WHITE_KNIGHT 	    6
#define WHITE_BISHOP 	    7
#define WHITE_ROOK 	   10
#define WHITE_VIRGIN_ROOK  11
#define WHITE_QUEEN 	   18
#define WHITE_KING 	   19
#define WHITE_VIRGIN_KING  20

#define ENPASSANTMAN BLACK_VIRGIN_KING

#define VALUE_BLACK_VIRGIN_KING -1000000
#define VALUE_BLACK_KING 	-1000000
#define VALUE_BLACK_QUEEN 	-90000
#define VALUE_BLACK_VIRGIN_ROOK -50000
#define VALUE_BLACK_ROOK 	-50000
#define VALUE_BLACK_BISHOP 	-30000
#define VALUE_BLACK_KNIGHT 	-30000
#define VALUE_BLACK_PAWN 	-10000
#define VALUE_WHITE_PAWN 	 10000
#define VALUE_WHITE_KNIGHT 	 30000
#define VALUE_WHITE_BISHOP 	 30000
#define VALUE_WHITE_ROOK 	 50000
#define VALUE_WHITE_VIRGIN_ROOK  50000
#define VALUE_WHITE_QUEEN 	 90000
#define VALUE_WHITE_KING 	 1000000
#define VALUE_WHITE_VIRGIN_KING  1000000

/* lost short -2x, lost long -1x, done 0 */
#define VALUE_CASTLING           2000

#define MAN_OFSET          (-BLACK_VIRGIN_KING)

#define SPECIAL_NONE 	   0
#define SPECIAL_QUEEN 	   1
#define SPECIAL_ROOK 	   2
#define SPECIAL_BISHOP 	   3
#define SPECIAL_KNIGHT 	   4
#define SPECIAL_ENPASSANT  5
#define SPECIAL_DOUBLE 	   6
#define SPECIAL_S_CASTLING 7
#define SPECIAL_L_CASTLING 8

#define IS_WHITE_PAWN(M)   ((M) == WHITE_PAWN)
#define IS_BLACK_PAWN(M)   ((M) == BLACK_PAWN)
#define IS_WHITE_KNIGHT(M) ((M) == WHITE_KNIGHT)
#define IS_BLACK_KNIGHT(M) ((M) == BLACK_KNIGHT)
#define IS_WHITE_BISHOP(M) ((M) == WHITE_BISHOP)
#define IS_BLACK_BISHOP(M) ((M) == BLACK_BISHOP)
#define IS_WHITE_ROOK(M)   ((M) == WHITE_ROOK || (M) == WHITE_VIRGIN_ROOK)
#define IS_BLACK_ROOK(M)   ((M) == BLACK_ROOK || (M) == BLACK_VIRGIN_ROOK)
#define IS_WHITE_QUEEN(M)  ((M) == WHITE_QUEEN)
#define IS_BLACK_QUEEN(M)  ((M) == BLACK_QUEEN)
#define IS_WHITE_KING(M)   ((M) == WHITE_KING || (M) == WHITE_VIRGIN_KING)
#define IS_BLACK_KING(M)   ((M) == BLACK_KING || (M) == BLACK_VIRGIN_KING)

/*
                      1         2         3
            01234567890123456789012345678901
        TT: PPPPPPPTTTTTCKKKKKKKKKKKKKKKKKKK
         P: PPPPPPPTTTTTKKKKKKKKKKKKKKKKKKKK

*/

#define PAWN_BIT      0 /* There are upto 96 pawn moves */
#define TAKEN_BIT     7 /* There are 32 men */
#define COLOR_BIT    12
#define TT_KEY_BIT   13
#define P_KEY_BIT    12

#define set_white_color() { sit_p->save.ttkey_root |= (1<<(COLOR_BIT));  }
#define set_black_color() { sit_p->save.ttkey_root &= ~(1<<(COLOR_BIT)); }

#define register_pawn_moved(_STEPS) 	              \
{ 					              \
  sit_p->save.checkpoint++; 	                      \
  sit_p->save.ttkey_root += (_STEPS)*(1<<(PAWN_BIT)); \
  sit_p->save.pkey_root  += (_STEPS)*(1<<(PAWN_BIT)); \
}

#define register_pice_taken()                 \
{                                             \
  sit_p->save.checkpoint++;                   \
  sit_p->save.ttkey_root += (1<<(TAKEN_BIT)); \
}

#define register_pawn_taken()                 \
{                                             \
  sit_p->save.checkpoint++;                   \
  sit_p->save.ttkey_root += (1<<(TAKEN_BIT)); \
  sit_p->save.pkey_root += (1<<(TAKEN_BIT));  \
}

#define set_hash()                                             \
{                                                              \
  sit_p->ttkey = sit_p->save.ttkey_root                        \
		| (sit_p->save.tthash & ~((1<<TT_KEY_BIT)-1)); \
  sit_p->tt_p  = &TT[sit_p->save.tthash & TTMASK];             \
  sit_p->pkey  = sit_p->save.pkey_root                         \
                | (sit_p->save.phash  & ~((1<<P_KEY_BIT)-1));  \
  sit_p->p_p   = &P[sit_p->save.phash & PMASK];                \
}

#define sit_pop() 		     \
{ 				     \
  sit_p = sit_p->last_p; 	     \
}

#define sit_push() 		     \
{ 				     \
  sit_p = sit_p->next_p; 	     \
  sit_p->save = *sit_p->last_save_p; \
}

#define Q(R,C) (((R) << 3) | (C))
#define GET_R(P) ((P) >> 3)
#define GET_C(P) ((P) & ((1 << 3)-1))

#define MASK_R(P) ((P) & Q(7,0))
#define MASK_C(P) ((P) & Q(0,7))

#define IS_BLACK(P) ((P)  < EMPTY)
#define IS_WHITE(P) ((P)  > EMPTY)
#define IS_EMPTY(P) ((P) == EMPTY)

#define A1 Q(0,0)
#define B1 Q(0,1)
#define C1 Q(0,2)
#define D1 Q(0,3)
#define E1 Q(0,4)
#define F1 Q(0,5)
#define G1 Q(0,6)
#define H1 Q(0,7)
#define A8 Q(7,0)
#define B8 Q(7,1)
#define C8 Q(7,2)
#define D8 Q(7,3)
#define E8 Q(7,4)
#define F8 Q(7,5)
#define G8 Q(7,6)
#define H8 Q(7,7)

/*
          3         2         1
         10987654321098765432109876543210
   Move: 0000000000000000ssssfffffftttttt
*/

#define GET_TO(M)              ((M) & ((1 << 6)-1))
#define GET_FROM(M)            (((M) >> 6) & ((1 << 6)-1))
#define GET_SPECIAL(M)         ((M) >> 12)
#define GET_BUTTERFLY(M)       ((M) & ((1 << 12)-1))

#define MAKE_S_MOVE(F,T,S)     (((S) << 12) | ((F) << 6) | (T))
#define MAKE_MOVE(F,T)         MAKE_S_MOVE(F,T,SPECIAL_NONE)

#define ENPASSANT_OR_MASK      (SPECIAL_ENPASSANT << 12)

#define ROW_DELTA              (Q(1,0) - Q(0,0))
#define COLUMN_DELTA           (Q(0,1) - Q(0,0))

typedef struct TTENTRY {
  unsigned long int key;
  long int value;
  unsigned long int checkpoint;
  unsigned long int move;
  unsigned long int depth_precision;
} TTENTRY;

extern TTENTRY TT[TTSIZE];

typedef struct PENTRY {
  unsigned long int key;
  long int value;
} PENTRY;

extern PENTRY P[PSIZE];

typedef struct SITUATION {
  struct {
    long int board[8*8];
    long int black_king_place;
    long int white_king_place;
    long int material_balance;
    long int bonus;
    long int enpassant;
    long int checkpoint;
    long int checkpoint_level;
    unsigned long int ttkey_root;
    unsigned long int pkey_root;
    unsigned long int phash;
    unsigned long int tthash;
  } save,
   *last_save_p;            /* Never change once set */
  struct SITUATION *last_p; /*           .-          */
  struct SITUATION *next_p; /*           .-          */
  long int *score_2_p; 	    /*           .-          */
  long int *score_1_p; 	    /*           .-          */
  long int score_0;
  long int moves[MAX_MOVES];
  long int scores[MAX_MOVES];
  unsigned long int ttkey;
  unsigned long int pkey;
  TTENTRY *tt_p;
  PENTRY *p_p;
} SITUATION;

extern SITUATION sit[MAX_PLY];
extern SITUATION *sit_p;

extern long int accepted_pc[MAX_PLY];
extern long int accepted_score;

extern long int number_of_evals;

extern long int have_message;
extern long int message_checked;

extern long int butterfly_board[GET_BUTTERFLY(~0)];

extern unsigned long int const hash[WHITE_VIRGIN_KING+MAN_OFSET+1][8*8];

/* from actions.c */
void init_moves();

/* from generate.c */
void generate_moves(
  long int for_white
);

/* from legal.c */
long int is_legal_move(
  long int for_white,
  long int move
);

/* from move.c */
long int do_move(
  long int themove,
  long int ply
);

/* from tools.c */
long int cannot_strike_at(
  long int for_white,
  long int place
);
void init_tools();
int board_convert(
 int  place
);
long int move_convert(
  long int move
);
long int man_value(
  long int man
);
void set_board_dependencies(
  long int for_white,
  long int optional_enpassant
);
long int man_convert(
  long int man
);
void dump_board(
  long int playing_white
);
long int move_is_available(
  long int themove
);
long int candidate_move(
  long int themove
);
char *convert_binary_move_to_ascii(
  long int themove
);
long int convert_ascii_to_binary_move(
  char *move_s
);
long int get_move();
void set_board();
void display_moves();
void sort_for_white();
void sort_for_black();
void load_board(
  long int *playing_white_p
);

/* from eval.c */
long int eval(
  long int for_white,
  long int cut_off
);

/* from loop.c */
void ttupdate(
  long int depth_precision,
  long int value,
  long int move,
  long int growth
);
void timeout_abort();
void timeout_unlimited();
void timeout_alter(
  long int playing_white,
  long int remaining_time,
  long int remaining_moves,
  long int used_time
);
void loop(
  long int playing_white,
  long int last_move
);

/* from strike.c */
long int strike(
  long int for_white
);

/* from chess.c */
void check_message();

#endif /* CHESS_H */
