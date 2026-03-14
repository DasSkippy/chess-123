Sean Frederico
CMPM 123

I have added functionality for pawns rooks and kings
I have also added functionality under the friendly() function to ensure pieces can be taken.

I have now also added functionality for rooks, bishops, and queens
It was interesting to realize that for the Queen function, I simply had to check it as if it was both a rook or a bishop in order to move

I have now added an AI opponent using negamax with a depth of 3 and alpha/beta pruning. Some challenges I encountered were negamax function getting called in an infinate loop, as well as some lag when moving pieces around the board. I had to solve this by making me generateAllMoves, evaluateBoard, and my makeMove and UndoMove functions more efficient. The Ai now plays very well and intelligently