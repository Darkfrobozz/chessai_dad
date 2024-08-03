/* tools.c -- by Vaino Hassinen */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "chess.h"

TTENTRY TT[TTSIZE];
PENTRY P[PSIZE];

SITUATION sit[MAX_PLY];
SITUATION *sit_p;

long int butterfly_board[GET_BUTTERFLY(~0)];
long int good_move[TTSIZE];

static void dump_hash()
{
  unsigned long int null_hash[8*8];
  long int pice;

  memset(null_hash,0,sizeof null_hash);

  for(pice = BLACK_VIRGIN_KING; pice <= WHITE_VIRGIN_KING; pice++) {
    if(memcmp(hash[pice+MAN_OFSET],null_hash,sizeof null_hash)) {
      int r,c;

      man_convert(pice);
      printf("\n");

      printf("    ");
      for(c = 0; c < 8; c++) {
	printf("%c   ",c+'a');
      }
      printf("\n  +");
      for(c = 0; c < 8; c++) {
	printf("---+");
      }
      printf("\n");
      for(r = 7; r >= 0; r--) {
	printf("%c |",r+'1');
	for(c = 0; c < 8; c++) {
	  long int place = Q(r,c);
	  if(hash[pice+MAN_OFSET][place] == 0ul) {
	    printf("  ");
	  } else {
	    printf(" *");
	  }
	  printf(" |");
	}
	printf("\n  +");
	for(c = 0; c < 8; c++) {
	  printf("---+");
	}
	printf("\n");
      }
      printf("    ");
      for(c = 0; c < 8; c++) {
	printf("%c   ",c+'a');
      }
      printf("\n");
      {char dummy[100]; fgets(dummy,sizeof(dummy),stdin); }
    }
  }
}

long int cannot_strike_at(
  long int for_white,
  long int place
)
{
  long int *mp;

  sit_push();
  generate_moves(for_white);
      
  for(mp = sit_p->moves; *mp; mp++) {
    if(GET_TO(*mp) == place) {
      sit_pop();
      return 0;
    }
  }

  sit_pop();
  return 1;
}

void init_tools()
{
  memset(TT,0,sizeof TT);
  memset(P,0,sizeof P);
  memset(butterfly_board,0,sizeof butterfly_board);
  memset(sit,0,sizeof sit);

  for(sit_p = sit; sit_p != &sit[MAX_PLY]; sit_p++) {
    sit_p->last_p = sit_p-1;
    sit_p->next_p = sit_p+1;
  }

  sit[0].last_p = &sit[MAX_PLY-1];
  sit[MAX_PLY-1].next_p = &sit[0];

  for(sit_p = sit; sit_p != &sit[MAX_PLY]; sit_p++) {
    sit_p->last_save_p = &sit_p->last_p->save;
    sit_p->score_1_p = &sit_p->last_p->score_0;
    sit_p->score_2_p = &sit_p->last_p->last_p->score_0;
  }

  sit_p = sit;
}

int board_convert(
  int place
)
{
  if(0 <= place && place <= 8*8) {
    printf("%c%c", 'a' + place % 8, '1' + place / 8);
    return 1;
  } else {
    printf("??");
    return 0;
  }
}

long int move_convert(
  long int move
)
{
  long int status = 1;

  if(!board_convert(GET_FROM(move))) status = 0;
  if(!board_convert(GET_TO(move))) status = 0;

  switch(GET_SPECIAL(move)) {
  case SPECIAL_NONE: 	   printf("  "); break;
  case SPECIAL_QUEEN: 	   printf("q "); break;
  case SPECIAL_ROOK: 	   printf("r "); break;
  case SPECIAL_BISHOP: 	   printf("b "); break;
  case SPECIAL_KNIGHT: 	   printf("n "); break;
  case SPECIAL_ENPASSANT:  printf("-e"); break;
  case SPECIAL_DOUBLE: 	   printf("-d"); break;
  case SPECIAL_S_CASTLING: printf("-s"); break;
  case SPECIAL_L_CASTLING: printf("-l"); break;
  default:
    printf("? ");
    status = 0;
    break;
  }

  return status;
}

