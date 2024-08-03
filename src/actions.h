/* generate.c -- by Vaino Hassinen */

#ifndef ACTIONS_H
#define ACTIONS_H

#define OP_DONE 		       0
#define OP_JUMP 		       1
#define OP_SET_FLAG 		       2
#define OP_CLEAR_FLAG 		       3
#define OP_JUMP_SET 		       4
#define OP_JUMP_CLEAR 		       5
#define OP_READ_MAN 		       6
#define OP_WHITE_PAWN_CAPTURE 	       7
#define OP_BLACK_PAWN_CAPTURE 	       8
#define OP_WHITE_PAWN_MOVE 	       9
#define OP_BLACK_PAWN_MOVE 	      10
#define OP_WHITE_PAWN_CAPTURE_RAISE   11
#define OP_BLACK_PAWN_CAPTURE_RAISE   12
#define OP_WHITE_PAWN_MOVE_RAISE      13
#define OP_BLACK_PAWN_MOVE_RAISE      14
#define OP_WHITE_ENPASSANT 	      15
#define OP_BLACK_ENPASSANT 	      16
#define OP_WHITE_PAWN_DOUBLE_MOVE     17
#define OP_BLACK_PAWN_DOUBLE_MOVE     18
#define OP_WHITE_R_RAY 		      19
#define OP_BLACK_R_RAY 		      20
#define OP_WHITE_B_RAY 		      21
#define OP_BLACK_B_RAY 		      22
#define OP_WHITE_P_RAY 		      23
#define OP_BLACK_P_RAY 		      24
#define OP_WHITE_MOVE 		      25
#define OP_BLACK_MOVE 		      26
#define OP_IS_NOT_MAN 		      27
#define OP_IS_NOT_THESE_MEN 	      28
#define OP_IS_NOT_MAN_BUT_EMPTY       29
#define OP_IS_NOT_THESE_MEN_BUT_EMPTY 30
#define OP_IS_MAN 		      31

extern long int *actions[2];
extern long int *spot_action[2][8*8];

#endif /* #ifndef ACTIONS_H */
