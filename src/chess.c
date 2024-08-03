/* chess.c -- by Vaino Hassinen */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "chess.h"
#include "client.h"

static char guessed[7];

static long int playing_white;
static long int remaining_time;
static long int remaining_moves;
static long int try_this_move;
static long int used_time;

long int accepted_pc[MAX_PLY];
long int accepted_score;

long int number_of_evals;

long int have_message;
long int message_checked;


// void timer_handler(int signum) {
//   static int count = 0;
//   printf("Timer signal received %d times\n", ++count);
//   if (client_listen() > 0) {
//     check_message();
//   }
// }

// void setup_signal_handler() {
//     struct sigaction sa;
//     sa.sa_handler = timer_handler;
//     sa.sa_flags = 0; // or SA_RESTART to automatically restart certain interrupted functions
//     sigemptyset(&sa.sa_mask);

//     if (sigaction(SIGALRM, &sa, NULL) == -1) {
//         perror("sigaction");
//         _exit(1);
//     }
// }

// void start_timer() {
//     struct itimerval timer;

//     // Configure the timer to expire after 2 seconds
//     timer.it_value.tv_sec = 2;
//     timer.it_value.tv_usec = 0;

//     // Configure the timer to reset to 2 seconds after it expires
//     // timer.it_interval.tv_sec = 2;
//     // timer.it_interval.tv_usec = 0;

//     // Start the timer
//     if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
//         perror("setitimer");
//         _exit(1);
//     }
// }

static void update_func()
{
  have_message++;
}

void check_message()
{
  char *f,*t;

  used_time = time(NULL) - used_time;

  message_checked++;

#ifdef ACTION
//  chess_client_get_move(command, &remaining_time, &remaining_moves);
#else
  printf("Your move> "); fgets(command, sizeof(command), stdin);
#endif

  command[79] = 0;
  for(t = f = command; *f; f++) {
    if('A' <= *f && *f <= 'Z') {
      *t++ = *f - 'A' + 'a';
    } else if(*f != ' ' && *f != '\t' && ' ' <= *f && *f <= 'z') {
      *t++ = *f;
    }
  }
  *t = 0;

  if(try_this_move > 0) {
    if(!strcmp(command,guessed)) {
      timeout_alter(playing_white,
		    (remaining_moves > 30
		     ? (remaining_time*3)/2
		     : remaining_time),
		    remaining_moves,
		    used_time);
      used_time = time(NULL) - used_time;
      return;

    } else {
      try_this_move = -1;
    }
  }

  timeout_abort();
  used_time = time(NULL) - used_time;
  return;
}

#ifndef ACTION
static void ctrl_c()
{
  update_func();
  signal(SIGINT,ctrl_c);
}
#endif

