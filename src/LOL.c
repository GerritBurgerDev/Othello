#include <arpa/inet.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = 2;
const int OUTER = 3;
const int ALLDIRECTIONS[8] = {-11, -10, -9, -1, 1, 9, 10, 11};

/*
 * Weighted board for evaluation function.
 */
int WEIGHTS[100] = {0,   0,   0,  0,  0,  0,  0,   0,   0, 0,
                    0, 150, -40, 20, 10, 10, 20, -40, 150, 0,
                    0, -40,-180, -5, -5, -5, -5,-180, -40, 0,
                    0,  20,  -5, 15,  3,  3, 15,  -5,  20, 0,
                    0,  10,  -5,  3,  3,  3,  3,  -5,  10, 0,
                    0,  10,  -5,  3,  3,  3,  3,  -5,  10, 0,
                    0,  20,  -5, 15,  3,  3, 15,  -5,  20, 0,
                    0, -40,-180, -5, -5, -5, -5,-180, -40, 0,
                    0, 150, -40, 20, 10, 10, 20, -40, 150, 0,
                    0,   0,   0,  0,  0,  0,  0,   0,   0, 0};

const int BOARDSIZE = 100;

char nameof(int piece);

char *gen_move();
char *get_move_string(int loc);
char *serial();

int count(int player, int *board);
int eval(int move, int colour, int *sent_board);
int findbracketingpiece(int square, int dir, int player, int *sent_board);
int gen_rand();
int get_loc(char *movestring);
int legalp(int move, int player, int *sent_board);
int minimax(int move_made, int alpha, int beta, int colour, int level, int *sent_board);
int opponent(int player);
int validp(int move);
int wouldflip(int move, int dir, int player, int *sent_board);
int randomstrategy();
int run_minimax(int move, int colour, int *sent_board);
int get_depth(int moves);

int *initialboard(void);
int *legalmoves(int player, int *sent_board);

void game_over();
void initialise();
void makemove(int move, int player, int *sent_board);
void makeflips(int move, int dir, int player, int *sent_board);
void play_move(char *move);
void printboard();
void run_worker();
void update(int *sent_board);

int my_colour;
int time_limit;
int running;
int rank;
int size;
int *board;
int firstrun = 1;
FILE *fp;

int main(int argc, char *argv[])
{
  int socket_desc, port, msg_len, i;
  char *ip, *cmd, *opponent_move, *my_move;
  char msg_buf[15], len_buf[2];
  struct sockaddr_in server;

  ip = argv[1];
  port = atoi(argv[2]);
  time_limit = atoi(argv[3]);
  my_colour = EMPTY;
  running = 1;

  /* starts MPI */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* get current process id */
  MPI_Comm_size(MPI_COMM_WORLD, &size); /* get number of processes */

  // Rank 0 is responsible for handling communication with the server
  if (rank == 0) {

#ifdef DEBUG
    fp = fopen(argv[4], "w");
    fprintf(fp, "This is an example of output written to file. %d \n", size);
    fflush(fp);
#endif

    initialise();
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc == -1) {
#ifdef DEBUG
      fprintf(fp, "Could not create socket\n");
      fflush(fp);
#endif
      return -1;
    }

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Connect to remote server
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
#ifdef DEBUG
      fprintf(fp, "Connect error\n");
      fflush(fp);
#endif
      return -1;
    }

#ifdef DEBUG
    fprintf(fp, "Connected\n");
    fflush(fp);
