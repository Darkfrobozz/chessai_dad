/* loop.c -- by Vaino Hassinen */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "chess.h"

static void update_princ(long int,long int);
static long int alpha_body(long int,long int,long int,long int,long int *);
static long int beta_body(long int,long int,long int,long int,long int *);
static void alpha_descend(long int,long int,long int);
static void beta_descend(long int,long int,long int);
static void first_alpha_descend(long int,long int);
static void first_beta_descend(long int,long int);

static long int pc[MAX_PLY][MAX_PLY];
static long int more;

static long int timeout_vector[4];
static long int timeout_level;
static long int timeout_delta[4];

static long int start_material_balance;

void ttupdate(
  long int depth_precision,
  long int value,
  long int move,
  long int growth
)
{
  if(sit_p->tt_p->checkpoint < sit_p->save.checkpoint_level
     || depth_precision >  sit_p->tt_p->depth_precision
     || depth_precision == sit_p->tt_p->depth_precision
        && (growth ? value > sit_p->tt_p->value : value < sit_p->tt_p->value))
  {
    sit_p->tt_p->key = 0;  /* If this routine is asynchronously interuppted */
    sit_p->tt_p->value = value;
    sit_p->tt_p->checkpoint = sit_p->save.checkpoint;
    sit_p->tt_p->move = move;
    sit_p->tt_p->depth_precision = depth_precision;
    sit_p->tt_p->key = sit_p->ttkey;
  }
}

static void timeout_white(
  int parameter
)
{
  long int delta_score_0 = accepted_score - start_material_balance;

  timeout_delta[timeout_level] = delta_score_0;

  switch(timeout_level) {
  case 0:  /* B/2 */
    if(delta_score_0 >= (VALUE_WHITE_QUEEN - VALUE_WHITE_PAWN
                         + VALUE_BLACK_PAWN/2))
    {
      more = 0;
    }
    break;
  case 1:  /* B */
    if(delta_score_0 >= VALUE_BLACK_PAWN/2)
    {
      more = 0;
    }
    break;
  case 2:  /* 2B */
    if(delta_score_0 >= (-VALUE_WHITE_BISHOP + VALUE_BLACK_PAWN/2))
    {
      more = 0;
    }
    break;
  default: /* 4B */
    more = 0;
    break;
  }

  if(more) {
    timeout_level++;
    signal(SIGALRM,timeout_white);
    alarm(timeout_vector[timeout_level]);
  } else if(accepted_pc[0] == 0) {
    more = -1;
  }
}

static void timeout_black(
  int parameter
)
{
  long int delta_score_0 = accepted_score - start_material_balance;

  timeout_delta[timeout_level] = delta_score_0;

  switch(timeout_level) {
  case 0:  /* B/2 */
    if(delta_score_0 <= (VALUE_BLACK_QUEEN - VALUE_BLACK_PAWN
                         + VALUE_WHITE_PAWN/2))
    {
      more = 0;
    }
    break;
  case 1:  /* B */
    if(delta_score_0 <= VALUE_WHITE_PAWN/2)
    {
      more = 0;
    }
    break;
  case 2:  /* 2B */
    if(delta_score_0 <= (-VALUE_BLACK_BISHOP + VALUE_WHITE_PAWN/2))
    {
      more = 0;
    }
    break;
  default: /* 4B */
    more = 0;
    break;
  }

  if(more) {
    timeout_level++;
    signal(SIGALRM,timeout_black);
    alarm(timeout_vector[timeout_level]);
  } else if(accepted_pc[0] == 0) {
    more = -1;
  }
}

void timeout_abort()
{
  more = 0;
}

void timeout_unlimited()
{
  more = 1;
}

void timeout_alter(
  long int playing_white,
  long int remaining_time,
  long int remaining_moves,
  long int used_time
)
{
  long int i;

  /* Make certain to not drop out */
  if(remaining_time < 3) {
    remaining_time = 1;
  } else {
    remaining_time -= 2;
  }

  timeout_vector[0] = (1+remaining_time/remaining_moves)/2;
  timeout_vector[1] = remaining_time/remaining_moves;
  timeout_vector[2] = 2*remaining_time/remaining_moves;
  timeout_vector[3] = 4*remaining_time/remaining_moves;

  /* Maintain some time for the remaining moves */
  remaining_time -= (remaining_moves-1)*timeout_vector[0]; 
  for(i = 0; i < 4; i++) {
    timeout_vector[i] -= used_time;

    if(timeout_vector[i] > remaining_time) {
      timeout_vector[i] = remaining_time;
    }
  }

  timeout_level = 0;
  for(i = 3; i >= 0; i--) {
    if(i > 0) {
      timeout_vector[i] -= timeout_vector[i-1];
    }

    if(timeout_vector[i] <= 0) timeout_level++;
  }

  if(timeout_level) {
    long int j = 3;

    for(i = 3; i >= 0; i--) {
      if(timeout_vector[i] > 0) {
	timeout_vector[j--] = timeout_vector[i];
      }
    }
  }

  if(timeout_level < 4) {
    more = 1;
    signal(SIGALRM,(playing_white ? timeout_white : timeout_black));
    alarm(timeout_vector[timeout_level]);

  } else if(accepted_pc[0] == 0) {
    more = -1;

  } else {
    more = 0;
  }
}

