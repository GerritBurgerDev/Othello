# Othello Project

### Project description

For this project you have to implement an Othello player, using C and MPI to distribute the tree search. A minimax or negamax search with alpha-beta pruning should be used to evaluate the available moves from it's current position before making a move.

You will be provided with a serial player (random.c) that can make random moves, which you can use as a starting point, as well as a standalone tournament engine, which you can use to test how your player plays against itself, the random player, or any other players.

After final hand-in we'll run a tournament in which all the players of the class will play in matches against all the other players of the class.




### What I did

#### A broad description:

I used the minimax algorithm that goes down the tree for a certain depth and then evaluates the approximated path it followed which then send the result back to rank 0 which then chooses the best move based on a score figured out by the evaluation function. Alpha-beta pruning has been implemented which allows the program to go deeper because it cuts off the bad choices.

#### Functionalities implemented and how they work:
###### * Minimax algorithm with alpha-beta pruning
* A basic minimax algorithm with alpha-beta pruning was used which goes down a tree until a certain depth was reached which then returns a score for the certain path it followed based on my evaluation function.
* The alpha-beta pruning cuts off the branches which would have given me bad results which results in unnecessary memory use and also wasted time which could have been used to go deeper down the tree.
* Process 0 gets the legal moves at the current state of the game and then sends the moves to the other processes which then sends the moves to the minimax algorithm and the minimax algorithm makes moves based on the received moves untill the depth is reached and then evaluates the score for that path via my evaluation function and then the score for the moves is sent back to process 0 which then chooses the move with the best score.

###### * Dynamic load balancing
The dynamic load balancing in my program works as follows:
* Process 0 gets all of the legal moves at the current state of the board and then sends the first amount of moves to process 1 to 3 and then 1 to 3 evaluates the moves and then sends the score back. While process 1 to 3 is doing work I probe to see when one of them has finished the evaluation and then receives the score and if more moves have to be evaluated they are each sent to the process which has just finished their work.
Ex. (5 moves are available)
```wiki
Moves 1 to 3 is sent to Process 1 to 3.
Process 1 finishes its work and move 4 is sent to process 1 to be evaluated.
Process 3 finishes its work and move 5 is sent to process 3 to be evaluated.
Process 2 finishes its work.
Process 1 and 3 finishes the evaluation of moves 4 and 5.
All moves have been evaluated via dynamic load balancing.
```

###### * Evaluation function
I implemented the following strategies:
* Weighted board - The weighted board counts all of the tokens for each player and a score is calculated based on the sum of the values which is given on the board and then I get a score by subtracting my score with the opponent's score. The board tries to force the opponent to make moves which allows my player to take the corners.
My weighted board:
```c
const int WEIGHTS[100] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                          0,150,-40, 20,  5,  5, 20,-40,150,  0,
                          0,-40,-180, -5, -5, -5, -5,-180,-40,0,
                          0, 20, -5, 15,  3,  3, 15, -5, 20,  0,
                          0,  5, -5,  3,  3,  3,  3, -5,  5,  0,
                          0,  5, -5,  3,  3,  3,  3, -5,  5,  0,
                          0, 20, -5, 15,  3,  3, 15, -5, 20,  0,
                          0,-40,-180, -5, -5, -5, -5,-180,-40,0,
                          0,150,-40, 20,  5,  5, 20,-40,150,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
```
* If a corner has been taken the weight is changed
```c
void update()
{
  if (board[11] != EMPTY) {
    WEIGHTS[12] = 15;
    WEIGHTS[21] = 15;
    WEIGHTS[22] = 15;
  }
  if (board[18] != EMPTY) {
    WEIGHTS[17] = 15;
    WEIGHTS[27] = 15;
    WEIGHTS[28] = 15;
  }
  if (board[81] != EMPTY) {
    WEIGHTS[71] = 15;
    WEIGHTS[72] = 15;
    WEIGHTS[82] = 15;
  }
  if (board[88] != EMPTY) {
    WEIGHTS[76] = 15;
    WEIGHTS[77] = 15;
    WEIGHTS[87] = 15;
  }
}
```
* Check if I have no moves left then give a nig negative score and if the other player has no tiles then give it a very high score.
```c
if (my_flips == 0) {
    return -5000;
} else if (other_flips == 0) {
    return 5000;
}
```
* Mobility - Giving a certain weight to maximize the amount of moves I can make and minimizes the amount of moves he will have.
* Potential mobility - Counts the edges that the other player will make available and maximizes that.
* Score calculation:
```c
score = score = weighting_val + (amount_of_moves * 5) + (edges * 15) - (opponent_moves * 10)
```
* Last few moves only use the count evaluation
```c
score = count(my_colour, board) - count(opponent(my_colour), board)
```

###### * A form of dynamic deepening
* The depth that the program will choose to go will be based on the following:
```c
int get_depth(int moves)
{
  if (moves >= 8 && moves < 15) {
    return 6;
  } else if (moves >= 15) {
    return 4;
  }
  return 11 - moves;
}
```



### How to use

##### Makefile
Make the executebles

```
In proj2_othello/

$ make
```



##### run.sh
* To use the run.sh do the following:

```
$ chmod +x
$ ./run.sh
```



## Author

**Gerrit Burger**