#endif

    if (socket_desc == -1) {
      return 1;
    }

    while (running == 1) {

      if (firstrun == 1) {
        char tempColour[1];

        if (recv(socket_desc, tempColour, 1, 0) < 0) {
#ifdef DEBUG
          fprintf(fp, "Receive failed\n");
          fflush(fp);
#endif
          running = 0;
          break;
        }

        my_colour = atoi(tempColour);

        MPI_Bcast(&my_colour, 1, MPI_INT, 0, MPI_COMM_WORLD);

#ifdef DEBUG
        printboard();
#endif

#ifdef DEBUG
        fprintf(fp, "Player colour is: %d\n", my_colour);
        fflush(fp);
#endif
        firstrun = 2;
      }

      if (recv(socket_desc, len_buf, 2, 0) < 0) {
#ifdef DEBUG
        fprintf(fp, "Receive failed\n");
        fflush(fp);
#endif
        running = 0;
        break;
      }

      msg_len = atoi(len_buf);

      if (recv(socket_desc, msg_buf, msg_len, 0) < 0) {
#ifdef DEBUG
        fprintf(fp, "Receive failed\n");
        fflush(fp);
#endif
        running = 0;
        break;
      }

      msg_buf[msg_len] = '\0';
      cmd = strtok(msg_buf, " ");

      if (strcmp(cmd, "game_over") == 0) {
        running = 0;
#ifdef DEBUG
        fprintf(fp, "Game over\n");
        fflush(fp);
#endif
        break;

      } else if (strcmp(cmd, "gen_move") == 0) {
        if (size != 1) {
          for (i = 1; i < size; i++) {
            MPI_Ssend(board, BOARDSIZE, MPI_INT, i, 1010, MPI_COMM_WORLD);
          }

          my_move = gen_move();
        } else {
          my_move = serial();
        }

        if (send(socket_desc, my_move, strlen(my_move), 0) < 0) {
          running = 0;
#ifdef DEBUG
          fprintf(fp, "Move send failed\n");
          fflush(fp);
#endif
          break;
        }

#ifdef DEBUG
        printboard();
#endif
      } else if (strcmp(cmd, "play_move") == 0) {
        opponent_move = strtok(NULL, " ");
        play_move(opponent_move);
#ifdef DEBUG
        printboard();
#endif
      }

      memset(len_buf, 0, 2);
      memset(msg_buf, 0, 15);
    }

    game_over();
  } else {
    initialise();
    run_worker();
    MPI_Finalize();
  }

  return 0;
}

/*
 * Called at the start of execution on all ranks
 */
void initialise()
{
  int i;
  running = 1;
  board = (int *)malloc(BOARDSIZE * sizeof(int));
  for (i = 0; i <= 9; i++)
    board[i] = OUTER;
  for (i = 10; i <= 89; i++) {
    if (i % 10 >= 1 && i % 10 <= 8)
      board[i] = EMPTY;
    else
      board[i] = OUTER;
  }
  for (i = 90; i <= 99; i++)
    board[i] = OUTER;
  board[44] = WHITE;
  board[45] = BLACK;
  board[54] = BLACK;
  board[55] = WHITE;
}

/*
 * Called at the start of execution on all ranks except for rank 0.
 * This is where messages are passed between workers to guide the search.
 * The colour is received when the game is initialised, also the board is
 * received after each itiration of the game. Termination detecion is
 * implemented by checking if the board[0] has a value of 1 and so the game
 * knows that the game has finished. There is a while true while the game is
 * running that receives the moves sent dynamically from rank 0 and then
 * executes the minimax algorithm on the sent move and send the result back to
 * rank 0.
 */
