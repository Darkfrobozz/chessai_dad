/* actions.c -- by Vaino Hassinen */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess.h"
#include "actions.h"

#define IS_UNCERTAIN 0
#define IS_THERE     1
#define IS_NOT_THERE 2

#define CMD_RAY_SLOT  0
#define CMD_MOVE_SLOT 1

long int *actions[2];
long int *spot_action[2][8*8];

static long int man_board_status[8*8][WHITE_VIRGIN_KING+MAN_OFSET+1];
static long int backup_man_board_status[8*8][WHITE_VIRGIN_KING+MAN_OFSET+1];
static long int *ray_board[8*8][8];
static long int white_ray_command[8][2] = {
  {OP_WHITE_R_RAY,OP_WHITE_MOVE}, /* 0 */
  {OP_WHITE_P_RAY,OP_WHITE_MOVE}, /* 1 */
  {OP_WHITE_R_RAY,OP_WHITE_MOVE}, /* 2 */
  {OP_WHITE_P_RAY,OP_WHITE_MOVE}, /* 3 */
  {OP_WHITE_R_RAY,OP_WHITE_MOVE}, /* 4 */
  {OP_WHITE_B_RAY,OP_WHITE_MOVE}, /* 5 */
  {OP_WHITE_R_RAY,OP_WHITE_MOVE}, /* 6 */
  {OP_WHITE_B_RAY,OP_WHITE_MOVE}  /* 7 */
};
static long int black_ray_command[8][2] = {
  {OP_BLACK_R_RAY,OP_BLACK_MOVE}, /* 0 */
  {OP_BLACK_B_RAY,OP_BLACK_MOVE}, /* 1 */
  {OP_BLACK_R_RAY,OP_BLACK_MOVE}, /* 2 */
  {OP_BLACK_B_RAY,OP_BLACK_MOVE}, /* 3 */
  {OP_BLACK_R_RAY,OP_BLACK_MOVE}, /* 4 */
  {OP_BLACK_P_RAY,OP_BLACK_MOVE}, /* 5 */
  {OP_BLACK_R_RAY,OP_BLACK_MOVE}, /* 6 */
  {OP_BLACK_P_RAY,OP_BLACK_MOVE}  /* 7 */
};

#define S(C) 		      \
{ 			      \
  count++; 		      \
  if(head_p) *head_p++ = (C); \
}

typedef struct DEPOSIT {
  long int *place_p;
#ifdef STANDALONE
  long int label;
#endif
  struct DEPOSIT *next_p;
} DEPOSIT;

static void deposit_address(
  DEPOSIT **dep_pp,
  long int *addr_p
)
{
  if(*dep_pp) {
    deposit_address(&(*dep_pp)->next_p,addr_p);
    if((*dep_pp)->place_p) *(long int **) (*dep_pp)->place_p = addr_p;
    free(*dep_pp);
    *dep_pp = NULL;
  }
}

static void defer_address(
  DEPOSIT **dep_pp,
  long int *place_p
)
{
  DEPOSIT *dep_p = (DEPOSIT *) malloc(sizeof(DEPOSIT));

  dep_p->place_p = place_p;

  dep_p->next_p = *dep_pp;
  *dep_pp = dep_p;
}

static void join_addresses(
  DEPOSIT **dep_pp,
  DEPOSIT **short_coalease_pp
)
{
  if(*dep_pp) {
    join_addresses(&(*dep_pp)->next_p,short_coalease_pp);
  } else {
    *dep_pp = *short_coalease_pp;
    *short_coalease_pp = NULL;
  }
}

static void clear_all_above(
  DEPOSIT **dep_pp,
  long int *place_p
)
{
  if(*dep_pp) {
    if((unsigned long int) (*dep_pp)->place_p >= (unsigned long int) place_p) {
      DEPOSIT *next_p = (*dep_pp)->next_p;

      free(*dep_pp);
      *dep_pp = next_p;
      clear_all_above(dep_pp,place_p);
    } else {
      clear_all_above(&(*dep_pp)->next_p,place_p);
    }
  }
}

static long int single_move(
  long int op,
  long int dr,
  long int dc,
  long int row,
  long int column,
  long int *head_p
)
{
  long int count = 0;
  long int r = row + dr;
  long int c = column + dc;

  if(0 <= r && r <= 7 && 0 <= c && c <= 7) {
    S(op);
    S(Q(r,c));
    S(MAKE_MOVE(Q(row,column),Q(r,c)));
  }

  return count;
}

#define SINGLE_MOVE(OP,DR,DC) 			   \
{ 						   \
  long int d = single_move(OP,DR,DC,row,column,head_p); \
  count += d; 					   \
  if(head_p) head_p += d; 			   \
}

/*
     2
   3   1
 4       0
   5   7
     6
*/

static long int ray(
  long int start,
  long int ray_command[8][2],
  DEPOSIT **dep_pp,
  long int *head_p,
  long int row,
  long int column
)
{
  long int count = 0;
  DEPOSIT *short_coalease_p = NULL;
  long int i;

  for(i = start; i < 8; i += 2) {
    long int *vp = ray_board[Q(row,column)][i];

    if(vp[0] && !vp[1]) {
      deposit_address(&short_coalease_p,head_p);

      S(ray_command[i][CMD_MOVE_SLOT]);
      S(GET_TO(vp[0]));
      S(vp[0]);
    } else if(vp[0]) {
      long int *pp;

      deposit_address(&short_coalease_p,head_p);

      for(pp = vp; *pp; pp++);
      for(pp--; pp != vp; pp--) {
	S(ray_command[i][CMD_RAY_SLOT]);
	S(GET_TO(*pp));
	S(*pp);
	defer_address(&short_coalease_p,head_p);
	S(0);
      }
      S(ray_command[i][CMD_MOVE_SLOT]);
      S(GET_TO(*pp));
      S(*pp);
    }
  }

  join_addresses(dep_pp,&short_coalease_p);

  return count;
}

#define RAY(B,RAY_COMMAND,DEP) 		            \
{ 						    \
  long int d = ray(B,RAY_COMMAND,DEP,head_p,row,column); \
  count += d; 					    \
  if(head_p) head_p += d; 			    \
}

