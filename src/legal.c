/* legal.c -- by Vaino Hassinen */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess.h"
#include "actions.h"

long int is_legal_move(
  long int for_white,
  long int move
)
{
  register long int *action_p;
  register long int *board_p;
  register long int case_man;
  long int man;
  long int flag;

  /* Skip OP_READ_MAN */
  action_p = spot_action[for_white][GET_FROM(move)] + 1;

  board_p = sit_p->save.board;

  case_man = board_p[*action_p++];
  action_p = (long int *) action_p[case_man + MAN_OFSET];

  while(1) {
    switch(*action_p++) {
    case OP_DONE:
      return 0;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP:
      action_p = *(long int **) action_p;
      break;

/*
  | OP |
->| OP |
*/
    case OP_SET_FLAG:
      flag = 1;
      break;

/*
  | OP |
->| OP |
*/
    case OP_CLEAR_FLAG:
      flag = 0;
      break;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP_SET:
      if(flag) {
	action_p = *(long int **) action_p;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP_CLEAR:
      if(!flag) {
	action_p = *(long int **) action_p;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|src |
  | -8 |-------\
  | -7 |-----\ |
  z    z     | |
  | OP |<----/ |
  z    z       |
  | OP |<------/
*/
    case OP_READ_MAN:
      return 0;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_CAPTURE:
      man = board_p[*action_p++];

      if(IS_BLACK(man)) {
	if(move == *action_p++) return 1;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_CAPTURE:
      man = board_p[*action_p++];

      if(IS_WHITE(man)) {
	if(move == *action_p++) return 1;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_MOVE:
      man = board_p[*action_p++];

      if(IS_EMPTY(man)) {
	if(move == *action_p++) return 1;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_MOVE:
      man = board_p[*action_p++];

      if(IS_EMPTY(man)) {
	if(move == *action_p++) return 1;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  |move|
  |move|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_CAPTURE_RAISE:
      man = board_p[*action_p++];

      if(IS_BLACK(man)) {
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
      } else {
	action_p += 4;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  |move|
  |move|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_CAPTURE_RAISE:
      man = board_p[*action_p++];

      if(IS_WHITE(man)) {
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
      } else {
	action_p += 4;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  |move|
  |move|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_MOVE_RAISE:
      man = board_p[*action_p++];

      if(IS_EMPTY(man)) {
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
      } else {
	action_p += 4;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  |move|
  |move|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_MOVE_RAISE:
      man = board_p[*action_p++];

      if(IS_EMPTY(man)) {
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
	if(move == *action_p++) return 1;
      } else {
	action_p += 4;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_ENPASSANT: {
      long int spot = *action_p++;

      man = board_p[spot];

      if(IS_BLACK(man)) {
	if(move == *action_p++) return 1;

      } else if(spot == sit_p->save.enpassant) {
	if(move == (*action_p++ | ENPASSANT_OR_MASK)) return 1;

      } else {
	action_p++;
      }
    } break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_ENPASSANT: {
      long int spot = *action_p++;

      man = board_p[spot];

      if(IS_WHITE(man)) {
	if(move == *action_p++) return 1;

      } else if(spot == sit_p->save.enpassant) {
	if(move == (*action_p++ | ENPASSANT_OR_MASK)) return 1;

      } else {
	action_p++;
      }
    } break;

/*
  | OP |
->|dest|
  |move|
  |dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_DOUBLE_MOVE:
      if(IS_EMPTY(board_p[*action_p++])) {
	if(move == *action_p++) return 1;
	if(IS_EMPTY(board_p[*action_p++])) {
	  if(move == *action_p++) return 1;
	} else {
	  action_p++;
	}
      } else {
	action_p += 3;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  |dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_DOUBLE_MOVE:
      if(IS_EMPTY(board_p[*action_p++])) {
	if(move == *action_p++) return 1;
	if(IS_EMPTY(board_p[*action_p++])) {
	  if(move == *action_p++) return 1;
	} else {
	  action_p++;
	}
      } else {
	action_p += 3;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | .  |--\
  | OP |  |
  z    z  |
  | OP |<-/
*/
    case OP_BLACK_R_RAY:
    case OP_BLACK_B_RAY:
    case OP_BLACK_P_RAY:
      man = board_p[*action_p++];

      if(IS_EMPTY(man)) {
	if(move == *action_p++) return 1;
	action_p++;
      } else if(IS_WHITE(man)) {
	if(move == *action_p++) return 1;
	action_p = *(long int **) action_p;
      } else {
	action_p++;
	action_p = *(long int **) action_p;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | .  |--\
  | OP |  |
  z    z  |
  | OP |<-/
*/
    case OP_WHITE_R_RAY:
    case OP_WHITE_B_RAY:
    case OP_WHITE_P_RAY:
      man = board_p[*action_p++];

      if(IS_EMPTY(man)) {
	if(move == *action_p++) return 1;
	action_p++;
      } else if(IS_BLACK(man)) {
	if(move == *action_p++) return 1;
	action_p = *(long int **) action_p;
      } else {
	action_p++;
	action_p = *(long int **) action_p;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_MOVE:
      man = board_p[*action_p++];

      if(IS_BLACK(man)) {
	action_p++;
      } else {
	if(move == *action_p++) return 1;
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_MOVE:
      man = board_p[*action_p++];

      if(IS_WHITE(man)) {
	action_p++;
      } else {
	if(move == *action_p++) return 1;
      }
      break;

/*
  | OP |
->|dest|
  |man |
  | .  |--\
  | OP |  |
  z    z  |
  | OP |<-/
*/
    case OP_IS_NOT_MAN:
      man = board_p[*action_p++];

      if(man == *action_p++) {
	action_p = *(long int **) action_p;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|dest|
  |man1|
  |man2|
  | .  |--\
  | OP |  |
  z    z  |
  | OP |<-/
*/
    case OP_IS_NOT_THESE_MEN:
      man = board_p[*action_p++];

      if(man == *action_p++) {
	action_p++;
	action_p = *(long int **) action_p;
      } else if(man == *action_p++) {
	action_p = *(long int **) action_p;
      } else {
	action_p++;
      }
      break;

/*
  | OP |
->|dest|
  |man |
  | .  |-------\
  | .  |-----\ |
  z    z     | |
  | OP |<----/ |
  z    z       |
  | OP |<------/
*/
    case OP_IS_NOT_MAN_BUT_EMPTY:
      man = board_p[*action_p++];

      if(man == *action_p++) {
	action_p = *(long int **) action_p;
      } else if(man != EMPTY) {
	action_p++;
	action_p = *(long int **) action_p;
      } else {
	action_p += 2;
      }
      break;

/*
  | OP |
->|dest|
  |man1|
  |man2|
  | .  |-------\
  | .  |-----\ |
  z    z     | |
  | OP |<----/ |
  z    z       |
  | OP |<------/
*/
    case OP_IS_NOT_THESE_MEN_BUT_EMPTY:
      man = board_p[*action_p++];

      if(man == *action_p++) {
	action_p++;
	action_p = *(long int **) action_p;
      } else if(man == *action_p++) {
	action_p = *(long int **) action_p;
      } else if(man != EMPTY) {
	action_p++;
	action_p = *(long int **) action_p;
      } else {
	action_p += 2;
      }
      break;

/*
  | OP |
->|dest|
  |man |
  | .  |--\
  | OP |  |
  z    z  |
  | OP |<-/
*/
    case OP_IS_MAN:
      man = board_p[*action_p++];

      if(man != *action_p++) {
	action_p = *(long int **) action_p;
      } else {
	action_p++;
      }
      break;

    default:
      printf("legal.c : Strange OP code %ld\n",action_p[-1]);
    }
  }  
}
