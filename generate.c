/* generate.c -- by Vaino Hassinen */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess.h"
#include "actions.h"

#define STORE_WHITE_MOVE(THEMOVE,THESCORE)     \
{ 					       \
  int father; 				       \
  int trailer; 				       \
  int thescore = (THESCORE); 		       \
  int themove = (THEMOVE); 		       \
 					       \
  trailer = vacant; 			       \
  father = trailer >> 1; 		       \
 					       \
  while(father && scores[father] < thescore) { \
    moves[trailer] = moves[father]; 	       \
    scores[trailer] = scores[father]; 	       \
 					       \
    trailer = father; 			       \
    father >>= 1; 			       \
  } 					       \
 					       \
  moves[trailer] = themove; 		       \
  scores[trailer] = thescore; 		       \
 					       \
  vacant++; 				       \
}

#define STORE_BLACK_MOVE(THEMOVE,THESCORE)     \
{ 					       \
  int father; 				       \
  int trailer; 				       \
  int thescore = (THESCORE); 		       \
  int themove = (THEMOVE); 		       \
 					       \
  trailer = vacant; 			       \
  father = trailer >> 1; 		       \
 					       \
  while(father && scores[father] > thescore) { \
    moves[trailer] = moves[father]; 	       \
    scores[trailer] = scores[father]; 	       \
 					       \
    trailer = father; 			       \
    father >>= 1; 			       \
  } 					       \
 					       \
  moves[trailer] = themove; 		       \
  scores[trailer] = thescore; 		       \
 					       \
  vacant++; 				       \
}