static void change_status(
  long int place,
  long int man,
  long int status
)
{
  long int op,mi,in_fact_empty;

  if(status == IS_THERE) {
    for(mi = BLACK_VIRGIN_KING; mi <= WHITE_VIRGIN_KING; mi++) {
      man_board_status[place][mi+MAN_OFSET] = IS_NOT_THERE;
    }

    switch(man) {
    case BLACK_VIRGIN_KING:
    case BLACK_KING:
      for(op = 0; op < 8*8; op++) {
	change_status(op,BLACK_KING,IS_NOT_THERE);
	change_status(op,BLACK_VIRGIN_KING,IS_NOT_THERE);
      }
      break;
    case WHITE_VIRGIN_KING:
    case WHITE_KING:
      for(op = 0; op < 8*8; op++) {
	change_status(op,WHITE_KING,IS_NOT_THERE);
	change_status(op,WHITE_VIRGIN_KING,IS_NOT_THERE);
      }
      break;
    }
  }

  man_board_status[place][man+MAN_OFSET] = status;

  in_fact_empty = 1;
  for(mi = BLACK_VIRGIN_KING; mi <= WHITE_VIRGIN_KING; mi++) {
    if(!IS_EMPTY(mi)) {
      if(man_board_status[place][mi+MAN_OFSET] != IS_NOT_THERE) {
	in_fact_empty = 0;
      }
    }
  }

  if(in_fact_empty) {
    man_board_status[place][EMPTY+MAN_OFSET] = IS_THERE;
  }
}

static void place_man_at(
  long int row,
  long int column,
  long int man,
  long int status
)
{
  if(0 <= row && row <= 7 && 0 <= column && column <= 7) {
    change_status(Q(row,column),man,status);

    if(status == IS_THERE) {
      switch(man) {
      case BLACK_VIRGIN_KING:
      case BLACK_KING:
	place_man_at(row+1,column+1,WHITE_KING,IS_NOT_THERE);
	place_man_at(row+1,column  ,WHITE_KING,IS_NOT_THERE);
	place_man_at(row+1,column-1,WHITE_KING,IS_NOT_THERE);
	place_man_at(row  ,column+1,WHITE_KING,IS_NOT_THERE);
	place_man_at(row  ,column-1,WHITE_KING,IS_NOT_THERE);
	place_man_at(row-1,column+1,WHITE_KING,IS_NOT_THERE);
	place_man_at(row-1,column  ,WHITE_KING,IS_NOT_THERE);
	place_man_at(row-1,column-1,WHITE_KING,IS_NOT_THERE);

	place_man_at(row+1,column+1,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row+1,column  ,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row+1,column-1,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row  ,column+1,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row  ,column-1,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row-1,column+1,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row-1,column  ,WHITE_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row-1,column-1,WHITE_VIRGIN_KING,IS_NOT_THERE);
	break;
      case WHITE_VIRGIN_KING:
      case WHITE_KING:
	place_man_at(row+1,column+1,BLACK_KING,IS_NOT_THERE);
	place_man_at(row+1,column  ,BLACK_KING,IS_NOT_THERE);
	place_man_at(row+1,column-1,BLACK_KING,IS_NOT_THERE);
	place_man_at(row  ,column+1,BLACK_KING,IS_NOT_THERE);
	place_man_at(row  ,column-1,BLACK_KING,IS_NOT_THERE);
	place_man_at(row-1,column+1,BLACK_KING,IS_NOT_THERE);
	place_man_at(row-1,column  ,BLACK_KING,IS_NOT_THERE);
	place_man_at(row-1,column-1,BLACK_KING,IS_NOT_THERE);

	place_man_at(row+1,column+1,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row+1,column  ,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row+1,column-1,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row  ,column+1,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row  ,column-1,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row-1,column+1,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row-1,column  ,BLACK_VIRGIN_KING,IS_NOT_THERE);
	place_man_at(row-1,column-1,BLACK_VIRGIN_KING,IS_NOT_THERE);
	break;
      }
    }
  }
}

static void make_status_uncertain()
{
  long int row,column,mi;

  for(row = 0; row < 8; row++) {
    for(column = 0; column < 8; column++) {
      for(mi = BLACK_VIRGIN_KING; mi <= WHITE_VIRGIN_KING; mi++) {
	switch(mi) {
	case BLACK_VIRGIN_KING:
	case BLACK_KING:
	case BLACK_QUEEN:
	case BLACK_VIRGIN_ROOK:
	case BLACK_ROOK:
	case BLACK_BISHOP:
	case BLACK_KNIGHT:
	case BLACK_PAWN:
	case EMPTY:
	case WHITE_PAWN:
	case WHITE_KNIGHT:
	case WHITE_BISHOP:
	case WHITE_ROOK:
	case WHITE_VIRGIN_ROOK:
	case WHITE_QUEEN:
	case WHITE_KING:
	case WHITE_VIRGIN_KING:
	  man_board_status[Q(row,column)][mi+MAN_OFSET] = IS_UNCERTAIN;
	  break;
	default:
	  man_board_status[Q(row,column)][mi+MAN_OFSET] = IS_NOT_THERE;
	  break;
	}
      }

      if(row != 0 && (column != 0 || column != 7)) {
	man_board_status[Q(row,column)][WHITE_VIRGIN_ROOK+MAN_OFSET]
	  = IS_NOT_THERE;
      }
      if(row != 0 && column != 4) {
	man_board_status[Q(row,column)][WHITE_VIRGIN_KING+MAN_OFSET]
	  = IS_NOT_THERE;
      }
      if(row != 7 && (column != 0 || column != 7)) {
	man_board_status[Q(row,column)][BLACK_VIRGIN_ROOK+MAN_OFSET]
	  = IS_NOT_THERE;
      }
      if(row != 7 && column != 4) {
	man_board_status[Q(row,column)][BLACK_VIRGIN_KING+MAN_OFSET]
	  = IS_NOT_THERE;
      }
    }
  }
}

static void make_status_save()
{
  long int place,mi;
  for(place = 0; place < 8*8; place++) {
    for(mi = BLACK_VIRGIN_KING; mi <= WHITE_VIRGIN_KING; mi++) {
      backup_man_board_status[place][mi+MAN_OFSET]
	= man_board_status[place][mi+MAN_OFSET];
    }
  }
}

static void make_status_restore()
{
  long int place,mi;
  for(place = 0; place < 8*8; place++) {
    for(mi = BLACK_VIRGIN_KING; mi <= WHITE_VIRGIN_KING; mi++) {
      man_board_status[place][mi+MAN_OFSET]
	= backup_man_board_status[place][mi+MAN_OFSET];
    }
  }
}

static long int add_is_not_man(
  long int upd,
  long int place,
  long int man,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;

  switch(man_board_status[place][man+MAN_OFSET]) {
  case IS_UNCERTAIN:
    S(OP_IS_NOT_MAN);
    S(place);
    S(man);
    defer_address(dep_pp,head_p);
    S(0);
    if(upd) {
      place_man_at(GET_R(place),GET_C(place),man,IS_NOT_THERE);
    }
    break;
  case IS_THERE:
    S(OP_JUMP);
    defer_address(dep_pp,head_p);
    S(0);
    break;
  case IS_NOT_THERE:
    break;
  }

  return count;
}

