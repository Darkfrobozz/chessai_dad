/* strike.c -- by Vaino Hassinen */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess.h"
#include "actions.h"

typedef struct SQUARE {
  long int first_move;
  long int queue[12];
  long int length;
} SQUARE;

static SQUARE black[8*8],white[8*8];

long int white_swap_off(long int,long int,long int *,long int,long int *,long int *);
long int black_swap_off(long int,long int,long int *,long int,long int *,long int *);

#define WHITE_STRIKES_ON(_HITMAN,_SPOT,_MAN,_FIRST_MOVE) \
{                                                        \
  register SQUARE *square = &white[_SPOT];               \
  register long int i;                                        \
  register long int first_move = _FIRST_MOVE;                 \
                                                         \
  if(xray_stop_p) {                                      \
    if(IS_BLACK(_MAN)) {                                 \
      value -= _MAN*5;                                   \
    }                                                    \
							 \
    for(i = square->length++;                            \
        i && _HITMAN < square->queue[i-1];               \
        square->queue[i] = square->queue[i-1], i--);     \
                                                         \
    square->queue[i] = _HITMAN;                          \
                                                         \
    if(!i) square->first_move = 0;                       \
                                                         \
  } else if(!first_move) {                               \
    value += _MAN*10;                                    \
                                                         \
    for(i = square->length++;                            \
        i && _HITMAN < square->queue[i-1];               \
        square->queue[i] = square->queue[i-1], i--);     \
                                                         \
    square->queue[i] = _HITMAN;                          \
                                                         \
    if(!i) square->first_move = 0;                       \
                                                         \
  } else {                                               \
    value -= _MAN*10;                                    \
                                                         \
    for(i = square->length++;                            \
        i && _HITMAN <= square->queue[i-1];              \
        square->queue[i] = square->queue[i-1], i--);     \
                                                         \
    square->queue[i] = _HITMAN;                          \
                                                         \
    if(!i) square->first_move = first_move;              \
  }                                                      \
}

#define BLACK_STRIKES_ON(_HITMAN,_SPOT,_MAN,_FIRST_MOVE) \
{                                                        \
  register long int i;                                        \
  register SQUARE *square = &black[_SPOT];               \
  register long int first_move = _FIRST_MOVE;                 \
                                                         \
  if(xray_stop_p) {                                      \
    if(IS_BLACK(_MAN)) {                                 \
      value -= _MAN*10;                                  \
    }                                                    \
							 \
    for(i = square->length++;                            \
        i && _HITMAN > square->queue[i-1];               \
        square->queue[i] = square->queue[i-1], i--);     \
                                                         \
    square->queue[i] = _HITMAN;                          \
                                                         \
    if(!i) square->first_move = 0;                       \
                                                         \
  } else if(!first_move) {                               \
    value += _MAN*10;                                    \
                                                         \
    for(i = square->length++;                            \
        i && _HITMAN > square->queue[i-1];               \
        square->queue[i] = square->queue[i-1], i--);     \
                                                         \
    square->queue[i] = _HITMAN;                          \
                                                         \
    if(!i) square->first_move = 0;                       \
                                                         \
  } else {                                               \
    value -= _MAN*10;                                    \
                                                         \
    for(i = square->length++;                            \
        i && _HITMAN >= square->queue[i-1];              \
        square->queue[i] = square->queue[i-1], i--);     \
                                                         \
    square->queue[i] = _HITMAN;                          \
                                                         \
    if(!i) square->first_move = first_move;              \
  }                                                      \
}

#define MOBILITY(D)                                      \
{                                                        \
  if(xray_stop_p) {                                      \
    value += (D)*25;                                     \
  } else {                                               \
    value += (D)*100;                                    \
  }                                                      \
}