int main(int n, char *v[])
{
  long int opponet_move;
  long int my_move;

  have_message = 0;
  message_checked = 0;
  try_this_move = 0;

  remaining_time = 30;
  remaining_moves = 1;

  accepted_score = 0;
  memset(accepted_pc,0,sizeof accepted_pc);

  init_moves();

#ifdef ACTION
  setup_timer();
  chess_client_init();
#else 
  signal(SIGINT,ctrl_c);
#endif

  while(1) {

    // Listen for color
    if(!try_this_move) {
      while(have_message <= 0) {
        #ifdef ACTION
        if (client_listen() > 0) {
          check_message();
          break;
        }
        #endif
        sleep(1);
      }
    }

    // Checking message?
    printf("Checks if I got a message");
    #ifdef ACTION
      if(client_listen() > 0) check_message();
    #else
      if(have_message > message_checked) check_message();
    #endif

    printf("opponent move: %s, %ld/%ld [sec/move]\n",
	   command, remaining_time, remaining_moves);

    if(!strcmp("white",command)) {
      init_tools();
      set_board();

      playing_white = 1;
      opponet_move = 0;
      try_this_move = 0;

    } else if(!strcmp("black",command)) {
      init_tools();
      set_board();

      playing_white = 0;
      opponet_move = 0;
      try_this_move = 0;

      generate_moves(0);
      continue;

    } else if(!strcmp("exit",command)) {
      exit(0);

#ifndef ACTION
    } else if(!strcmp("load",command)) {
      init_tools();
      set_board();

      opponet_move = 0;
      try_this_move = 0;

      load_board(&playing_white);
#endif

    } else {
      switch(try_this_move) {
      case -1:
	try_this_move = 0;
	sit_pop();
	accepted_score = 0;
	memset(accepted_pc,0,sizeof accepted_pc);
	printf("Wrong guess, sob!!\n");

	/* fall through */
      case 0:
	opponet_move = convert_ascii_to_binary_move(command);

	if(!opponet_move) {
	  printf("Have to generate the moves\n");
	  generate_moves(!playing_white);

	  opponet_move = convert_ascii_to_binary_move(command);
	  if(!opponet_move) {
	    printf("Opponents move is not legal\n");
	    dump_board(!playing_white);
	    display_moves();
	    exit(0);
	  }
	}

	ttupdate(DEPTH_PRECISION_RECURSION,0,opponet_move,!playing_white);

	switch(do_move(opponet_move,0)) {
	case 1:
	  break;

	case 2:
	  if(playing_white) {
	    printf("White king taken, sob.\n");
	  } else {
	    printf("Black king taken, sob.\n");
	  }
	  break;

	default:
	  printf("Illegal move, opponent is trying...\n");
	  break;
	}
	break;

      default:
	opponet_move = try_this_move;
	printf("Right guess, yea!!\n");

	sit_p = sit_p->last_p;
	ttupdate(DEPTH_PRECISION_RECURSION,0,opponet_move,!playing_white);
	sit_p = sit_p->next_p;

	break;
      }
    }

    sit_p->save.checkpoint_level = sit_p->save.checkpoint;

    if(!try_this_move) {
      number_of_evals = 0;

      timeout_alter(playing_white,
		    (remaining_moves > 30
		     ? (remaining_time*3)/2
		     : remaining_time),
		    remaining_moves,
		    0);

      used_time = time(NULL);
      loop(playing_white,opponet_move);
      used_time = time(NULL) - used_time;
    }

    my_move = accepted_pc[0];

    if(my_move) {
#ifdef ACTION
      client_send_move(convert_binary_move_to_ascii(my_move));
      start_timer();
#else
      printf("Send move %s\n",convert_binary_move_to_ascii(my_move));
#endif

      ttupdate(DEPTH_PRECISION_RECURSION,0,my_move,playing_white);

      switch(do_move(my_move,0)) {
      case 1:
	break;

      case 2:
	if(playing_white) {
	  printf("Black king taken, good\n");
	} else {
	  printf("White king taken, good\n");
	}
	break;

      default:
	printf("Illegal move, hope nobody noticed\n");
	break;
      }

    } else {
#ifdef ACTION
      client_send_move("nomove");
#else
      printf("Send move nomove\n");
#endif
    }

    sit_p->save.checkpoint_level = sit_p->save.checkpoint;

    printf("Time %ld, evals %ld (%0.2f /sec)\n",
	   used_time,number_of_evals,
	   (1.0*number_of_evals)/(used_time > 0 ? used_time : 1));

    if(playing_white) {
      if(!cannot_strike_at(0,sit_p->save.white_king_place)) {
	printf("White is mated, but we keep wishing for bugs\n");
      }
    } else {
      if(!cannot_strike_at(1,sit_p->save.black_king_place)) {
	printf("Black is mated, but we keep wishing for bugs\n");
      }
    }

    if(!my_move) {
      dump_board(playing_white);

      printf("It's draw\n");
      exit(0);

    } else {
      dump_board(!playing_white);

      accepted_score = 0;
      try_this_move = accepted_pc[1];

      if(try_this_move) {
	long int i;

	for(i = 0; accepted_pc[i] = accepted_pc[i+2]; i++);
	for(; i < MAX_PLY; i++) accepted_pc[i] = 0;

	strcpy(guessed,convert_binary_move_to_ascii(try_this_move));
	printf("Guessing %s\n",guessed);

      } else {
	memset(accepted_pc,0,sizeof accepted_pc);
      }

      if(try_this_move) {
	long int t;

	switch(do_move(try_this_move,0)) {
	case 1:
	  number_of_evals = 0;

	  timeout_unlimited();
	  used_time = time(NULL);
	  loop(playing_white,try_this_move);
	  used_time = time(NULL) - used_time;
	  break;

	case 2:
	  if(playing_white) {
	    printf("White king may be taken, let opponent find it.\n");
	  } else {
	    printf("Black king may be taken, let opponent find it.\n");
	  }
	  sit_pop();
	  try_this_move = 0;
	  memset(accepted_pc,0,sizeof accepted_pc);
	  break;

	default:
	  printf("Opps, that move was illegal\n");
	  sit_pop();
	  try_this_move = 0;
	  memset(accepted_pc,0,sizeof accepted_pc);
	  break;
	}
      }
    }
  }
}
