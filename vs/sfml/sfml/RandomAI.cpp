#include "RandomAI.h"

#include <assert.h>
#include <algorithm>
#include "BoardState.h"
#include "MeepleBag.h"
#include <time.h>       

RandomAI::RandomAI(){
    srand(static_cast<unsigned int>(time(NULL)));
}

const Meeple& RandomAI::selectOpponentsMeeple(const MeepleBag& ownBag, const MeepleBag& opponentBag, const BoardState& board){    
    return *opponentBag.getMeeple( rand() % opponentBag.getMeepleCount() );
}

BoardPos RandomAI::selectMeeplePosition(const MeepleBag& ownBag, const MeepleBag& opponentBag, const BoardState& board, const Meeple& meepleToSet){
    BoardPos pos;
    do{
        pos.x = rand() % 4;
        pos.y = rand() % 4;
    } while (!board.isFieldEmpty(pos));    
    return pos;
}