static long int setup_strikes()
{
  register long int *action_p;
  register long int *board_p = sit_p->save.board;
  register long int case_man;
  long int for_white;
  long int man;
  long int flag;
  long int spot;
  long int value = 0;
  long int *xray_stop_p;
  long int xray_steps;

  memset(white,0,sizeof white);
  memset(black,0,sizeof black);

  for_white = 1;
  action_p = actions[for_white];
  xray_stop_p = NULL;
  xray_steps = 0;

  while(1) {
    if(xray_steps) {
      xray_steps--;
      if(!xray_steps) action_p = xray_stop_p;
    }

    if(action_p == xray_stop_p) {
      xray_stop_p = NULL;
    }

    switch(*action_p++) {
    case OP_DONE:
      if(for_white) {
	for_white = 0;
	action_p = actions[for_white];
	xray_stop_p = NULL;
	xray_steps = 0;
      } else {
	return value;
      }
      break;

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
      spot = *action_p++;
      case_man = board_p[spot];
      action_p = (long int *) action_p[case_man +MAN_OFSET];
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_CAPTURE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_WHITE(man)) {
	WHITE_STRIKES_ON(case_man,spot,man,0);
      } else if(IS_BLACK(man)) {
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p);
      }
      action_p++;
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_CAPTURE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_BLACK(man)) {
	BLACK_STRIKES_ON(case_man,spot,man,0);
      } else if(IS_WHITE(man)) {
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p);
      }
      action_p++;
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_MOVE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+1);
      }
      action_p++;
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_MOVE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-1);
      }
      action_p++;
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_WHITE(man)) {
	WHITE_STRIKES_ON(case_man,spot,man,0);
      } else if(IS_BLACK(man)) {
	MOBILITY(+4);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p); /* Queen move */
      }
      action_p += 4;
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_BLACK(man)) {
	BLACK_STRIKES_ON(case_man,spot,man,0);
      } else if(IS_WHITE(man)) {
	MOBILITY(-4);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p); /* Queen move */
      }
      action_p += 4;
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+4);
      }
      action_p += 4;
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-4);
      }
      action_p += 4;
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_ENPASSANT:
      spot = *action_p++;

      man = board_p[spot];

      if(IS_WHITE(man)) {
	WHITE_STRIKES_ON(case_man,spot,man,0);
      } else if(IS_BLACK(man)) {
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p);
      } else if(spot == sit_p->save.enpassant) {
	man = BLACK_PAWN;
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p | ENPASSANT_OR_MASK);
      }
      action_p++;
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_ENPASSANT:
      spot = *action_p++;

      man = board_p[spot];

      if(IS_BLACK(man)) {
	BLACK_STRIKES_ON(case_man,spot,man,0);
      } else if(IS_WHITE(man)) {
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p);
      } else if(spot == sit_p->save.enpassant) {
	man = WHITE_PAWN;
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p | ENPASSANT_OR_MASK);
      }
      action_p++;
      break;

/*
  | OP |
->|dest|
  |move|
  |dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_DOUBLE_MOVE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+1);
	action_p++;

	spot = *action_p++;
	man = board_p[spot];

	if(IS_EMPTY(man)) {
	  MOBILITY(+1);
	}
	action_p++;
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-1);
	action_p++;

	spot = *action_p++;
	man = board_p[spot];

	if(IS_EMPTY(man)) {
	  MOBILITY(-1);
	}
	action_p++;
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
    case OP_BLACK_P_RAY:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-1);
	action_p += 2;

      } else if(IS_WHITE(man)) {
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p);

	if(IS_WHITE_BISHOP(man) ||
	   IS_WHITE_QUEEN(man)  && IS_BLACK_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}

      } else {
	BLACK_STRIKES_ON(case_man,spot,man,0);

	if(IS_BLACK_PAWN(man)) { /* Xraying trought but only one step */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;
	  if(!xray_steps) xray_steps = 2;

	} else if(IS_BLACK_BISHOP(man) ||
	          IS_BLACK_QUEEN(man)  && IS_BLACK_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}
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
    case OP_BLACK_B_RAY:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-1);
	action_p += 2;

      } else if(IS_WHITE(man)) {
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p);

	if(IS_WHITE_PAWN(man)) { /* Xraying trought but only one step */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;
	  if(!xray_steps) xray_steps = 2;

	} else if(IS_WHITE_BISHOP(man) ||
	          IS_WHITE_QUEEN(man)  && IS_BLACK_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}

      } else {
	BLACK_STRIKES_ON(case_man,spot,man,0);

	if(IS_BLACK_BISHOP(man) ||
	   IS_BLACK_QUEEN(man)  && IS_BLACK_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-1);
	action_p += 2;

      } else if(IS_WHITE(man)) {
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p);

	if(IS_WHITE_ROOK(man)  ||
	   IS_WHITE_QUEEN(man) && IS_BLACK_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}

      } else {
	BLACK_STRIKES_ON(case_man,spot,man,0);

	if(IS_BLACK_ROOK(man)  ||
	   IS_BLACK_QUEEN(man) && IS_BLACK_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}
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
    case OP_WHITE_P_RAY:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+1);
	action_p += 2;

      } else if(IS_BLACK(man)) {
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p);

	if(IS_BLACK_BISHOP(man) ||
	   IS_BLACK_QUEEN(man)  && IS_WHITE_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}

      } else {
	WHITE_STRIKES_ON(case_man,spot,man,0);

	if(IS_WHITE_PAWN(man)) { /* Xraying trought but only one step */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;
	  if(!xray_steps) xray_steps = 2;

	} else if(IS_WHITE_BISHOP(man) ||
	          IS_WHITE_QUEEN(man)  && IS_WHITE_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}
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
    case OP_WHITE_B_RAY:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+1);
	action_p += 2;

      } else if(IS_BLACK(man)) {
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p);

	if(IS_BLACK_PAWN(man)) { /* Xraying trought but only one step */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;
	  if(!xray_steps) xray_steps = 2;

	} else if(IS_BLACK_BISHOP(man) ||
		  IS_BLACK_QUEEN(man)  && IS_WHITE_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}

      } else {
	WHITE_STRIKES_ON(case_man,spot,man,0);

	if(IS_WHITE_BISHOP(man) ||
	   IS_WHITE_QUEEN(man)  && IS_WHITE_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}
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
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+1);
	action_p += 2;

      } else if(IS_BLACK(man)) {
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p);

	if(IS_BLACK_ROOK(man)  ||
	   IS_BLACK_QUEEN(man) && IS_WHITE_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}

      } else {
	WHITE_STRIKES_ON(case_man,spot,man,0);

	if(IS_WHITE_ROOK(man)  ||
	   IS_WHITE_QUEEN(man) && IS_WHITE_QUEEN(case_man))
	{ /* Xraying trought cheaper only */
	  action_p++;
	  xray_stop_p = *(long int **) action_p++;

	} else {
	  action_p++;
	  action_p = *(long int **) action_p;
	}
      }
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_MOVE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(-1);
      } else if(IS_WHITE(man)) {
	MOBILITY(-1);
	BLACK_STRIKES_ON(case_man,spot,man,*action_p);
      } else if(IS_BLACK(man)) {
	BLACK_STRIKES_ON(case_man,spot,man,0);
      }
      action_p++;
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_MOVE:
      spot = *action_p++;
      man = board_p[spot];

      if(IS_EMPTY(man)) {
	MOBILITY(+1);
      } else if(IS_BLACK(man)) {
	MOBILITY(+1);
	WHITE_STRIKES_ON(case_man,spot,man,*action_p);
      } else if(IS_WHITE(man)) {
	WHITE_STRIKES_ON(case_man,spot,man,0);
      }
      action_p++;
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
      printf("strike.c : Strange OP code %ld\n",action_p[-1]);
    }
  }
}