void loop(
  long int playing_white,
  long int last_move
)
{
  SITUATION *start_sit_p;
  long int dmax;
  long int i;
  long int start_dmax;

  start_material_balance = sit_p->save.material_balance;
  printf("Beginning material %4.1f\n",
	 (start_material_balance*1.0)/VALUE_WHITE_PAWN);

  start_dmax = 2;
  sit_p->score_0 = *sit_p->score_1_p;

  if(playing_white) {
    *sit_p->score_2_p = MIN_VALUE;
    *sit_p->score_1_p = MAX_VALUE;
  } else {
    *sit_p->score_2_p = MAX_VALUE;
    *sit_p->score_1_p = MIN_VALUE;
  }

  generate_moves(playing_white);

  start_sit_p = sit_p;
  if(playing_white) {
    for(dmax = start_dmax; more; dmax++) {
      if(accepted_pc[0]) {
	butterfly_board[GET_BUTTERFLY(last_move)] = accepted_pc[0];
	for(i = 1; i < MAX_PLY && accepted_pc[i]; i++) {
	  butterfly_board[GET_BUTTERFLY(accepted_pc[i-1])] = accepted_pc[i];
	}
      }
      if(dmax > start_dmax) sort_for_white();
      first_alpha_descend(dmax,last_move);
    }
  } else {
    for(dmax = start_dmax; more; dmax++) {
      if(accepted_pc[0]) {
	butterfly_board[GET_BUTTERFLY(last_move)] = accepted_pc[0];
	for(i = 1; i < MAX_PLY && accepted_pc[i]; i++) {
	  butterfly_board[GET_BUTTERFLY(accepted_pc[i-1])] = accepted_pc[i];
	}
      }
      if(dmax > start_dmax) sort_for_black();
      first_beta_descend(dmax,last_move);
    }
  }
  sit_p = start_sit_p;
  sit_p->score_0 = accepted_score;

  printf("Timeout level %ld\n",timeout_level);
}

static void update_princ(
  long int ply,
  long int move
)
{
  register long int j;

  pc[ply][ply] = move;
  for(j = ply + 1; pc[ply][j] = pc[ply+1][j]; j++);

  if(!ply) {
    accepted_score = sit_p->score_0;

    if(accepted_score < -2*(VALUE_WHITE_KING) + MAX_PLY) {
      printf("  BM(%ld) ",accepted_score + 2*(VALUE_WHITE_KING));

    } else if(accepted_score < -(VALUE_WHITE_KING)/2) {
      printf("  BM(?) ");

    } else if(accepted_score > -2*(VALUE_BLACK_KING) - MAX_PLY) {
      printf("  WM(%ld) ",-(accepted_score + 2*(VALUE_BLACK_KING)));

    } else if(accepted_score > -(VALUE_BLACK_KING)/2) {
      printf("  WM(?) ");

    } else {
      printf("%7.3f ",(accepted_score*1.0)/VALUE_WHITE_PAWN);
    }

    for(j = 0; accepted_pc[j] = pc[0][j]; j++) {
      move_convert(pc[0][j]);
      printf(" ");
    }

    printf("\n");

    if(more < 0) more = 0;
  }
}

static long int alpha_body(
  long int ply,
  long int dmax,
  long int move,
  long int last_move,
  long int *pat_possible_p
)
{
  long int pre_material_balance = sit_p->save.material_balance;

  switch(do_move(move,ply)) {
  case 1:
    if(pre_material_balance != start_material_balance &&
       sit_p->save.material_balance == start_material_balance)
    {
      beta_descend(ply+1,dmax+1,move);
    } else {
      beta_descend(ply+1,dmax,move);
    }

    if(*pat_possible_p) {
      if(sit_p->score_0 != -2*(VALUE_WHITE_KING) + ply) {
	*pat_possible_p = 0;
      }
    }

    if(sit_p->tt_p->key == sit_p->ttkey &&
       sit_p->tt_p->depth_precision == DEPTH_PRECISION_RECURSION)
    {
      sit_p->score_0   = sit_p->tt_p->value;
      pc[ply+1][ply+1] = sit_p->tt_p->move;
      pc[ply+1][ply+2] = 0;
    }
    break;

  case 2: /* Black king was taken of board */
    pc[ply+1][ply+1] = 0;
    sit_p->score_0 = -2*(VALUE_BLACK_KING) - (ply-1);
    *pat_possible_p = 0;
    break;

  default:
    pc[ply+1][ply+1] = 0;
    sit_p->score_0 = -(VALUE_WHITE_KING)/2; /* Illegal move */
    break;
  }

  if(have_message > message_checked) check_message();
  if(!more) return 0;

  if(sit_p->score_0 > *sit_p->score_1_p) {
    *sit_p->score_1_p = sit_p->score_0;
    update_princ(ply,move);
    sit_pop();
    ttupdate(MAKE_DEPTH_PRECISION(dmax-ply,ESTIMATE),
	     sit_p->score_0,
	     move,
	     1);

  } else {
    sit_pop();
  }

  if(sit_p->score_0 >= *sit_p->score_1_p) {
    pc[ply][ply] = move;
    pc[ply][ply+1] = 0;
    butterfly_board[GET_BUTTERFLY(last_move)] = move;
    ttupdate(MAKE_DEPTH_PRECISION(dmax-ply,ESTIMATE),
	     sit_p->score_0,
	     move,
	     1);
    return 0;
  }

  return 1;
}

