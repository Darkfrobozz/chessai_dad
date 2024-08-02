/* eval.c -- by Vaino Hassinen */

#include "chess.h"

static void eval_pawn_structure()
{
  int *place;
  int balance,row,column,blacks,whites;

  balance = 0;
  place = &sit_p->save.board[A1];
  for(column = 1; column <= 8; column++, place += (COLUMN_DELTA-8*ROW_DELTA)) {
    whites = 0;
    blacks = 0;
    for(row = 1; row <= 8; row++, place += ROW_DELTA) {
      switch(*place) {
      case WHITE_PAWN:
	whites++;
	if(column < 8) {
	  if(*(place+COLUMN_DELTA)           == WHITE_PAWN) balance += 3;
	  if(*(place+COLUMN_DELTA+ROW_DELTA) == WHITE_PAWN) balance += 2;
	  if(*(place+COLUMN_DELTA-ROW_DELTA) == WHITE_PAWN) balance += 2;
	}
	balance += ((VALUE_WHITE_PAWN/20) >> (9-row));
	break;
      case WHITE_KING:
	if(row < 5) {
	  if(column == 1 || *(place-COLUMN_DELTA+ROW_DELTA) == WHITE_PAWN) {
	    balance += 40;
	  }
	  if(*(place+ROW_DELTA) == WHITE_PAWN) {
	    balance += 40;
	  }
	  if(column == 8 || *(place+COLUMN_DELTA+ROW_DELTA) == WHITE_PAWN) {
	    balance += 40;
	  }
	}
	break;
      case BLACK_PAWN:
	blacks++;
	if(column < 8) {
	  if(*(place+COLUMN_DELTA)           == BLACK_PAWN) balance -= 3;
	  if(*(place+COLUMN_DELTA+ROW_DELTA) == BLACK_PAWN) balance -= 2;
	  if(*(place+COLUMN_DELTA-ROW_DELTA) == BLACK_PAWN) balance -= 2;
	}
	balance -= ((VALUE_WHITE_PAWN/20) >> row);
	break;
      case BLACK_KING:
	if(row > 4) {
	  if(column == 1 || *(place-COLUMN_DELTA-ROW_DELTA) == BLACK_PAWN) {
	    balance += 40;
	  }
	  if(*(place+ROW_DELTA) == BLACK_PAWN) {
	    balance += 40;
	  }
	  if(column == 8 || *(place+COLUMN_DELTA-ROW_DELTA) == BLACK_PAWN) {
	    balance += 40;
	  }
	}
	break;
      default:
	break;
      }
    }

    if(whites == 1 && blacks == 0) {
      balance += 20;
    } else if(whites > 1) {
      balance -= (whites-1)*VALUE_WHITE_PAWN/10;
    }

    if(blacks == 1 && whites == 0) {
      balance -= 20;
    } else if(blacks > 1) {
      balance -= (blacks-1)*VALUE_BLACK_PAWN/10;
    }
  }

  sit_p->p_p->key = 0; /* If this routine is asynchronously interuppted */
  sit_p->p_p->value = balance;
  sit_p->p_p->key = sit_p->pkey;
}

int eval(
  int for_white,
  int cut_off
)
{
  register int value;

  number_of_evals++;

  if(for_white) {
    cut_off += VALUE_WHITE_PAWN;

    value = sit_p->save.material_balance + sit_p->save.bonus;
    if(cut_off < value) return value;

    if(sit_p->p_p->key != sit_p->pkey) eval_pawn_structure();

    value += sit_p->p_p->value;
    if(cut_off < value) return value;

    return value + strike(1);

  } else {
    cut_off += VALUE_BLACK_PAWN;

    value = sit_p->save.material_balance + sit_p->save.bonus;
    if(cut_off > value) return value;

    if(sit_p->p_p->key != sit_p->pkey) eval_pawn_structure();

    value += sit_p->p_p->value;
    if(cut_off > value) return value;

    return value + strike(0);
  }
}