#define ADD_IS_NOT_MAN(UPD,PLACE,MAN,DEP_PP) 	       \
{ 						       \
  long int d = add_is_not_man(UPD,PLACE,MAN,DEP_PP,head_p); \
  count += d; 					       \
  if(head_p) head_p += d; 			       \
}

static long int add_is_man(
  long int upd,
  long int place,
  long int man,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;

  switch(man_board_status[place][man+MAN_OFSET]) {
  case IS_UNCERTAIN:
    S(OP_IS_MAN);
    S(place);
    S(man);
    defer_address(dep_pp,head_p);
    S(0);
    if(upd) {
      place_man_at(GET_R(place),GET_C(place),man,IS_THERE);
    }
    break;
  case IS_THERE:
    break;
  case IS_NOT_THERE:
    S(OP_JUMP);
    defer_address(dep_pp,head_p);
    S(0);
    break;
  }

  return count;
}

#define ADD_IS_MAN(UPD,PLACE,MAN,DEP_PP) 	   \
{ 						   \
  long int d = add_is_man(UPD,PLACE,MAN,DEP_PP,head_p); \
  count += d; 					   \
  if(head_p) head_p += d; 			   \
}

static long int add_is_not_these_men(
  long int upd,
  long int place,
  long int man1,
  long int man2,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;

  switch(man_board_status[place][man1+MAN_OFSET]) {
  case IS_UNCERTAIN:
    switch(man_board_status[place][man2+MAN_OFSET]) {
    case IS_UNCERTAIN:
      S(OP_IS_NOT_THESE_MEN);
      S(place);
      S(man1);
      S(man2);
      defer_address(dep_pp,head_p);
      S(0);
      if(upd) {
	place_man_at(GET_R(place),GET_C(place),man1,IS_NOT_THERE);
	place_man_at(GET_R(place),GET_C(place),man2,IS_NOT_THERE);
      }
      break;
    case IS_THERE:
      S(OP_JUMP);
      defer_address(dep_pp,head_p);
      S(0);
      break;
    case IS_NOT_THERE:
      ADD_IS_NOT_MAN(upd,place,man1,dep_pp);
      break;
    }
    break;
  case IS_THERE:
    S(OP_JUMP);
    defer_address(dep_pp,head_p);
    S(0);
    break;
  case IS_NOT_THERE:
    ADD_IS_NOT_MAN(upd,place,man2,dep_pp);
    break;
  }

  return count;
}

#define ADD_IS_NOT_THESE_MEN(UPD,PLACE,MAN1,MAN2,DEP_PP) 	   \
{ 								   \
  long int d = add_is_not_these_men(UPD,PLACE,MAN1,MAN2,DEP_PP,head_p); \
  count += d; 							   \
  if(head_p) head_p += d; 					   \
}

static long int add_is_not_man_but_empty(
  long int upd,
  long int *done_p,
  long int place,
  long int man,
  DEPOSIT **d1_pp,
  DEPOSIT **d2_pp,
  long int *head_p
)
{
  long int count = 0;

  if(!*done_p) {
    if(man_board_status[place][EMPTY+MAN_OFSET] == IS_THERE) {
      *done_p = 2;
    } else {
      switch(man_board_status[place][man+MAN_OFSET]) {
      case IS_UNCERTAIN:
	S(OP_IS_NOT_MAN_BUT_EMPTY);
	S(place);
	S(man);
	defer_address(d1_pp,head_p);
	S(0);
	defer_address(d2_pp,head_p);
	S(0);
	if(upd) {
	  place_man_at(GET_R(place),GET_C(place),man,IS_NOT_THERE);
	}
	break;
      case IS_THERE:
	*done_p = 1;
	break;
      case IS_NOT_THERE:
	break;
      }
    }
  }

  return count;
}

#define ADD_IS_NOT_MAN_BUT_EMPTY(UPD,DP,PLACE,MAN,D1_PP,D2_PP) 		 \
{ 									 \
  long int d = add_is_not_man_but_empty(UPD,DP,PLACE,MAN,D1_PP,D2_PP,head_p); \
  count += d; 								 \
  if(head_p) head_p += d; 						 \
}

static long int add_is_not_these_men_but_empty(
  long int upd,
  long int *done_p,
  long int place,
  long int man1,
  long int man2,
  DEPOSIT **d1_pp,
  DEPOSIT **d2_pp,
  long int *head_p
)
{
  long int count = 0;

  if(!*done_p) {
    if(man_board_status[place][EMPTY+MAN_OFSET] == IS_THERE) {
      *done_p = 2;
    } else {
      switch(man_board_status[place][man1+MAN_OFSET]) {
      case IS_UNCERTAIN:
	switch(man_board_status[place][man2+MAN_OFSET]) {
	case IS_UNCERTAIN:
	  S(OP_IS_NOT_THESE_MEN_BUT_EMPTY);
	  S(place);
	  S(man1);
	  S(man2);
	  defer_address(d1_pp,head_p);
	  S(0);
	  defer_address(d2_pp,head_p);
	  S(0);
	  if(upd) {
	    place_man_at(GET_R(place),GET_C(place),man1,IS_NOT_THERE);
	    place_man_at(GET_R(place),GET_C(place),man2,IS_NOT_THERE);
	  }
	  break;
	case IS_THERE:
	  *done_p = 1;
	  break;
	case IS_NOT_THERE:
	  ADD_IS_NOT_MAN_BUT_EMPTY(upd,done_p,place,man1,d1_pp,d2_pp);
	  break;
	}
	break;
      case IS_THERE:
	*done_p = 1;
	break;
      case IS_NOT_THERE:
	ADD_IS_NOT_MAN_BUT_EMPTY(upd,done_p,place,man2,d1_pp,d2_pp);
	break;
      }
    }
  }

  return count;
}

#define ADD_IS_NOT_THESE_MEN_BUT_EMPTY(UPD,DP,PLACE,MAN1,MAN2,D1_PP,D2_PP) \
{ 									   \
  long int d = add_is_not_these_men_but_empty(UPD, 				   \
					 DP, 				   \
					 PLACE, 			   \
					 MAN1, 				   \
					 MAN2, 				   \
					 D1_PP, 			   \
					 D2_PP, 			   \
					 head_p); 			   \
  count += d; 								   \
  if(head_p) head_p += d; 						   \
}