void generate_moves(
  int for_white
)
{
  register int *action_p = actions[for_white];
  int vacant;
  int *moves = &sit_p->moves[-1];
  int *scores = &sit_p->scores[-1];
  register int *board_p = sit_p->save.board;
  register int case_man;
  int man;
  int flag;

  vacant = 1;

  while(1) {
    switch(*action_p++) {
    case OP_DONE:
      moves[vacant] = 0;
#ifdef TEST_IS_LEGAL
      for(moves = sit_p->moves; *moves; moves++) {
	if(!is_legal_move(for_white,*moves)) {
	  printf("This move (");
	  move_convert(*moves);
	  printf(") was not considered legal for %s\n",
		 for_white ? "white" : "black");
	}
	if(is_legal_move(!for_white,*moves)) {
	  printf("This move (");
	  move_convert(*moves);
	  printf(") was considered legal for %s\n",
		 !for_white ? "white" : "black");
	}
      }
#endif
      return;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP:
      action_p = *(int **) action_p;
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
	action_p = *(int **) action_p;
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
	action_p = *(int **) action_p;
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
      case_man = board_p[*action_p++];
      action_p = (int *) action_p[case_man +MAN_OFSET];
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_CAPTURE:
      man = board_p[*action_p++];

      if(IS_BLACK(man)) {
	STORE_WHITE_MOVE(*action_p++,-man-case_man);
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
	STORE_BLACK_MOVE(*action_p++,-man-case_man);
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
	STORE_WHITE_MOVE(*action_p++,-EMPTY-case_man);
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
	STORE_BLACK_MOVE(*action_p++,-EMPTY-case_man);
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
	STORE_WHITE_MOVE(*action_p++,-man+(WHITE_QUEEN -WHITE_PAWN));
	STORE_WHITE_MOVE(*action_p++,-man+(WHITE_ROOK  -WHITE_PAWN));
	STORE_WHITE_MOVE(*action_p++,-man+(WHITE_BISHOP-WHITE_PAWN));
	STORE_WHITE_MOVE(*action_p++,-man+(WHITE_KNIGHT-WHITE_PAWN));
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
	STORE_BLACK_MOVE(*action_p++,-man+(BLACK_QUEEN -BLACK_PAWN));
	STORE_BLACK_MOVE(*action_p++,-man+(BLACK_ROOK  -BLACK_PAWN));
	STORE_BLACK_MOVE(*action_p++,-man+(BLACK_BISHOP-BLACK_PAWN));
	STORE_BLACK_MOVE(*action_p++,-man+(BLACK_KNIGHT-BLACK_PAWN));
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
	STORE_WHITE_MOVE(*action_p++,-EMPTY+(WHITE_QUEEN -WHITE_PAWN));
	STORE_WHITE_MOVE(*action_p++,-EMPTY+(WHITE_ROOK  -WHITE_PAWN));
	STORE_WHITE_MOVE(*action_p++,-EMPTY+(WHITE_BISHOP-WHITE_PAWN));
	STORE_WHITE_MOVE(*action_p++,-EMPTY+(WHITE_KNIGHT-WHITE_PAWN));
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
	STORE_BLACK_MOVE(*action_p++,-EMPTY+(BLACK_QUEEN -BLACK_PAWN));
	STORE_BLACK_MOVE(*action_p++,-EMPTY+(BLACK_ROOK  -BLACK_PAWN));
	STORE_BLACK_MOVE(*action_p++,-EMPTY+(BLACK_BISHOP-BLACK_PAWN));
	STORE_BLACK_MOVE(*action_p++,-EMPTY+(BLACK_KNIGHT-BLACK_PAWN));
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
      int spot = *action_p++;

      man = board_p[spot];

      if(IS_BLACK(man)) {
	STORE_WHITE_MOVE(*action_p++,-man-case_man);

      } else if(spot == sit_p->save.enpassant) {
	STORE_WHITE_MOVE(*action_p++ | ENPASSANT_OR_MASK,-BLACK_PAWN-case_man);

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
      int spot = *action_p++;

      man = board_p[spot];

      if(IS_WHITE(man)) {
	STORE_BLACK_MOVE(*action_p++,-man-case_man);

      } else if(spot == sit_p->save.enpassant) {
	STORE_BLACK_MOVE(*action_p++ | ENPASSANT_OR_MASK,-WHITE_PAWN-case_man);

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
	STORE_WHITE_MOVE(*action_p++,-EMPTY-case_man);
	if(IS_EMPTY(board_p[*action_p++])) {
	  STORE_WHITE_MOVE(*action_p++,-EMPTY-case_man);
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
	STORE_BLACK_MOVE(*action_p++,-EMPTY-case_man);
	if(IS_EMPTY(board_p[*action_p++])) {
	  STORE_BLACK_MOVE(*action_p++,-EMPTY-case_man);
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
	STORE_BLACK_MOVE(*action_p++,-EMPTY-case_man);
	action_p++;
      } else if(IS_WHITE(man)) {
	STORE_BLACK_MOVE(*action_p++,-man-case_man);
	action_p = *(int **) action_p;
      } else {
	action_p++;
	action_p = *(int **) action_p;
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
	STORE_WHITE_MOVE(*action_p++,-EMPTY-case_man);
	action_p++;
      } else if(IS_BLACK(man)) {
	STORE_WHITE_MOVE(*action_p++,-man-case_man);
	action_p = *(int **) action_p;
      } else {
	action_p++;
	action_p = *(int **) action_p;
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
	STORE_BLACK_MOVE(*action_p++,-man-case_man);
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
	STORE_WHITE_MOVE(*action_p++,-man-case_man);
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
	action_p = *(int **) action_p;
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
	action_p = *(int **) action_p;
      } else if(man == *action_p++) {
	action_p = *(int **) action_p;
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
	action_p = *(int **) action_p;
      } else if(man != EMPTY) {
	action_p++;
	action_p = *(int **) action_p;
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
	action_p = *(int **) action_p;
      } else if(man == *action_p++) {
	action_p = *(int **) action_p;
      } else if(man != EMPTY) {
	action_p++;
	action_p = *(int **) action_p;
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
	action_p = *(int **) action_p;
      } else {
	action_p++;
      }
      break;

    default:
      printf("generate.c : Strange OP code %d\n",action_p[-1]);
    }
  }
}