static long int beta_body(
  long int ply,
  long int dmax,
  long int move,
  long int last_move,
  long int *pat_possible_p
)
{
  long int pre_material_balance = sit_p->save.material_balance;

  switch(do_move(move,ply)) {
  case 1:
    if(pre_material_balance != start_material_balance &&
       sit_p->save.material_balance == start_material_balance)
    {
      alpha_descend(ply+1,dmax+1,move);
    } else {
      alpha_descend(ply+1,dmax,move);
    }

    if(*pat_possible_p) {
      if(sit_p->score_0 != -2*(VALUE_BLACK_KING) - ply) {
	*pat_possible_p = 0;
      }
    }

    if(sit_p->tt_p->key == sit_p->ttkey &&
       sit_p->tt_p->depth_precision == DEPTH_PRECISION_RECURSION)
    {
      sit_p->score_0   = sit_p->tt_p->value;
      pc[ply+1][ply+1] = sit_p->tt_p->move;
      pc[ply+1][ply+2] = 0;
    }
    break;

  case 2: /* White king was taken of board */
    pc[ply+1][ply+1] = 0;
    sit_p->score_0 = -2*(VALUE_WHITE_KING) + (ply-1);
    *pat_possible_p = 0;
    break;

  default:
    pc[ply+1][ply+1] = 0;
    sit_p->score_0 = -(VALUE_BLACK_KING)/2; /* Illegal move */
    break;
  }

  if(have_message > message_checked) check_message();
  if(!more) return 0;

  if(sit_p->score_0 < *sit_p->score_1_p) {
    *sit_p->score_1_p = sit_p->score_0;
    update_princ(ply,move);
    sit_pop();
    ttupdate(MAKE_DEPTH_PRECISION(dmax-ply,ESTIMATE),
	     sit_p->score_0,
	     move,
	     0);
  } else {
    sit_pop();
  }

  if(sit_p->score_0 <= *sit_p->score_1_p) {
    pc[ply][ply] = move;
    pc[ply][ply+1] = 0;
    butterfly_board[GET_BUTTERFLY(last_move)] = move;
    ttupdate(MAKE_DEPTH_PRECISION(dmax-ply,ESTIMATE),
	     sit_p->score_0,
	     move,
	     0);
    return 0;
  }

  return 1;
}

static void first_alpha_descend(
  long int dmax,
  long int last_move
)
{
  long int *mp;
  long int *vp;
  long int killer_score,gm_score;
  long int pat_possible = 1;
  long int gm_move = (sit_p->tt_p->key == sit_p->ttkey ? sit_p->tt_p->move : 0);
  long int killer_move = butterfly_board[GET_BUTTERFLY(last_move)];

  sit_p->score_0 = *sit_p->score_2_p;
    
  if(gm_move && is_legal_move(1,gm_move)) {
    if(gm_move == killer_move) killer_move = 0;
    if(!alpha_body(0,dmax,gm_move,last_move,&pat_possible)) return;
    gm_score = sit_p->next_p->score_0;
  }

  if(killer_move && is_legal_move(1,killer_move)) {
    if(!alpha_body(0,dmax,killer_move,last_move,&pat_possible)) return;
    killer_score = sit_p->next_p->score_0;
  }

  mp = sit_p->moves;
  vp = sit_p->scores;
  if(*mp) {
    for(; *mp; mp++,vp++) {
      if(gm_move == *mp) {
	*vp = gm_score;
      } else if(killer_move == *mp) {
	*vp = killer_score;
      } else {
	if(!alpha_body(0,dmax,*mp,last_move,&pat_possible)) return;
	*vp = sit_p->next_p->score_0;
      }
    }

    if(pat_possible) {
      pat_possible = cannot_strike_at(0,sit_p->save.white_king_place);
    }
  }

  if(pat_possible) {
    accepted_pc[0] = 0;
    accepted_score = 0; /* Stale mate */
    alarm(0);
    more = 0;
  }
}