static long int step_one_unthreat(
  long int dr,
  long int dc,
  long int man,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;

  if(0 <= dr && dr <= 7 && 0 <= dc && dc <= 7) {
    ADD_IS_NOT_MAN(1,Q(dr,dc),man,dep_pp);
  }

  return count;
}

#define STEP_ONE_UNTHREAT(DR,DC,MAN,DEP) 	   \
{ 						   \
  long int d = step_one_unthreat(DR,DC,MAN,DEP,head_p); \
  count += d; 					   \
  if(head_p) head_p += d; 			   \
}

static long int step_two_unthreat(
  long int dr,
  long int dc,
  long int man1,
  long int man2,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;

  if(0 <= dr && dr <= 7 && 0 <= dc && dc <= 7) {
    ADD_IS_NOT_THESE_MEN(1,Q(dr,dc),man1,man2,dep_pp);
  }

  return count;
}

#define STEP_TWO_UNTHREAT(DR,DC,MAN1,MAN2,DEP) 		 \
{ 							 \
  long int d = step_two_unthreat(DR,DC,MAN1,MAN2,DEP,head_p); \
  count += d; 						 \
  if(head_p) head_p += d; 				 \
}

static long int b_unthreat(
  long int r,
  long int c,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;
  DEPOSIT *short_coalease_p = NULL;
  long int *start_head_p = head_p;
  long int i;

  for(i = 0; i < 8; i++) {
    long int *vp = ray_board[Q(r,c)][i];

    if(vp[0]) {
      long int *pp;
      long int first = 1;
      long int done = 0;

      for(pp = vp; *pp; pp++);
      for(pp--; pp != vp && !done; pp--) {
	ADD_IS_NOT_THESE_MEN_BUT_EMPTY(
	  first,
	  &done,
	  GET_TO(*pp),
	  WHITE_QUEEN,
	  (i % 2 ? WHITE_BISHOP : WHITE_ROOK),
	  dep_pp,
	  &short_coalease_p);
	first = 0;
      }
      if(!done) {
	ADD_IS_NOT_THESE_MEN(
	  first,
	  GET_TO(vp[0]),
	  WHITE_QUEEN,
	  (i % 2 ? WHITE_BISHOP : WHITE_ROOK),
	  dep_pp);
      }

      if(done == 1) {
	head_p = start_head_p;
	count = 0;
	clear_all_above(&short_coalease_p,head_p);
	clear_all_above(dep_pp,head_p);
	S(OP_JUMP);
	deposit_address(dep_pp,head_p);
	S(0);
	return count;
      }
    }

    deposit_address(&short_coalease_p,head_p);
  }

  STEP_ONE_UNTHREAT(r+1,c+1,WHITE_KING,dep_pp);
  STEP_ONE_UNTHREAT(r+1,c,  WHITE_KING,dep_pp);
  STEP_ONE_UNTHREAT(r+1,c-1,WHITE_KING,dep_pp);
  STEP_ONE_UNTHREAT(r,  c+1,WHITE_KING,dep_pp);
  STEP_ONE_UNTHREAT(r,  c-1,WHITE_KING,dep_pp);
  STEP_TWO_UNTHREAT(r-1,c+1,WHITE_PAWN,WHITE_KING,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c,  WHITE_KING,dep_pp);
  STEP_TWO_UNTHREAT(r-1,c-1,WHITE_PAWN,WHITE_KING,dep_pp);

  STEP_ONE_UNTHREAT(r+1,c+2,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c+2,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r+1,c-2,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c-2,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r+2,c+1,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-2,c+1,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r+2,c-1,WHITE_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-2,c-1,WHITE_KNIGHT,dep_pp);

  return count;
}

#define B_UNTHREAT(R,C,DEP) 	      \
{ 				      \
  long int d = b_unthreat(R,C,DEP,head_p); \
  count += d; 			      \
  if(head_p) head_p += d; 	      \
}

static long int w_unthreat(
  long int r,
  long int c,
  DEPOSIT **dep_pp,
  long int *head_p
)
{
  long int count = 0;
  DEPOSIT *short_coalease_p = NULL;
  long int *start_head_p = head_p;
  long int i;

  for(i = 0; i < 8; i++) {
    long int *vp = ray_board[Q(r,c)][i];

    if(vp[0]) {
      long int *pp;
      long int first = 1;
      long int done = 0;

      for(pp = vp; *pp; pp++);
      for(pp--; pp != vp && !done; pp--) {
	ADD_IS_NOT_THESE_MEN_BUT_EMPTY(
	  first,
	  &done,
	  GET_TO(*pp),
	  BLACK_QUEEN,
	  (i % 2 ? BLACK_BISHOP : BLACK_ROOK),
	  dep_pp,
	  &short_coalease_p);
	first = 0;
      }
      if(!done) {
	ADD_IS_NOT_THESE_MEN(
	  first,
	  GET_TO(vp[0]),
	  BLACK_QUEEN,
	  (i % 2 ? BLACK_BISHOP : BLACK_ROOK),
	  dep_pp);
      }

      if(done == 1) {
	head_p = start_head_p;
	count = 0;
	clear_all_above(&short_coalease_p,head_p);
	clear_all_above(dep_pp,head_p);
	S(OP_JUMP);
	deposit_address(dep_pp,head_p);
	S(0);
	return count;
      }
    }

    deposit_address(&short_coalease_p,head_p);
  }

  STEP_TWO_UNTHREAT(r+1,c+1,BLACK_PAWN,BLACK_KING,dep_pp);
  STEP_ONE_UNTHREAT(r+1,c,  BLACK_KING,dep_pp);
  STEP_TWO_UNTHREAT(r+1,c-1,BLACK_PAWN,BLACK_KING,dep_pp);
  STEP_ONE_UNTHREAT(r,  c+1,BLACK_KING,dep_pp);
  STEP_ONE_UNTHREAT(r,  c-1,BLACK_KING,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c+1,BLACK_KING,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c,  BLACK_KING,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c-1,BLACK_KING,dep_pp);

  STEP_ONE_UNTHREAT(r+1,c+2,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c+2,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r+1,c-2,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-1,c-2,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r+2,c+1,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-2,c+1,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r+2,c-1,BLACK_KNIGHT,dep_pp);
  STEP_ONE_UNTHREAT(r-2,c-1,BLACK_KNIGHT,dep_pp);

  return count;
}

#define W_UNTHREAT(R,C,DEP) 	      \
{ 				      \
  long int d = w_unthreat(R,C,DEP,head_p); \
  count += d; 			      \
  if(head_p) head_p += d; 	      \
}