void run_worker()
{
  int *result;
  int val, move;

  MPI_Bcast(&my_colour, 1, MPI_INT, 0, MPI_COMM_WORLD);

  result = malloc(2 * sizeof(int));

  while (running) {
    MPI_Recv(board, BOARDSIZE, MPI_INT, 0, 1010, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);

    if (board[0] == 1) {
      free(result);
      break;
    }

    while (1) {
      MPI_Recv(&move, 1, MPI_INT, 0, 1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      if (move == -1) {
        break;
      }

      val = run_minimax(move, my_colour, board);

      result[0] = move;
      result[1] = val;

      MPI_Ssend(result, 2, MPI_INT, 0, 2000, MPI_COMM_WORLD);
    }
  }
}

/*
 * Initializes the minimax algorithm for a move.
 * It saves the board state so that it is reset after the minimax algorithm has
 * completed.
 */
int run_minimax(int move, int colour, int *sent_board)
{
  int *moves, score, depth;

  moves = legalmoves(my_colour, board);
  depth = get_depth(moves[0]);
  free(moves);

  score = minimax(move, -10000, 10000, colour, depth, sent_board);

  return score;
}

int minimax(int move_made, int alpha, int beta, int colour, int level, int *sent_board)
{
  int *moves, *temp_board;
  int val;

  temp_board = malloc(BOARDSIZE * sizeof(int));
  memcpy(temp_board, sent_board, BOARDSIZE * sizeof(int));
  makemove(move_made, colour, temp_board);

  if (level == 0) {
    return eval(move_made, my_colour, temp_board);
  }

  moves = legalmoves(opponent(colour), temp_board);

  if (moves[0] == 0) {
    free(moves);
    return eval(move_made, my_colour, temp_board);
  }

  for (int i = 1; i <= moves[0]; i++) {
    val = minimax(moves[i], alpha, beta, opponent(colour), level - 1, temp_board);

    if (colour == my_colour && val < beta) {
      beta = val;
    } else if (colour != my_colour && val > alpha) {
      alpha = val;
    }

    if (alpha >= beta) {
      break;
    }
  }

  if (colour == my_colour) {
    free(moves);
    free(temp_board);
    return beta;
  } else {
    free(moves);
    free(temp_board);
    return alpha;
  }
}

/*
 * Evaluation function used to give a move a weight for better decision making.
 * This function uses a weighted board to try and forcefully grab a corner, it
 * checks the amount of available moves the player will have and also it will
 * try to maximize the amount of moves the opponent will allow the player by
 * checking available edges from the other player.
 */
int eval(int move, int colour, int *sent_board)
{
  int *moves;
  int weighting_val, i, opponent_cnt, player_cnt, other_p;
  int amount_of_moves, opponent_moves, score, edges, empty, my_flips,
      other_flips;

  other_p = opponent(my_colour);

  my_flips = count(my_colour, sent_board);
  other_flips = count(other_p, sent_board);

  if (my_flips == 0) {
    return -5000;
  } else if (other_flips == 0) {
    return 5000;
  }

  moves = legalmoves(other_p, sent_board);
  opponent_moves = moves[0];
  free(moves);
  moves = legalmoves(my_colour, sent_board);
  amount_of_moves = moves[0];
  free(moves);

  edges = empty = player_cnt = opponent_cnt = 0;

  for (i = 11; i <= 88; i++) {
    update(sent_board);
    if (sent_board[i] == my_colour) {
      player_cnt += WEIGHTS[i];
    }

    if (sent_board[i] == other_p) {
      opponent_cnt += WEIGHTS[i];
      for (int j = 0; j < 8; j++) {
        if (sent_board[i + ALLDIRECTIONS[j]] == EMPTY) {
          edges++;
          break;
        }
      }
    }

    if (sent_board[i] == EMPTY) {
      empty++;
    }
  }

  if (empty != 0) {
    weighting_val = player_cnt - opponent_cnt;

    if (weighting_val > 0) {
      score = weighting_val + (amount_of_moves * 10) + (edges * 15) + (my_flips - other_flips) - (opponent_moves * 10);
    } else {
      score = weighting_val;
    }
  } else {
    score = my_flips - other_flips;
  }

  free(sent_board);
  return score;
}

/*
 * When a corner has been taken change the weight
 */
void update(int *sent_board)
{
  if (sent_board[11] != EMPTY) {
    WEIGHTS[12] = 15;
    WEIGHTS[21] = 15;
    WEIGHTS[22] = 15;
  }
  if (sent_board[18] != EMPTY) {
    WEIGHTS[17] = 15;
    WEIGHTS[27] = 15;
    WEIGHTS[28] = 15;
  }
  if (sent_board[81] != EMPTY) {
    WEIGHTS[71] = 15;
    WEIGHTS[72] = 15;
    WEIGHTS[82] = 15;
  }
  if (sent_board[88] != EMPTY) {
    WEIGHTS[76] = 15;
    WEIGHTS[77] = 15;
    WEIGHTS[87] = 15;
  }
}

/*
 * Run program as a serial program with only one thread.
 */
char *serial()
{
  int result, *moves;
  int loc, i, best_score, best_move;
  char *move;

  moves = legalmoves(my_colour, board);

  best_move = -100;
  best_score = -2000;

  for (i = 1; i <= moves[0]; i++) {
    result = run_minimax(moves[i], my_colour, board);

    if (result > best_score) {
      best_move = moves[i];
      best_score = result;
    }
  }

  if (best_move != -100) {
    loc = best_move;
  } else {
    loc = -1;
  }

  if (loc == -1) {
    move = "pass\n";
  } else {
    move = get_move_string(loc);
    makemove(loc, my_colour, board);
  }

  return move;
}

/*
 * Rank 0 gets the available moves at the current state and then sends the moves
 * dynamically to the other processes which then runs the moves through the
 * minimax algorithm and returns a score and the move that was evaluated. Rank 0
 * sends the first few moves to the other processes and then probes to check
 * whether a process has finished evaluating a moves and sends it another move
 * to evaluate.
 */
char *gen_move()
{
  MPI_Status status;
  char *move;
  int *result, *moves;
  int amount_of_moves, available, sent, received, done;
  int loc, i, best_score, best_move, flag;

  flag = 0;

  moves = legalmoves(my_colour, board);
  amount_of_moves = moves[0];
  available = moves[0];
  sent = received = 0;

  for (i = 1; i < size; i++) {

    if (available > 0) {
      MPI_Ssend(&moves[i], 1, MPI_INT, i, 1000, MPI_COMM_WORLD);
      sent += 1;
      available -= 1;
    }
  }

  best_move = -100;
  best_score = -2000;

  result = malloc(2 * sizeof(int));

  while (received < amount_of_moves) {
    MPI_Iprobe(MPI_ANY_SOURCE, 2000, MPI_COMM_WORLD, &flag, &status);

    if (flag == 1) {
      MPI_Recv(result, 2, MPI_INT, status.MPI_SOURCE, 2000, MPI_COMM_WORLD,
               &status);
      received += 1;

      if (result[1] > best_score) {
        best_score = result[1];
        best_move = result[0];
      }

      if (sent < amount_of_moves && available > 0) {
        sent += 1;
        available -= 1;
        MPI_Ssend(&moves[sent], 1, MPI_INT, status.MPI_SOURCE, 1000,
                  MPI_COMM_WORLD);
      }
    }
  }

  free(result);

  done = -1;
  for (i = 1; i < size; i++) {
    MPI_Ssend(&done, 1, MPI_INT, i, 1000, MPI_COMM_WORLD);
  }

  if (best_move != -100) {
    loc = best_move;
  } else {
    loc = -1;
  }

  if (loc == -1) {
    move = "pass\n";
  } else {
    move = get_move_string(loc);
    makemove(loc, my_colour, board);
  }

  return move;
}

/*
 * Gets a dynamic depth based on the amount of moves available at that time
 */
int get_depth(int moves)
{
  if (moves >= 8 && moves < 15) {
    return 5;
  } else if (moves >= 15) {
    return 4;
  } else if (moves >= 3 && moves < 8) {
    return 6;
  } else {
    return 7;
  }

  return 2;
}

/*
 * Called when the other engine has made a move. The move is given in a string
 * parameter of the form "xy", where x and y represent the row and column where
 * the opponent's piece is placed, respectively.
 */
void play_move(char *move)
{
  int loc;
  if (my_colour == EMPTY) {
    my_colour = WHITE;
  }
  if (strcmp(move, "pass") == 0) {
    return;
  }
  loc = get_loc(move);
  makemove(loc, opponent(my_colour), board);
}

/*
 * Called when the match is over.
 */
void game_over()
{
  int i;

  board[0] = 1;

  for (i = 1; i < size; i++) {
    MPI_Ssend(board, BOARDSIZE, MPI_INT, i, 1010, MPI_COMM_WORLD);
  }

  MPI_Finalize();
}

char *get_move_string(int loc)
{
  static char ms[3];
  int row, col, new_loc;

  new_loc = loc - (9 + 2 * (loc / 10));
  row = new_loc / 8;
  col = new_loc % 8;
  ms[0] = row + '0';
  ms[1] = col + '0';
  ms[2] = '\n';

  return ms;
}

int get_loc(char *movestring)
{
  int row, col;
  row = movestring[0] - '0';
  col = movestring[1] - '0';
  return (10 * (row + 1)) + col + 1;
}

int *legalmoves(int player, int *sent_board)
{
  int move, i, *moves;
  moves = (int *)malloc(65 * sizeof(int));
  moves[0] = 0;
  i = 0;
  for (move = 11; move <= 88; move++)
    if (legalp(move, player, sent_board)) {
      i++;
      moves[i] = move;
    }
  moves[0] = i;
  return moves;
}

int legalp(int move, int player, int *sent_board)
{
  int i;
  if (!validp(move))
    return 0;
  if (sent_board[move] == EMPTY) {
    i = 0;
    while (i <= 7 && !wouldflip(move, ALLDIRECTIONS[i], player, sent_board))
      i++;
    if (i == 8)
      return 0;
    else
      return 1;
  } else
    return 0;
}

int validp(int move)
{
  if ((move >= 11) && (move <= 88) && (move % 10 >= 1) && (move % 10 <= 8))
    return 1;
  else
    return 0;
}

int wouldflip(int move, int dir, int player, int *sent_board)
{
  int c;
  c = move + dir;
  if (sent_board[c] == opponent(player))
    return findbracketingpiece(c + dir, dir, player, sent_board);
  else
    return 0;
}

int findbracketingpiece(int square, int dir, int player, int *sent_board)
{
  while (sent_board[square] == opponent(player))
    square = square + dir;
  if (sent_board[square] == player)
    return square;
  else
    return 0;
}

int opponent(int player)
{
  switch (player) {
  case 1:
    return 2;
  case 2:
    return 1;
  default:
    printf("illegal player\n");
    return 0;
  }
}

void makemove(int move, int player, int *sent_board)
{
  int i;
  sent_board[move] = player;
  for (i = 0; i <= 7; i++)
    makeflips(move, ALLDIRECTIONS[i], player, sent_board);
}

void makeflips(int move, int dir, int player, int *sent_board)
{
  int bracketer, c;
  bracketer = wouldflip(move, dir, player, sent_board);
  if (bracketer) {
    c = move + dir;
    do {
      sent_board[c] = player;
      c = c + dir;
    } while (c != bracketer);
  }
}

void printboard()
{
  int row, col;
  fprintf(fp, "   1 2 3 4 5 6 7 8 [%c=%d %c=%d]\n", nameof(BLACK),
          count(BLACK, board), nameof(WHITE), count(WHITE, board));
  for (row = 1; row <= 8; row++) {
    fprintf(fp, "%d  ", row);
    for (col = 1; col <= 8; col++)
      fprintf(fp, "%c ", nameof(board[col + (10 * row)]));
    fprintf(fp, "\n");
  }
  fflush(fp);
}

char nameof(int piece)
{
  static char piecenames[5] = ".bw?";
  return (piecenames[piece]);
}

int count(int player, int *board)
{
  int i, cnt;
  cnt = 0;
  for (i = 11; i <= 88; i++)
    if (board[i] == player)
      cnt++;
  return cnt;
}