static void first_beta_descend(
  long int dmax,
  long int last_move
)
{
  long int *mp;
  long int *vp;
  long int killer_score,gm_score;
  long int pat_possible = 1;
  long int gm_move = (sit_p->tt_p->key == sit_p->ttkey ? sit_p->tt_p->move : 0);
  long int killer_move = butterfly_board[GET_BUTTERFLY(last_move)];

  sit_p->score_0 = *sit_p->score_2_p;

  if(gm_move && is_legal_move(0,gm_move)) {
    if(gm_move == killer_move) killer_move = 0;
    if(!beta_body(0,dmax,gm_move,last_move,&pat_possible)) return;
    gm_score = sit_p->next_p->score_0;
  }

  if(killer_move && is_legal_move(0,killer_move)) {
    if(!beta_body(0,dmax,killer_move,last_move,&pat_possible)) return;
    killer_score = sit_p->next_p->score_0;
  }
    
  mp = sit_p->moves;
  vp = sit_p->scores;
  if(*mp) {
    for(; *mp; mp++,vp++) {
      if(gm_move == *mp) {
	*vp = gm_score;
      } else if(killer_move == *mp) {
	*vp = killer_score;
      } else {
	if(!beta_body(0,dmax,*mp,last_move,&pat_possible)) return;
	*vp = sit_p->next_p->score_0;
      }
    }

    if(pat_possible) {
      pat_possible = cannot_strike_at(1,sit_p->save.black_king_place);
    }
  }

  if(pat_possible) {
    accepted_pc[0] = 0;
    accepted_score = 0; /* Stale mate */
    alarm(0);
    more = 0;
  }
}

static void alpha_descend(
  long int ply,
  long int dmax,
  long int last_move
)
{
  if(ply < dmax) {
    long int *mp;
    long int pat_possible = 1;
    long int gm_move = (sit_p->tt_p->key == sit_p->ttkey ? sit_p->tt_p->move : 0);
    long int killer_move = butterfly_board[GET_BUTTERFLY(last_move)];

    sit_p->score_0 = *sit_p->score_2_p;

    if(gm_move && is_legal_move(1,gm_move)) {
      if(gm_move == killer_move) killer_move = 0;
      if(!alpha_body(ply,dmax,gm_move,last_move,&pat_possible)) return;
    }
    if(killer_move && is_legal_move(1,killer_move)) {
      if(!alpha_body(ply,dmax,killer_move,last_move,&pat_possible)) return;
    }

    generate_moves(1);
    
    mp = sit_p->moves;

    if(*mp) {
      for(; *mp; mp++) {
	if(killer_move != *mp && gm_move != *mp) {
	  if(!alpha_body(ply,dmax,*mp,last_move,&pat_possible)) return;
	}
      }

      if(pat_possible) {
	pat_possible = cannot_strike_at(0,sit_p->save.white_king_place);
      }
    }

    if(pat_possible) {
      pc[ply][ply] = 0;
      sit_p->score_0 = 0; /* Stale mate */
    }

  } else {
    pc[ply][ply] = 0;
    sit_p->score_0 = eval(1,*sit_p->score_1_p);
  }
}

static void beta_descend(
  long int ply,
  long int dmax,
  long int last_move
)
{
  if(ply < dmax) {
    long int *mp;
    long int pat_possible = 1;
    long int gm_move = (sit_p->tt_p->key == sit_p->ttkey ? sit_p->tt_p->move : 0);
    long int killer_move = butterfly_board[GET_BUTTERFLY(last_move)];

    sit_p->score_0 = *sit_p->score_2_p;

    if(gm_move && is_legal_move(0,gm_move)) {
      if(gm_move == killer_move) killer_move = 0;
      if(!beta_body(ply,dmax,gm_move,last_move,&pat_possible)) return;
    }
    if(killer_move && is_legal_move(0,killer_move)) {
      if(!beta_body(ply,dmax,killer_move,last_move,&pat_possible)) return;
    }

    generate_moves(0);
    
    mp = sit_p->moves;
    if(*mp) {
      for(; *mp; mp++) {
	if(killer_move != *mp && gm_move != *mp) {
	  if(!beta_body(ply,dmax,*mp,last_move,&pat_possible)) return;
	}
      }

      if(pat_possible) {
	pat_possible = cannot_strike_at(1,sit_p->save.black_king_place);
      }
    }

    if(pat_possible) {
      pc[ply][ply] = 0;
      sit_p->score_0 = 0; /* Stale mate */
    }

  } else {
    pc[ply][ply] = 0;
    sit_p->score_0 = eval(0,*sit_p->score_1_p);
  }
}