static long int action_for(
  long int row,
  long int column,
  long int man,
  long int *head_p,
  DEPOSIT **dep_pp,
  long int pending_jumps
)
{
  DEPOSIT *coalease_p = NULL;
  DEPOSIT *short_coalease_p = NULL;
  long int count = 0;

  if(pending_jumps) {
    S(OP_JUMP);
    S(0);
  }

  switch(man) {
  case BLACK_PAWN:
    if(row == 7 || row == 0) {
      break;

    } else if(row == 1) {
      if(0 < column) {
	S(OP_BLACK_PAWN_CAPTURE_RAISE);
	S(Q(row-1,column-1));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column-1),SPECIAL_QUEEN));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column-1),SPECIAL_ROOK));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column-1),SPECIAL_BISHOP));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column-1),SPECIAL_KNIGHT));
      }
      if(column < 7) {
	S(OP_BLACK_PAWN_CAPTURE_RAISE);
	S(Q(row-1,column+1));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column+1),SPECIAL_QUEEN));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column+1),SPECIAL_ROOK));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column+1),SPECIAL_BISHOP));
	S(MAKE_S_MOVE(Q(row,column),Q(row-1,column+1),SPECIAL_KNIGHT));
      }
      S(OP_BLACK_PAWN_MOVE_RAISE);
      S(Q(row-1,column));
      S(MAKE_S_MOVE(Q(row,column),Q(row-1,column),SPECIAL_QUEEN));
      S(MAKE_S_MOVE(Q(row,column),Q(row-1,column),SPECIAL_ROOK));
      S(MAKE_S_MOVE(Q(row,column),Q(row-1,column),SPECIAL_BISHOP));
      S(MAKE_S_MOVE(Q(row,column),Q(row-1,column),SPECIAL_KNIGHT));
    } else {
      if(0 < column) {
	if(row == 3) {
	  S(OP_BLACK_ENPASSANT);
	  S(Q(row-1,column-1));
	  S(MAKE_MOVE(Q(row,column),Q(row-1,column-1)));
	} else {
	  S(OP_BLACK_PAWN_CAPTURE);
	  S(Q(row-1,column-1));
	  S(MAKE_MOVE(Q(row,column),Q(row-1,column-1)));
	}
      }
      if(column < 7) {
	if(row == 3) {
	  S(OP_BLACK_ENPASSANT);
	  S(Q(row-1,column+1));
	  S(MAKE_MOVE(Q(row,column),Q(row-1,column+1)));
	} else {
	  S(OP_BLACK_PAWN_CAPTURE);
	  S(Q(row-1,column+1));
	  S(MAKE_MOVE(Q(row,column),Q(row-1,column+1)));
	}
      }
      if(row == 6) {
	S(OP_BLACK_PAWN_DOUBLE_MOVE);
	S(Q(row-1,column));
	S(MAKE_MOVE(Q(row,column),Q(row-1,column)));
	S(Q(row-2,column));
	S(MAKE_S_MOVE(Q(row,column),Q(row-2,column),SPECIAL_DOUBLE));
      } else {
	S(OP_BLACK_PAWN_MOVE);
	S(Q(row-1,column));
	S(MAKE_MOVE(Q(row,column),Q(row-1,column)));
      }
    }
    break;

  case BLACK_KNIGHT:
    SINGLE_MOVE(OP_BLACK_MOVE, 1, 2);
    SINGLE_MOVE(OP_BLACK_MOVE,-1, 2);
    SINGLE_MOVE(OP_BLACK_MOVE, 1,-2);
    SINGLE_MOVE(OP_BLACK_MOVE,-1,-2);
    SINGLE_MOVE(OP_BLACK_MOVE, 2, 1);
    SINGLE_MOVE(OP_BLACK_MOVE,-2, 1);
    SINGLE_MOVE(OP_BLACK_MOVE, 2,-1);
    SINGLE_MOVE(OP_BLACK_MOVE,-2,-1);
    break;

  case BLACK_BISHOP:
    RAY(1,black_ray_command,dep_pp);
    break;

  case BLACK_VIRGIN_ROOK:
    break;

  case BLACK_ROOK:
    RAY(0,black_ray_command,dep_pp);
    break;

  case BLACK_QUEEN:
    RAY(1,black_ray_command,&coalease_p);
    deposit_address(&coalease_p,head_p);
    break;

  case BLACK_VIRGIN_KING:
    if(row != 7 || column != 4) break;

    S(OP_CLEAR_FLAG);

    make_status_uncertain();
    place_man_at(7,4,BLACK_VIRGIN_KING,IS_THERE);

    ADD_IS_MAN(1,Q(7,5),EMPTY,&short_coalease_p);
    ADD_IS_MAN(1,Q(7,6),EMPTY,&short_coalease_p);
    ADD_IS_MAN(1,Q(7,7),BLACK_VIRGIN_ROOK,&short_coalease_p);

    make_status_uncertain();
    place_man_at(7,4,BLACK_VIRGIN_KING,IS_THERE);

    B_UNTHREAT(7,4,&coalease_p);

    S(OP_SET_FLAG);

    place_man_at(7,5,EMPTY,IS_THERE);
    place_man_at(7,6,EMPTY,IS_THERE);
    place_man_at(7,7,BLACK_VIRGIN_ROOK,IS_THERE);

    B_UNTHREAT(7,5,&short_coalease_p);
    S(OP_BLACK_MOVE);
    S(Q(7,6));
    S(MAKE_S_MOVE(Q(7,4),Q(7,6),SPECIAL_S_CASTLING));

    deposit_address(&short_coalease_p,head_p);

    make_status_uncertain();
    place_man_at(7,4,BLACK_VIRGIN_KING,IS_THERE);

    ADD_IS_MAN(1,Q(7,3),EMPTY,&coalease_p);
    ADD_IS_MAN(1,Q(7,2),EMPTY,&coalease_p);
    ADD_IS_MAN(1,Q(7,1),EMPTY,&coalease_p);
    ADD_IS_MAN(1,Q(7,0),BLACK_VIRGIN_ROOK,&coalease_p);

    S(OP_JUMP_SET);
    defer_address(&short_coalease_p,head_p);
    S(0);

    B_UNTHREAT(7,4,&coalease_p);

    deposit_address(&short_coalease_p,head_p);

    B_UNTHREAT(7,3,&coalease_p);
    S(OP_BLACK_MOVE);
    S(Q(7,2));
    S(MAKE_S_MOVE(Q(7,4),Q(7,2),SPECIAL_L_CASTLING));

    deposit_address(&coalease_p,head_p);
    break;

  case BLACK_KING:
    SINGLE_MOVE(OP_BLACK_MOVE, 1, 1);
    SINGLE_MOVE(OP_BLACK_MOVE, 1, 0);
    SINGLE_MOVE(OP_BLACK_MOVE, 1,-1);
    SINGLE_MOVE(OP_BLACK_MOVE, 0, 1);
    SINGLE_MOVE(OP_BLACK_MOVE, 0,-1);
    SINGLE_MOVE(OP_BLACK_MOVE,-1, 1);
    SINGLE_MOVE(OP_BLACK_MOVE,-1, 0);
    SINGLE_MOVE(OP_BLACK_MOVE,-1,-1);
    break;

  case WHITE_PAWN:
    if(row == 7 || row == 0) {
      break;

    } else if(row == 6) {
      if(0 < column) {
	S(OP_WHITE_PAWN_CAPTURE_RAISE);
	S(Q(row+1,column-1));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column-1),SPECIAL_QUEEN));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column-1),SPECIAL_ROOK));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column-1),SPECIAL_BISHOP));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column-1),SPECIAL_KNIGHT));
      }
      if(column < 7) {
	S(OP_WHITE_PAWN_CAPTURE_RAISE);
	S(Q(row+1,column+1));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column+1),SPECIAL_QUEEN));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column+1),SPECIAL_ROOK));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column+1),SPECIAL_BISHOP));
	S(MAKE_S_MOVE(Q(row,column),Q(row+1,column+1),SPECIAL_KNIGHT));
      }
      S(OP_WHITE_PAWN_MOVE_RAISE);
      S(Q(row+1,column));
      S(MAKE_S_MOVE(Q(row,column),Q(row+1,column),SPECIAL_QUEEN));
      S(MAKE_S_MOVE(Q(row,column),Q(row+1,column),SPECIAL_ROOK));
      S(MAKE_S_MOVE(Q(row,column),Q(row+1,column),SPECIAL_BISHOP));
      S(MAKE_S_MOVE(Q(row,column),Q(row+1,column),SPECIAL_KNIGHT));
    } else {
      if(0 < column) {
	if(row == 4) {
	  S(OP_WHITE_ENPASSANT);
	  S(Q(row+1,column-1));
	  S(MAKE_MOVE(Q(row,column),Q(row+1,column-1)));
	} else {
	  S(OP_WHITE_PAWN_CAPTURE);
	  S(Q(row+1,column-1));
	  S(MAKE_MOVE(Q(row,column),Q(row+1,column-1)));
	}
      }
      if(column < 7) {
	if(row == 4) {
	  S(OP_WHITE_ENPASSANT);
	  S(Q(row+1,column+1));
	  S(MAKE_MOVE(Q(row,column),Q(row+1,column+1)));
	} else {
	  S(OP_WHITE_PAWN_CAPTURE);
	  S(Q(row+1,column+1));
	  S(MAKE_MOVE(Q(row,column),Q(row+1,column+1)));
	}
      }
      if(row == 1) {
	S(OP_WHITE_PAWN_DOUBLE_MOVE);
	S(Q(row+1,column));
	S(MAKE_MOVE(Q(row,column),Q(row+1,column)));
	S(Q(row+2,column));
	S(MAKE_S_MOVE(Q(row,column),Q(row+2,column),SPECIAL_DOUBLE));
      } else {
	S(OP_WHITE_PAWN_MOVE);
	S(Q(row+1,column));
	S(MAKE_MOVE(Q(row,column),Q(row+1,column)));
      }
    }
    break;

  case WHITE_KNIGHT:
    SINGLE_MOVE(OP_WHITE_MOVE, 1, 2);
    SINGLE_MOVE(OP_WHITE_MOVE,-1, 2);
    SINGLE_MOVE(OP_WHITE_MOVE, 1,-2);
    SINGLE_MOVE(OP_WHITE_MOVE,-1,-2);
    SINGLE_MOVE(OP_WHITE_MOVE, 2, 1);
    SINGLE_MOVE(OP_WHITE_MOVE,-2, 1);
    SINGLE_MOVE(OP_WHITE_MOVE, 2,-1);
    SINGLE_MOVE(OP_WHITE_MOVE,-2,-1);
    break;

  case WHITE_BISHOP:
    RAY(1,white_ray_command,dep_pp);
    break;

  case WHITE_VIRGIN_ROOK:
    break;

  case WHITE_ROOK:
    RAY(0,white_ray_command,dep_pp);
    break;

  case WHITE_QUEEN:
    RAY(1,white_ray_command,&coalease_p);
    deposit_address(&coalease_p,head_p);
    break;

  case WHITE_VIRGIN_KING:
    if(row != 0 || column != 4) break;

    S(OP_CLEAR_FLAG);

    make_status_uncertain();
    place_man_at(0,4,WHITE_VIRGIN_KING,IS_THERE);

    ADD_IS_MAN(1,Q(0,5),EMPTY,&short_coalease_p);
    ADD_IS_MAN(1,Q(0,6),EMPTY,&short_coalease_p);
    ADD_IS_MAN(1,Q(0,7),WHITE_VIRGIN_ROOK,&short_coalease_p);

    make_status_uncertain();
    place_man_at(0,4,WHITE_VIRGIN_KING,IS_THERE);

    W_UNTHREAT(0,4,&coalease_p);

    S(OP_SET_FLAG);

    place_man_at(0,5,EMPTY,IS_THERE);
    place_man_at(0,6,EMPTY,IS_THERE);
    place_man_at(0,7,WHITE_VIRGIN_ROOK,IS_THERE);

    W_UNTHREAT(0,5,&short_coalease_p);
    S(OP_WHITE_MOVE);
    S(Q(0,6));
    S(MAKE_S_MOVE(Q(0,4),Q(0,6),SPECIAL_S_CASTLING));

    deposit_address(&short_coalease_p,head_p);

    make_status_uncertain();
    place_man_at(0,4,WHITE_VIRGIN_KING,IS_THERE);

    ADD_IS_MAN(1,Q(0,3),EMPTY,&coalease_p);
    ADD_IS_MAN(1,Q(0,2),EMPTY,&coalease_p);
    ADD_IS_MAN(1,Q(0,1),EMPTY,&coalease_p);
    ADD_IS_MAN(1,Q(0,0),WHITE_VIRGIN_ROOK,&coalease_p);

    S(OP_JUMP_SET);
    defer_address(&short_coalease_p,head_p);
    S(0);

    W_UNTHREAT(0,4,&coalease_p);

    deposit_address(&short_coalease_p,head_p);

    W_UNTHREAT(0,3,&coalease_p);
    S(OP_WHITE_MOVE);
    S(Q(0,2));
    S(MAKE_S_MOVE(Q(0,4),Q(0,2),SPECIAL_L_CASTLING));

    deposit_address(&coalease_p,head_p);
    break;

  case WHITE_KING:
    SINGLE_MOVE(OP_WHITE_MOVE, 1, 1);
    SINGLE_MOVE(OP_WHITE_MOVE, 1, 0);
    SINGLE_MOVE(OP_WHITE_MOVE, 1,-1);
    SINGLE_MOVE(OP_WHITE_MOVE, 0, 1);
    SINGLE_MOVE(OP_WHITE_MOVE, 0,-1);
    SINGLE_MOVE(OP_WHITE_MOVE,-1, 1);
    SINGLE_MOVE(OP_WHITE_MOVE,-1, 0);
    SINGLE_MOVE(OP_WHITE_MOVE,-1,-1);
    break;

  default:
    /* Skip holes */
    break;
  }

  return count;
}