long int black_swap_off(
  long int stake,
  long int black_men_left,
  long int *black_queue,
  long int white_men_left,
  long int *white_queue,
  long int *depth_p
)
{
  if(black_men_left) {
    if(stake > WHITE_QUEEN) {
      (*depth_p)++;
      return VALUE_WHITE_KING*2/VALUE_WHITE_PAWN;

    } else {
      long int gain = stake + white_swap_off(*black_queue,
					black_men_left-1,
					black_queue+1,
					white_men_left,
					white_queue,
					depth_p);
      if(gain > 0) {
	(*depth_p)++;
	return gain;
      }
    }
  }

  *depth_p = 0;
  return 0;
}

long int white_swap_off(
  long int stake,
  long int black_men_left,
  long int *black_queue,
  long int white_men_left,
  long int *white_queue,
  long int *depth_p
)
{
  if(white_men_left) {
    if(stake < BLACK_QUEEN) {
      (*depth_p)++;
      return VALUE_BLACK_KING*2/VALUE_WHITE_PAWN;

    } else {
      long int gain = stake + black_swap_off(*white_queue,
					black_men_left,
					black_queue,
					white_men_left-1,
					white_queue+1,
					depth_p);
      if(gain < 0) {
	(*depth_p)++;
	return gain;
      }
    }
  }

  *depth_p = 0;
  return 0;
}

long int strike(
  long int for_white
)
{
  register long int place;
  register long int man;
  long int gain_depth;
  long int value = setup_strikes();
  long int max_gain = 0;
  long int first_move = 0;
  long int gain,depth;

  if(for_white) {
    for(place = 0; place < 8*8; place++) {
      man = sit_p->save.board[place];

      if(IS_BLACK(man) && white[place].length) {
	gain = white_swap_off(man,
			      black[place].length,
			      black[place].queue,
			      white[place].length,
			      white[place].queue,
			      &depth);

	if(max_gain > gain) {
	  max_gain = gain;
	  first_move = white[place].first_move;
	  gain_depth = depth;
	}
      }
    }

    if(max_gain && first_move) {
      value += -gain_depth - max_gain*VALUE_WHITE_PAWN/2;
    }

  } else {
    for(place = 0; place < 8*8; place++) {
      man = sit_p->save.board[place];

      if(IS_WHITE(man) && black[place].length) {
	gain = black_swap_off(man,
			      black[place].length,
			      black[place].queue,
			      white[place].length,
			      white[place].queue,
			      &depth);

	if(max_gain < gain) {
	  max_gain = gain;
	  first_move = black[place].first_move;
	  gain_depth = depth;
	}
      }
    }

    if(max_gain && first_move) {
      value += gain_depth - max_gain*VALUE_WHITE_PAWN/2;
    }
  }

  return value;
}