long int man_value(
  long int man
)
{
  switch(man) {
  case BLACK_VIRGIN_KING: return VALUE_BLACK_VIRGIN_KING;
  case BLACK_KING:        return VALUE_BLACK_KING;
  case BLACK_QUEEN:       return VALUE_BLACK_QUEEN;
  case BLACK_VIRGIN_ROOK: return VALUE_BLACK_VIRGIN_ROOK;
  case BLACK_ROOK:        return VALUE_BLACK_ROOK;
  case BLACK_BISHOP:      return VALUE_BLACK_BISHOP;
  case BLACK_KNIGHT:      return VALUE_BLACK_KNIGHT;
  case BLACK_PAWN:        return VALUE_BLACK_PAWN;
  case WHITE_PAWN:        return VALUE_WHITE_PAWN;
  case WHITE_KNIGHT:      return VALUE_WHITE_KNIGHT;
  case WHITE_BISHOP:      return VALUE_WHITE_BISHOP;
  case WHITE_ROOK:        return VALUE_WHITE_ROOK;
  case WHITE_VIRGIN_ROOK: return VALUE_WHITE_VIRGIN_ROOK;
  case WHITE_QUEEN:       return VALUE_WHITE_QUEEN;
  case WHITE_KING:        return VALUE_WHITE_KING;
  case WHITE_VIRGIN_KING: return VALUE_WHITE_VIRGIN_KING;
  default:                return 0;
  }
}

void set_board_dependencies(
  long int for_white,
  long int optional_enpassant
)
{
  long int nopices;
  long int nopawns;
  long int nopawnsteps;
  long int place;

  sit_p->save.material_balance = 0;

  sit_p->save.black_king_place = -1;
  sit_p->save.white_king_place = -1;

  sit_p->save.enpassant = optional_enpassant;

  sit_p->save.tthash = 0;
  sit_p->save.phash = 0;
  sit_p->save.ttkey_root = 0;
  sit_p->save.pkey_root = 0;

  if(for_white) {
    set_white_color();
  } else {
    set_black_color();
  }

  if(sit_p->save.enpassant >= 0) {
    sit_p->save.tthash ^= hash[ENPASSANTMAN+MAN_OFSET][sit_p->save.enpassant];
  }

  nopawns = 0;
  nopices = 0;
  nopawnsteps = 0;
  for(place = 0; place < 8*8; place++) {
    long int man = sit_p->save.board[place];

    sit_p->save.material_balance += man_value(man);

    switch(man) {
    case WHITE_KING:
    case WHITE_VIRGIN_KING:
      sit_p->save.white_king_place = place;
      sit_p->save.tthash ^= hash[man+MAN_OFSET][place];
      sit_p->save.phash ^= hash[man+MAN_OFSET][place];
      nopices++;
      break;
    case BLACK_KING:
    case BLACK_VIRGIN_KING:
      sit_p->save.black_king_place = place;
      sit_p->save.tthash ^= hash[man+MAN_OFSET][place];
      sit_p->save.phash ^= hash[man+MAN_OFSET][place];
      nopices++;
      break;
    case BLACK_PAWN:
      sit_p->save.tthash ^= hash[man+MAN_OFSET][place];
      sit_p->save.phash ^= hash[man+MAN_OFSET][place];
      nopawnsteps += (6-GET_R(place));
      nopawns++;
      break;
    case WHITE_PAWN:
      sit_p->save.tthash ^= hash[man+MAN_OFSET][place];
      sit_p->save.phash ^= hash[man+MAN_OFSET][place];
      nopawnsteps += (GET_R(place)-1);
      nopawns++;
      break;
    case EMPTY:
      break;
    default:
      nopices++;
      sit_p->save.tthash ^= hash[man+MAN_OFSET][place];
      break;
    }
  }

  for(; nopices < 16; nopices++) {
    register_pice_taken();
  }

  for(; nopawns < 16; nopawns++) {
    register_pawn_taken();
  }

  register_pawn_moved(nopawnsteps);

  set_hash();

  sit_p->score_0 = eval(for_white,
			(for_white ? MAX_VALUE : MIN_VALUE));
  *sit_p->score_1_p = sit_p->score_0;
  *sit_p->score_2_p = sit_p->score_0;
}