static long int init_pice_pathes(
  long int *head_p
)
{
  long int count = 0;
  long int for_white,row,column,lp_man;
  long int *vector_p;
  DEPOSIT *dep_p = NULL;
  long int pending_jumps;

  for(for_white = 0; for_white < 2; for_white++) {
    actions[for_white] = head_p;
    for(row = (for_white ? 7 : 0);
	(for_white ? row >= 0 : row < 8);
	row = (for_white ? row-1 : row+1))
    {
      for(column = 0; column < 8; column++) {
	spot_action[for_white][Q(row,column)] = head_p;
	S(OP_READ_MAN);
	S(Q(row,column));
	vector_p = head_p +MAN_OFSET;
	count += WHITE_VIRGIN_KING - BLACK_VIRGIN_KING + 1;
	if(head_p) head_p = &vector_p[WHITE_VIRGIN_KING] + 1;

	defer_address(&dep_p, head_p ? &vector_p[EMPTY] : NULL);

	pending_jumps = 0;
	for(lp_man = WHITE_VIRGIN_KING; lp_man > EMPTY; lp_man--) {
	  long int man = (for_white ? lp_man : -lp_man);
	  long int *start_head_p = head_p;
	  long int d = action_for(row,column,man,head_p,&dep_p,pending_jumps);

	  if(d > 2) { /* skip lonely jump */
	    if(head_p) head_p += d;
	    count += d;
	    if(start_head_p) {
	      if(pending_jumps) {
		vector_p[man] = (long int) (start_head_p + 2);
		defer_address(&dep_p, start_head_p + 1);
	      } else {
		vector_p[man] = (long int) start_head_p;
	      }
	    }
	    
	    if(man == WHITE_VIRGIN_KING ||
	       man == BLACK_VIRGIN_KING ||
	       man == WHITE_QUEEN ||
	       man == BLACK_QUEEN)
	    {
	      pending_jumps = 0;
	    } else {
	      pending_jumps = 1;
	    }
	    
	  } else {
	    defer_address(&dep_p, head_p ? &vector_p[man] : NULL);
	  }

	  defer_address(&dep_p, head_p ? &vector_p[-man] : NULL);
	}

	deposit_address(&dep_p,head_p);

	if(head_p && (column == 0 || column == 7)) {
	  if(row == 0) {
	    vector_p[WHITE_VIRGIN_ROOK] = vector_p[WHITE_ROOK];
	  } else if(row == 7) {
	    vector_p[BLACK_VIRGIN_ROOK] = vector_p[BLACK_ROOK];
	  }
	}
      }
    }
    S(OP_DONE);
    S(OP_DONE); /* In case of speeding ;-) */
  }

  return count;
}
/*
     2
   3   1
 4       0
   5   7
     6
*/

static long int create_one_ray(
  long int dr,
  long int dc,
  long int row,
  long int column,
  long int *head_p
)
{
  long int count = 0;
  long int r = row;
  long int c = column;

  while(0 <= r && r <= 7 && 0 <= c && c <= 7) {
    r += dr;
    c += dc;
  }
  r -= dr;
  c -= dc;

  while(r != row || c != column) {
    S(MAKE_MOVE(Q(row,column),Q(r,c)));
    r -= dr;
    c -= dc;
  }
  S(0);

  return count;
}

#define CREATE_ONE_RAY(R,DR,DC) 	       \
{ 					       \
  long int d; 				       \
  R = head_p; 				       \
  d = create_one_ray(DR,DC,row,column,head_p); \
  count += d; 				       \
  if(head_p) head_p += d; 		       \
}


static long int create_star_rays(
  long int row,
  long int column,
  long int *head_p,
  long int *rb[8]
)
{
  long int count = 0;

  CREATE_ONE_RAY(rb[0], 0, 1);
  CREATE_ONE_RAY(rb[1], 1, 1);
  CREATE_ONE_RAY(rb[2], 1, 0);
  CREATE_ONE_RAY(rb[3], 1,-1);
  CREATE_ONE_RAY(rb[4], 0,-1);
  CREATE_ONE_RAY(rb[5],-1,-1);
  CREATE_ONE_RAY(rb[6],-1, 0);
  CREATE_ONE_RAY(rb[7],-1, 1);

  return count;
}

static long int init_rays(
  long int *head_p
)
{
  long int count = 0;
  long int row,column;

  S(0);
  for(row = 0; row < 8; row++) {
    for(column = 0; column < 8; column++) {
      long int d = create_star_rays(row,column,head_p,ray_board[Q(row,column)]);
      if(head_p) head_p += d;
      count += d;
    }
  }

  return count;
}

void init_moves()
{
  long int *ray_ptr;
  long int *op_ptr;
  long int count;
#ifdef vms
  long int *addr[2];
  long int sys$lkwset(long int **,long int **,long int);
#endif

  count  = init_rays(NULL);
  ray_ptr = (long int *) malloc(count*sizeof(long int));
  init_rays(ray_ptr);

  count = init_pice_pathes(NULL);
  op_ptr = (long int *) malloc(count*sizeof(long int));

#ifdef vms
  addr[0] = &op_ptr[0];
  addr[1] = &op_ptr[count];
  if(sys$lkwset(addr,NULL,0) & 1 == 0) {
    printf("Impossible to lock op-code in memory\n");
  }
#endif

  init_pice_pathes(op_ptr);

  free(ray_ptr);
  memset(ray_board,0,sizeof ray_board);
}

#ifdef STANDALONE

long int number_of_errors = 0;
long int from = -1;
long int to = -1;