long int man_convert(
  long int man
)
{
  switch(man) {
  case BLACK_VIRGIN_KING: printf("*k"); break;
  case BLACK_KING: 	  printf("*K"); break;
  case BLACK_QUEEN: 	  printf("*Q"); break;
  case BLACK_VIRGIN_ROOK: printf("*r"); break;
  case BLACK_ROOK: 	  printf("*R"); break;
  case BLACK_BISHOP: 	  printf("*B"); break;
  case BLACK_KNIGHT: 	  printf("*N"); break;
  case BLACK_PAWN: 	  printf("*P"); break;
  case EMPTY: 		  printf("  "); break;
  case WHITE_PAWN: 	  printf(" P"); break;
  case WHITE_KNIGHT: 	  printf(" N"); break;
  case WHITE_BISHOP: 	  printf(" B"); break;
  case WHITE_ROOK: 	  printf(" R"); break;
  case WHITE_VIRGIN_ROOK: printf(" r"); break;
  case WHITE_QUEEN: 	  printf(" Q"); break;
  case WHITE_KING: 	  printf(" K"); break;
  case WHITE_VIRGIN_KING: printf(" k"); break;
  default:
    printf("??");
    return 0;
  }

  return 1;
}

void dump_board(
  long int playing_white
)
{
  int r,c;

  printf("    ");
  for(c = 0; c < 8; c++) {
    printf("%c   ",c+'a');
  }
  printf("\n  +");
  for(c = 0; c < 8; c++) {
    printf("---+");
  }
  printf("\n");
  for(r = 7; r >= 0; r--) {
    if(playing_white && r == 0 || !playing_white && r == 7) {
      printf("%c>|",r+'1');
    } else {
      printf("%c |",r+'1');
    }
    for(c = 0; c < 8; c++) {
      long int place = Q(r,c);
      if(IS_EMPTY(sit_p->save.board[place])) {
	if(place == sit_p->save.enpassant) {
	  printf(" :");
	} else {
	  printf("  ");
	}
      } else {
	man_convert(sit_p->save.board[place]);
      }
      printf(" |");
    }
    printf("\n  +");
    for(c = 0; c < 8; c++) {
      printf("---+");
    }
    printf("\n");
  }
  printf("    ");
  for(c = 0; c < 8; c++) {
    printf("%c   ",c+'a');
  }
  printf("\n");
}

long int move_is_available(
  long int themove
)
{
  long int *mp;

  for(mp = sit_p->moves; *mp && *mp != themove; mp++);

  return *mp;
}

long int candidate_move(
  long int themove
)
{
  if(GET_SPECIAL(themove)) {
    return move_is_available(themove);

  } else {
    long int *mp;

    for(mp = sit_p->moves;
	*mp && MAKE_MOVE(GET_FROM(*mp),GET_TO(*mp)) != themove;
	mp++);

    return *mp;
  }
}

char *convert_binary_move_to_ascii(
  long int themove
)
{
  static char move_s[6];

  move_s[0] = 'a' + GET_C(GET_FROM(themove));
  move_s[1] = '1' + GET_R(GET_FROM(themove));
  move_s[2] = 'a' + GET_C(GET_TO(themove));
  move_s[3] = '1' + GET_R(GET_TO(themove));

  switch(GET_SPECIAL(themove)) {
  case SPECIAL_QUEEN:  move_s[4] = 'q'; break;
  case SPECIAL_ROOK:   move_s[4] = 'r'; break;
  case SPECIAL_BISHOP: move_s[4] = 'b'; break;
  case SPECIAL_KNIGHT: move_s[4] = 'n'; break;
  default:
    move_s[4] = 0;
    return move_s;
  }

  move_s[5] = 0;
  return move_s;
}