static void local_board_convert(
  long int place
)
{
  if(!board_convert(place)) number_of_errors++;
  to = place;
}

static void local_move_convert(
  long int move
)
{
  if(!move_convert(move)) number_of_errors++;

  if(from != GET_FROM(move)) {
    printf(" f_err ");
    number_of_errors++;
  }
  if(to != GET_TO(move)) {
    printf(" t_err ");
    number_of_errors++;
  }
}

static void local_man_convert(
  long int man
)
{
  if(!man_convert(man)) {
    if(man < BLACK_VIRGIN_KING || WHITE_VIRGIN_KING < man) {
      number_of_errors++;
    }
  }
  printf(" ");
}

static long int push_destination(
  DEPOSIT **dep_pp,
  long int place,
  long int *next_label_p
)
{
  if(*dep_pp == NULL ||
     (unsigned long int) place < (unsigned long int) (*dep_pp)->place_p)
  {
    DEPOSIT *new_dep_p = (DEPOSIT *) malloc(sizeof(DEPOSIT));

    new_dep_p->place_p = (long int *) place;
    new_dep_p->label = (*next_label_p)++;
    new_dep_p->next_p = (*dep_pp);
    *dep_pp = new_dep_p;

  } else if((long int *) place != (*dep_pp)->place_p) {
    return push_destination(&(*dep_pp)->next_p,place,next_label_p);
  }

  to = -1;
  return (*dep_pp)->label;
}

static void print_label(
  long int *action_p,
  DEPOSIT **dep_pp
)
{
  if(*dep_pp == NULL ||
     (unsigned long int) (*dep_pp)->place_p > (unsigned long int) action_p)
  {
    printf("          ");

  } else {
    DEPOSIT *next_dep_p = (*dep_pp)->next_p;

    printf("%5d",(*dep_pp)->label);

    if((*dep_pp)->place_p == action_p) {
      printf("     ");
    } else {
      printf(" err ");
      number_of_errors++;
    }

    free(*dep_pp);
    *dep_pp = next_dep_p;
  }
}

static void decompile(
  long int *action_p
)
{
  long int label = 0;
  DEPOSIT *dep_p = NULL;

  while(1) {
    print_label(action_p,&dep_p);
    switch(*action_p++) {
    case OP_DONE:
      printf("done\n");
      return;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP:
      printf("jump ->%ld\n",push_destination(&dep_p,*action_p++,&label));
      break;

/*
  | OP |
->| OP |
*/
    case OP_SET_FLAG:
      printf("Set flag\n");
      break;

/*
  | OP |
->| OP |
*/
    case OP_CLEAR_FLAG:
      printf("Clear flag\n");
      break;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP_SET:
      printf("jump_set ->%ld\n",push_destination(&dep_p,*action_p++,&label));
      break;

/*
  | OP |
->| .  |--\
  z    z  |
  | OP |<-/
*/
    case OP_JUMP_CLEAR:
      printf("jump_clear ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
    case OP_READ_MAN: {
      long int man;

      printf("READ ");
      local_board_convert(*action_p++);
      from = to;
      to = -1;
      printf(" {\n");
      for(man = BLACK_VIRGIN_KING; man <= WHITE_VIRGIN_KING; man++) {
	printf("          ");
	local_man_convert(man);
	printf(": ->%ld\n",push_destination(&dep_p,*action_p++,&label));
      }
      printf("          ");
      printf("}\n");
    } break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_CAPTURE:
      printf("WHITE_PAWN_CAPTURE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_CAPTURE:
      printf("BLACK_PAWN_CAPTURE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_PAWN_MOVE:
      printf("WHITE_PAWN_MOVE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_PAWN_MOVE:
      printf("BLACK_PAWN_MOVE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("WHITE_PAWN_MOVE_RAISE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("BLACK_PAWN_MOVE_RAISE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("BLACK_PAWN_CAPTURE_RAISE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("WHITE_PAWN_CAPTURE_RAISE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(",");
      local_move_convert(*action_p++);
      printf(")\n");
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_ENPASSANT:
      printf("WHITE_ENPASSANT ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_ENPASSANT:
      printf("BLACK_ENPASSANT ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("WHITE_PAWN_DOUBLE_MOVE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("BLACK_PAWN_DOUBLE_MOVE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("BLACK_P_RAY ");

      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("BLACK_R_RAY ");

      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("BLACK_B_RAY ");

      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("WHITE_P_RAY ");

      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("WHITE_R_RAY ");

      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("WHITE_B_RAY ");

      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(") ->%ld\n",push_destination(&dep_p,*action_p++,&label));
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_BLACK_MOVE:
      printf("BLACK_MOVE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
      break;

/*
  | OP |
->|dest|
  |move|
  | OP |
*/
    case OP_WHITE_MOVE:
      printf("WHITE_MOVE ");
      local_board_convert(*action_p++);
      printf(" (");
      local_move_convert(*action_p++);
      printf(")\n");
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
      printf("IS_NOT_MAN ");
      local_board_convert(*action_p++);
      printf(" [");
      local_man_convert(*action_p++);
      printf("] ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("IS_NOT_THESE_MEN ");
      local_board_convert(*action_p++);
      printf(" [");
      local_man_convert(*action_p++);
      printf(",");
      local_man_convert(*action_p++);
      printf("] ->%ld\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("IS_NOT_MAN_BUT_EMPTY ");
      local_board_convert(*action_p++);
      printf(" [");
      local_man_convert(*action_p++);
      printf("] ->(%ld,",push_destination(&dep_p,*action_p++,&label));
      printf("%ld)\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("IS_NOT_THESE_MEN_BUT_EMPTY ");
      local_board_convert(*action_p++);
      printf(" [");
      local_man_convert(*action_p++);
      printf(",");
      local_man_convert(*action_p++);
      printf("] ->(%ld,",push_destination(&dep_p,*action_p++,&label));
      printf("%ld)\n",push_destination(&dep_p,*action_p++,&label));
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
      printf("IS_MAN ");
      local_board_convert(*action_p++);
      printf(" [");
      local_man_convert(*action_p++);
      printf("] ->%ld\n",push_destination(&dep_p,*action_p++,&label));
      break;

    default:
      printf("Strange OP code %ld\n",action_p[-1]);
      number_of_errors++;
    }
  }
}

main()
{
  init_moves();

  printf("\nMoves for white\n");
  decompile(actions[1]);

  printf("Moves for black\n");
  decompile(actions[0]);

  if(number_of_errors) {
    printf("number_of_errors: %ld\n",number_of_errors);
  } else {
    printf("No errors detected\n");
  }
}

#endif /* STANDALONE */