long int convert_ascii_to_binary_move(
  char *move_s
)
{
  long int themove;
  char *f,*t;

  t = move_s;
  for(f = move_s; *f; f++) {
    if('A' <= *f && *f <= 'Z') {
      *t++ = *f - 'A' + 'a';
    } else if('a' <= *f && *f <= 'z') {
      *t++ = *f;
    } else if('0' <= *f && *f <= '9') {
      *t++ = *f;
    } else if('-' == *f) {
      *t++ = *f;
    }
  }
  *t = 0;

  if(!strcmp(move_s,"0-0") || !strcmp(move_s,"o-o")) {
    if(themove = candidate_move(MAKE_MOVE(E1,G1))) return themove;

    return candidate_move(MAKE_MOVE(E8,G8));

  } else if(!strcmp(move_s,"0-0-0") || !strcmp(move_s,"o-o-o")) {
    if(themove = candidate_move(MAKE_MOVE(E1,C1))) return themove;

    return candidate_move(MAKE_MOVE(E8,C8));

  } else if(move_s[4]) {
    switch(move_s[4]) {
    case 'q':
      return candidate_move(MAKE_S_MOVE(Q(move_s[1]-'1',move_s[0]-'a'),
					Q(move_s[3]-'1',move_s[2]-'a'),
					SPECIAL_QUEEN));
    case 'r':
      return candidate_move(MAKE_S_MOVE(Q(move_s[1]-'1',move_s[0]-'a'),
					Q(move_s[3]-'1',move_s[2]-'a'),
					SPECIAL_ROOK));
    case 'b':
      return candidate_move(MAKE_S_MOVE(Q(move_s[1]-'1',move_s[0]-'a'),
					Q(move_s[3]-'1',move_s[2]-'a'),
					SPECIAL_BISHOP));
    case 'n':
      return candidate_move(MAKE_S_MOVE(Q(move_s[1]-'1',move_s[0]-'a'),
					Q(move_s[3]-'1',move_s[2]-'a'),
					SPECIAL_KNIGHT));
    default:
      return 0;
    }

  } else {
    return candidate_move(MAKE_MOVE(Q(move_s[1]-'1',move_s[0]-'a'),
				    Q(move_s[3]-'1',move_s[2]-'a')));
  }
}

long int get_move()
{
  char buffer[100];

  printf("Move> "); fgets(buffer,sizeof(buffer),stdin);
  return convert_ascii_to_binary_move(buffer);
}

void set_board()
{
  long int c;

  sit_p->save.board[A1] = WHITE_VIRGIN_ROOK;
  sit_p->save.board[B1] = WHITE_KNIGHT;
  sit_p->save.board[C1] = WHITE_BISHOP;
  sit_p->save.board[D1] = WHITE_QUEEN;
  sit_p->save.board[E1] = WHITE_VIRGIN_KING;
  sit_p->save.board[F1] = WHITE_BISHOP;
  sit_p->save.board[G1] = WHITE_KNIGHT;
  sit_p->save.board[H1] = WHITE_VIRGIN_ROOK;

  for(c = 0; c < 8; c++) {
    sit_p->save.board[Q(1,c)] = WHITE_PAWN;
    sit_p->save.board[Q(2,c)] = EMPTY;
    sit_p->save.board[Q(3,c)] = EMPTY;
    sit_p->save.board[Q(4,c)] = EMPTY;
    sit_p->save.board[Q(5,c)] = EMPTY;
    sit_p->save.board[Q(6,c)] = BLACK_PAWN;
  }

  sit_p->save.board[A8] = BLACK_VIRGIN_ROOK;
  sit_p->save.board[B8] = BLACK_KNIGHT;
  sit_p->save.board[C8] = BLACK_BISHOP;
  sit_p->save.board[D8] = BLACK_QUEEN;
  sit_p->save.board[E8] = BLACK_VIRGIN_KING;
  sit_p->save.board[F8] = BLACK_BISHOP;
  sit_p->save.board[G8] = BLACK_KNIGHT;
  sit_p->save.board[H8] = BLACK_VIRGIN_ROOK;

  set_board_dependencies(1,-1);
}

void display_moves()
{
  long int online = 0;
  long int *mp = sit_p->moves;
  long int *vp = sit_p->scores;

  while(*mp) {
    move_convert(*mp++);
    printf(" (%ld) ",*vp++);
    online++;
    if(online > 80/13) {
      online = 0;
      printf("\n");
    }
  }
  if(online) {
    printf("\n");
  }
}

void sort_for_white()
{
  register long int father;
  register long int *scores = &sit_p->scores[-1];
  register long int trailer;
  register long int *moves = &sit_p->moves[-1];
  register long int thescore;
  register long int vacant;
  register long int themove;

  vacant = 1;

  while(themove = moves[vacant]) {
    thescore = scores[vacant];

    trailer = vacant;
    father = trailer >> 1;

    while(father && scores[father] < thescore) {
      moves[trailer] = moves[father];
      scores[trailer] = scores[father];

      trailer = father;
      father >>= 1;
    }

    if(trailer != vacant) {
      moves[trailer] = themove;
      scores[trailer] = thescore;
    }

    vacant++;
  }
}

void sort_for_black()
{
  register long int father;
  register long int *scores = &sit_p->scores[-1];
  register long int trailer;
  register long int *moves = &sit_p->moves[-1];
  register long int thescore;
  register long int vacant;
  register long int themove;

  vacant = 1;

  while(themove = moves[vacant]) {
    thescore = scores[vacant];

    trailer = vacant;
    father = trailer >> 1;

    while(father && scores[father] > thescore) {
      moves[trailer] = moves[father];
      scores[trailer] = scores[father];

      trailer = father;
      father >>= 1;
    }

    if(trailer != vacant) {
      moves[trailer] = themove;
      scores[trailer] = thescore;
    }

    vacant++;
  }
}

void load_board(
  long int *playing_white_p
)
{
  long int playing_white;
  long int row;
  long int load_cnt;
  long int row_load[8];

  printf("Give chess board>\n");

  sit_push();

  playing_white = *playing_white_p;

  sit_p->save.enpassant = -1;

  load_cnt = 0;
  for(row = 0; row < 8; row++) row_load[row] = 0;

  while(load_cnt < 8)  {
    char buff[80],*bp;

    buff[0] = 0;
    if(!fgets(buff,sizeof(buff),stdin)) {
      sit_pop();
      return;
    }

    bp = buff;
    if('1' <= *bp && *bp <= '8') {
      long int column;

      row = (*bp++) - '1';
      if(!row_load[row]) load_cnt++;
      row_load[row] = 1;

      if(*bp == '>') {
	playing_white = (row == 0);
      }

      for(column = 0; column < 8; column++) {
	char man_ch;
	long int man;
	long int place;
	long int is_black;

	place = Q(row,column);

	bp += 2;
	is_black = (*bp++ == '*');

	man_ch = *bp++;

	if(man_ch == ':') {
	  sit_p->save.enpassant = place;
	  man_ch = ' ';
	}

	switch(man_ch) {
	case 'k':
	  man = (is_black ? BLACK_VIRGIN_KING : WHITE_VIRGIN_KING); break;
	case 'K':
	  man = (is_black ? BLACK_KING : WHITE_KING); break;
	case 'Q':
	  man = (is_black ? BLACK_QUEEN : WHITE_QUEEN); break;
	case 'r':
	  man = (is_black ? BLACK_VIRGIN_ROOK : WHITE_VIRGIN_ROOK); break;
	case 'R':
	  man = (is_black ? BLACK_ROOK : WHITE_ROOK); break;
	case 'B':
	  man = (is_black ? BLACK_BISHOP : WHITE_BISHOP); break;
	case 'N':
	  man = (is_black ? BLACK_KNIGHT : WHITE_KNIGHT); break;
	case 'P':
	  man = (is_black ? BLACK_PAWN : WHITE_PAWN); break;
	case ' ':
	  man = EMPTY; break;
	default:
	  printf("Strange man <%c%c>\n", (is_black ? '*' : ' '), man_ch);
	  sit_pop();
	  return;
	}

	sit_p->save.board[place] = man;
      }
    }
  }

  set_board_dependencies(playing_white,sit_p->save.enpassant);

  *playing_white_p = playing_white;
}

#ifdef STANDALONE

#define N 100000

main()
{
  long long int t;
  long int i;

  init_tools();
  init_moves();

  set_board();

  generate_moves(0);
  display_moves();

  generate_moves(1);
  display_moves();

  t = clock();
  for(i = 0; i < N; i++) {
    generate_moves(1);
  }
  printf("Times per generate %lf us\n",(clock()-t)/(N*100.0/1e6));
}

#endif /* STANDALONE */
