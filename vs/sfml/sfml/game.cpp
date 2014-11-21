#include "Game.h"
#include "Meeple.h"
#include "MeepleBag.h"

#include "Board.h"
#include "RBoard.h"
#include "RBag.h"

#include "I_Player.h"
#include "config.h"
#include "helper.h"

#include <SFML/System.hpp>
#include <SFML/System/Clock.hpp>

#include "RMeeple.h"

#include "ThreadController.h"

#include <vector>

#include <assert.h>
#include <string>
#include <iostream>

#include <math.h>
#include "ParticleSystem.h"



//using namespace std;


const sf::Color STANDARD_GLOW = sf::Color::Yellow;
const sf::Color SELECTED_GLOW = sf::Color::Red;


enum LoopState{
	INIT_STATE,
	//Select Meeple for opponent
	I_PLAYER_SELECT_MEEPLE, 
	HUMAN_SELECT_MEEPLE,
	TC_START_SELECT_MEEPLE, TC_WAIT_FOR_SELECTED_MEEPLE, HIGHLIGHT_SELECTED_MEEPLE,
	//Select Meeple position
	I_PLAYER_SELECT_MEEPLE_POSITION,
	HUMAN_SELECT_MEEPLE_POSITION,
	TC_START_SELECT_MEEPLE_POSITION, TC_WAIT_FOR_SELECTED_MEEPLE_POSITION, MOVE_MEEPLE_TO_SELECTED_POSITION,
	//check win /tie 
	CHECK_END_CONDITION, DISPLAY_END_SCREEN
};





Game::Game(sf::RenderWindow& window,Player& player1,  Player& player2) : window(&window),drawEndScreen(false){
    assert(&player1 != &player2);   //hehehe, that won't crash this game

	players[0] = &player1;
	players[0]->logicalMeepleBag = new MeepleBag(MeepleColor::WHITE);
	players[0]->rbag = new RBag();

	players[1] = &player2;
	players[1]->logicalMeepleBag = new MeepleBag(MeepleColor::BLACK);
	players[1]->rbag = new RBag();

	logicalBoard = new Board();

	loadTextures();
	

	board = new RBoard(boardTexture, fieldTexture, fieldTextureOccupied);

    gameStates[0] = new GameState(players[0]->logicalMeepleBag, players[1]->logicalMeepleBag, logicalBoard);
    gameStates[1] = new GameState(players[1]->logicalMeepleBag, players[0]->logicalMeepleBag, logicalBoard);
    
	initMeeples();

	color4MGlow[0] = 0.1f;
	color4MGlow[1] = 0.25f;
	color4MGlow[2] = 0.35f;
	color4MGlow[3] = 0.45f;
}


Game::~Game(){
    delete gameStates[1];
    delete gameStates[0];
	delete players[1]->logicalMeepleBag;
	delete players[0]->logicalMeepleBag;
	delete logicalBoard;
	delete board;				//	deletes all fields
	delete players[1]->rbag;	//	deletes all rendermeeples for p2
	delete players[0]->rbag;	//	deletes all rendermeeples for p1
}


void Game::reset(){
	players[0]->logicalMeepleBag->reset();
	players[1]->logicalMeepleBag->reset();

	logicalBoard->reset();
    
	//players1->reset();
 //   player2->reset();
	//TODO reset players.. inkl ifs.. 
	//players[0].controller->run_resetPlayer()
}



//Game Loop for one game, until there is a winner or the board is full
GameWinner::Enum Game::runGame(){
	sf::Clock clock;
    float elapsedTime = 0;
	
	const Meeple* meepleToSet = nullptr;
	RMeeple* glowMeepleTmp = nullptr;
	rMeepleToSet = nullptr;
	BoardPos posMeepleTo{ 42, 42 };
	bool dragMeeple = false;

	LoopState loopState = INIT_STATE;



    ParticleSystem* particleSystem = new ParticleSystem();



    ////FOR DUST-CLOUDS:
    //ParticleBuilder* builder = new ParticleBuilder({ 300, 300 }, { 5, 30 }, { 50, 150 }, { 290, 320 }, { 500, 2000 }, { 300, 500 });
    //builder->setRotation();
    //builder->setGravity(120, 90);
    //particleSystem->newParticleCloud(20, *builder);
    
    ////FOR MOUSE-CLICKS:
    //ParticleBuilder* mbBuilder = new ParticleBuilder({ 300, 300 }, { 5, 30 }, { 50, 150 });
    //mbBuilder->setRotation({ 0.1, 3.5 });
    //mbBuilder->setGravity(120, 90);
        


    ParticleBuilder* endScreepParticleBuilder = new ParticleBuilder({ 0, static_cast<float>(WINDOW_HEIGHT_TO_CALCULATE) }, { 5, 30 });
    endScreepParticleBuilder->setPath({ 10, 200 }, { 275, 350 })
                            ->setGravity(30)
                            ->setRotation({ 100, 600 }, { -1, 3 })
                            ->setFadeoutSpeed({ 60, 80 });      
        

	while (window->isOpen()){
        elapsedTime = clock.getElapsedTime().asSeconds();
	    float fps = 1.f / elapsedTime;
		clock.restart();

		text.setFont(font);
		text.setString("");
		text.setCharacterSize(50); // in pixels, not points!
		text.setColor(sf::Color::Black);
		//text.setStyle(sf::Text::Bold /*| sf::Text::Underlined*/);
		
       /* mbBuilder->setPosition(convertedMousePos);
        if (pressedLeftMouse){
            particleSystem->newParticleCloud(5, *mbBuilder);
        }*/
        
        


		pollEvents();

		switch (loopState)
		{
		case INIT_STATE:
		{
			//todo das stimmt no ned ganz human iplayer und tc !!!!
			loopState = players[activePlayerIndex]->type == Player::HUMAN ? HUMAN_SELECT_MEEPLE : TC_START_SELECT_MEEPLE;
			//goto ?
			break;
		}
		case I_PLAYER_SELECT_MEEPLE:
		{
			assert(players[activePlayerIndex]->type == Player::I_PLAYER);
			meepleToSet = &players[activePlayerIndex]->player->selectOpponentsMeeple(*gameStates[activePlayerIndex]);
			loopState = HIGHLIGHT_SELECTED_MEEPLE;
			//todo: goto??
			break;
		}
		case HUMAN_SELECT_MEEPLE:
		{
			assert(players[activePlayerIndex]->type == Player::HUMAN);
			setString(std::string("Player " + std::to_string(activePlayerIndex + 1) + " choose Meeple"));

			if (releasedLeftMouse){
				rMeepleToSet = players[(activePlayerIndex + 1) % 2]->rbag->getRMeepleAtPosition(convertedMousePos);
				if (rMeepleToSet != nullptr)
				{
					rMeepleToSet->setGlow(&SELECTED_GLOW);
					meepleToSet = rMeepleToSet->getLogicalMeeple();

					switchPlayers();
					glowMeepleTmp = nullptr;
					if (players[activePlayerIndex]->type == Player::TC){
						loopState = TC_START_SELECT_MEEPLE_POSITION;
					}
					else if (players[activePlayerIndex]->type == Player::HUMAN) {
						loopState = HUMAN_SELECT_MEEPLE_POSITION;
					}
					else{
						loopState = I_PLAYER_SELECT_MEEPLE_POSITION;
					}
				}
			}
			if (glowMeepleTmp != nullptr)
			{
				glowMeepleTmp->setGlow(nullptr);
			}
			glowMeepleTmp = players[(activePlayerIndex + 1) % 2]->rbag->getRMeepleAtPosition(convertedMousePos);
			if (glowMeepleTmp != nullptr)
			{
				glowMeepleTmp->setGlow(&STANDARD_GLOW);
			}
			break;
		}
		case TC_START_SELECT_MEEPLE:
		{
			assert(players[activePlayerIndex]->type == Player::TC);
			players[activePlayerIndex]->controller->run_selectOpponentsMeeple(*gameStates[activePlayerIndex]);
			loopState = TC_WAIT_FOR_SELECTED_MEEPLE;
			break;
		}
		case TC_WAIT_FOR_SELECTED_MEEPLE:
		{
			assert(players[activePlayerIndex]->type == Player::TC);
			if (!players[activePlayerIndex]->controller->isResultAvailable()){
				break;
			}
			meepleToSet = &players[activePlayerIndex]->controller->getOpponentsMeeple();

			loopState = HIGHLIGHT_SELECTED_MEEPLE;
			//intentional fall through
		}
		case HIGHLIGHT_SELECTED_MEEPLE:
		{
			assert(players[activePlayerIndex]->type == Player::TC);

			switchPlayers();

			rMeepleToSet = players[activePlayerIndex]->rbag->isPassedMeepleInUnused(meepleToSet);
			assert(rMeepleToSet != nullptr);
			rMeepleToSet->setGlow(&SELECTED_GLOW);
			//highlight meeple (with glow animation? light -> heavy glow)
			//search meeple in meeplebag  -> glow

			//===todo auf 
			loopState = players[activePlayerIndex]->type == Player::HUMAN ? HUMAN_SELECT_MEEPLE_POSITION : TC_START_SELECT_MEEPLE_POSITION;
			break;
		}
		case I_PLAYER_SELECT_MEEPLE_POSITION:
		{
			assert(players[activePlayerIndex]->type == Player::I_PLAYER);
			posMeepleTo = players[activePlayerIndex]->player->selectMeeplePosition(*gameStates[activePlayerIndex], *meepleToSet);
			loopState = MOVE_MEEPLE_TO_SELECTED_POSITION;
			//todo: goto??
			break;
		}
		case HUMAN_SELECT_MEEPLE_POSITION:
		{
			assert(players[activePlayerIndex]->type == Player::HUMAN);
			assert(rMeepleToSet != nullptr);
			//human code
			//move my fucking meeple :)
			//clicked meeple -> start to drag
			if (pressedLeftMouse && rMeepleToSet->containsPosition(convertedMousePos)){
				lastValidPosition = rMeepleToSet->getPosition();
				mousePosRelativeToMeepleBoundary = rMeepleToSet->getMousePosRelativeToMeepleBoundary(convertedMousePos);
				dragMeeple = true;
			}

			if (dragMeeple){ // todo checken ob !releaseleftmous braucht
				sf::Vector2f test(convertedMousePos.x - mousePosRelativeToMeepleBoundary.x, convertedMousePos.y - mousePosRelativeToMeepleBoundary.y);
				rMeepleToSet->setPosition(test);
				sf::Vector2f lookupPos = rMeepleToSet->getGlobalOrigin();
				board->setHoveredField(board->getBoardPosForPosititon(lookupPos));
			}

			if (releasedLeftMouse && dragMeeple){
				dragMeeple = false;
				sf::Vector2f lookupPos = rMeepleToSet->getGlobalOrigin();
				BoardPos pos = board->getBoardPosForPosititon(lookupPos);
				if ((pos.x < 4 && pos.y < 4) && logicalBoard->isFieldEmpty(pos)){

					sf::FloatRect fieldBounds = board->getFieldGlobalBounds(pos);
					sf::Vector2f newPosition(fieldBounds.left + fieldBounds.width / 2.f, fieldBounds.top + fieldBounds.height / 2.f);
					rMeepleToSet->setPosition(newPosition);


					players[activePlayerIndex]->rbag->changeRMeepleToUsed(*rMeepleToSet);

					Meeple* placeMe = players[activePlayerIndex]->logicalMeepleBag->removeMeeple(*meepleToSet);
					logicalBoard->setMeeple(pos, *placeMe);
					rMeepleToSet->setGlow(nullptr);
					rMeepleToSet = nullptr;
					board->setHoveredField({ 42, 42 });
					loopState = CHECK_END_CONDITION;
				}
				else{
					rMeepleToSet->setPosition(lastValidPosition);
				}
			}
			break;
		}
		case TC_START_SELECT_MEEPLE_POSITION:
		{
			assert(players[activePlayerIndex]->type == Player::TC);
			players[activePlayerIndex]->controller->run_selectMeeplePosition(*gameStates[activePlayerIndex], *meepleToSet);
			loopState = TC_WAIT_FOR_SELECTED_MEEPLE_POSITION;
			//fall through?
			break;
		}
		case TC_WAIT_FOR_SELECTED_MEEPLE_POSITION:
		{
			assert(players[activePlayerIndex]->type == Player::TC);
			if (!players[activePlayerIndex]->controller->isResultAvailable()){
				break;
			}
			posMeepleTo = players[activePlayerIndex]->controller->getMeeplePosition();

			loopState = MOVE_MEEPLE_TO_SELECTED_POSITION;
			//intentional fall through
		}
		case MOVE_MEEPLE_TO_SELECTED_POSITION:
		{
			assert(players[activePlayerIndex]->type == Player::TC);
			assert(rMeepleToSet != nullptr);

			sf::FloatRect fieldBounds = board->getFieldGlobalBounds(posMeepleTo);
			sf::Vector2f newPosition(fieldBounds.left + fieldBounds.width / 2.f, fieldBounds.top + fieldBounds.height / 2.f);

			rMeepleToSet->setPosition(newPosition);
			rMeepleToSet->setGlow(nullptr);

			Meeple* placeMe = players[activePlayerIndex]->logicalMeepleBag->removeMeeple(*meepleToSet);
			logicalBoard->setMeeple(posMeepleTo, *placeMe);

			players[activePlayerIndex]->rbag->changeRMeepleToUsed(*rMeepleToSet);

			loopState = CHECK_END_CONDITION;
			break;
		}

		case CHECK_END_CONDITION:
		{
			#if PRINT_BOARD_TO_CONSOLE
			std::cout << "Player " << activePlayerIndex + 1 << " chose meeple \"" << meepleToSet->toString() << '\"' << std::endl;
			std::cout << "Player " << (activePlayerIndex + 1) % 2 + 1 << " sets meeple to " << meepleToSet->toString() << std::endl;
				logicalBoard->print(std::cout);
			#endif
			meepleToSet = nullptr;
			rMeepleToSet = nullptr;

			const WinCombination* combi = logicalBoard->checkWinSituation();
            if (combi != nullptr){    //player2 won
#if PRINT_WINNER_PER_ROUND
                std::cout << "Player " << activePlayerIndex + 1 << " wins!" << std::endl;
#endif
                loopState = DISPLAY_END_SCREEN;

                for (uint8_t i = 0; i < 4; ++i){
                    RMeeple* temp = players[activePlayerIndex]->rbag->isPassedMeepleInUsed(combi->meeples[i]);
                    if (temp == nullptr)
                    {
                        winningCombiRMeeples[i] = players[(activePlayerIndex + 1) % 2]->rbag->isPassedMeepleInUsed(combi->meeples[i]);
                    }
                    else
                    {
                        winningCombiRMeeples[i] = temp;
                    }
                    assert(winningCombiRMeeples[i] != nullptr);
                }
            }
            else if (activePlayerIndex == 1 && logicalBoard->isFull()){
#if PRINT_WINNER_PER_ROUND
                std::cout << "Tie! There is no winner." << std::endl;
#endif
                loopState = DISPLAY_END_SCREEN;
            }
            else{
                //switchPlayers();
                loopState = INIT_STATE;
                break;
            }
            //intentional fall through
        }
        case DISPLAY_END_SCREEN:
            assert(winningCombiRMeeples[0] != nullptr && winningCombiRMeeples[1] != nullptr &&winningCombiRMeeples[2] != nullptr &&winningCombiRMeeples[3] != nullptr);
            
            if (drawEndScreen != true || rand()%100 < 3){
                int particle_count = 50;
                endScreepParticleBuilder->setPosition({ 0, static_cast<float>(WINDOW_HEIGHT_TO_CALCULATE) }, { 5, 30 })
                                        ->setPath({ 10, 200 }, { 275, 350 });
                particleSystem->newParticleCloud(particle_count, *endScreepParticleBuilder);
                endScreepParticleBuilder->setPosition({ static_cast<float>(WINDOW_WIDTH_TO_CALCULATE), static_cast<float>(WINDOW_HEIGHT_TO_CALCULATE) })
                                        ->setPath({ 10, 200 }, { 190, 265 });
                particleSystem->newParticleCloud(particle_count, *endScreepParticleBuilder);
                endScreepParticleBuilder->setPosition({ 0, 0 })
                                        ->setPath({ 10, 200 }, { -15, 89 });                                        
                particleSystem->newParticleCloud(particle_count, *endScreepParticleBuilder);
                endScreepParticleBuilder->setPosition({ static_cast<float>(WINDOW_WIDTH_TO_CALCULATE), 0 })
                                        ->setPath({ 10, 200 }, { 89, 195 });
                particleSystem->newParticleCloud(particle_count, *endScreepParticleBuilder);
            }
            drawEndScreen = true;

			if (meepleGlowAnimationClock.getElapsedTime().asSeconds() > 0.03f){

				//color4MGlow[] = 0.f;
				for (uint8_t i = 0; i < 4; ++i){
				sf::Color c= rainbow(color4MGlow[i]);
					winningCombiRMeeples[i]->setGlow(&c);

					color4MGlow[i] += 0.01f;
					if (color4MGlow[i] > 1){
						color4MGlow[i] = 0;
					}
				}
				meepleGlowAnimationClock.restart();
			}
			
			// ein button f�r men� und ein button f�r wrestart

			// glow winning combination in rainbowcolor -> nyan cat -> nyan song



			//TODO player .. state resettten jakob? 
			break;

		}

		sf::RectangleShape test;
		test.setSize(sf::Vector2f(250, 250));
		test.setPosition(0, 0);
		test.setFillColor(sf::Color(228,128,128,98));

		background.setTexture(&backgroundTexture);
		background.setSize(sf::Vector2f(static_cast<float>(WINDOW_WIDTH_TO_CALCULATE), static_cast<float>(WINDOW_HEIGHT_TO_CALCULATE)));
		background.setPosition(0, 0);

		std::string title("4Wins by Jakob M., Sebastian S. and Simon D.   @");
		title.append(std::to_string(fps));
		title.append(" fps");
		window->setTitle(title);

		window->clear(sf::Color::White);
		window->draw(background);
		
		board->draw(*window);

		//players[0]->rbag->draw(*window);
		//players[1]->rbag->draw(*window);

		window->draw(text);
             

		//window->draw(test);
        
		std::sort(meeplesToDrawAndSort.begin(), meeplesToDrawAndSort.end(), [](RMeeple* a, RMeeple* b){return a->getYPos() < b->getYPos(); });
		for (std::vector<RMeeple*>::iterator it = meeplesToDrawAndSort.begin(); it != meeplesToDrawAndSort.end(); ++it){
			(*it)->draw(*window);
		}

        particleSystem->update(elapsedTime);
        particleSystem->draw(*window);

		window->display();
	}

	return GameWinner::PLAYER_1;
}

sf::Color Game::rainbow(float progress) const
{
	/*convert to rainbow RGB*/
	float a = (1.f - progress) / 0.2f;
	uint8_t X = static_cast<uint8_t>(floor(a));
	float Y = floor(255 * (a - X));
	switch (X)
	{
	case 0:
		return sf::Color(255, static_cast<uint8_t>( Y), 0, 255);

	case 1:
		return sf::Color(static_cast<uint8_t>(255 - Y) , 255, 0, 255);

	case 2:
		return sf::Color(0, 255, static_cast<uint8_t>(Y), 255);

	case 3:
		return	sf::Color(0, static_cast<uint8_t>(255 - Y), 255, 255);

	case 4:
		return sf::Color(static_cast<uint8_t>(Y), 0, 255, 255);

	case 5:
		return sf::Color(255, 0, 255, 255);
	default:
		return sf::Color::Blue;
	}
}


void Game::initMeeples(){
	//cout << "init meeples" << endl;
	for (int i = 0; i < 16; ++i){

		unsigned int meepleIndex;
		unsigned int bagInd = 0;
		if (i < 8){
			meepleIndex = i;
		}
		else{
			meepleIndex = i - 8;
			bagInd = 1;
		}
		const Meeple* meeple = players[bagInd]->logicalMeepleBag->getMeeple(meepleIndex);

		float xCoord = 70.f;
		float yCoord = 130.f;


		if (meeple->getShape() == MeepleShape::ROUND && meeple->getDetail() == MeepleDetail::HOLE)
		{
			xCoord += 250.f;
			yCoord += 55.f;
		}
		if (meeple->getShape() == MeepleShape::ROUND && meeple->getDetail() == MeepleDetail::NO_HOLE)
		{
			xCoord += 340.f;
			yCoord += 110.f;
		}
		if (meeple->getShape() == MeepleShape::SQUARE && meeple->getDetail() == MeepleDetail::HOLE)
		{
			xCoord += 430.f;
			yCoord += 170.f;
		}
		if (meeple->getShape() == MeepleShape::SQUARE && meeple->getDetail() == MeepleDetail::NO_HOLE)
		{
			xCoord += 520.f;
			yCoord += 220.f;
		}

		if (meeple->getSize() == MeepleSize::SMALL)
		{
			xCoord += -20.f;
			yCoord += 25.f;
		}

		if (meeple->getColor() == MeepleColor::WHITE)
		{
			xCoord = WINDOW_WIDTH_TO_CALCULATE / 2.f - xCoord;
		} else
		{
			xCoord += WINDOW_WIDTH_TO_CALCULATE / 2.f;
		}
		sf::Vector2f initPos(xCoord, yCoord);

		RMeeple* rmeeple = new RMeeple(*meeple, meepleSprite, glowSprite, initPos);

		meeplesToDrawAndSort.push_back(rmeeple);
		players[bagInd]->rbag->addRMeeple(rmeeple);
	}
}


void Game::loadTextures(){
	if (!meepleSprite.loadFromFile("Resources\\pencilStyle.png")){
		std::cerr << "couldn't load the texture: meepleSprites" << std::endl;
		assert(false);
	}
	if (!glowSprite.loadFromFile("Resources\\glow.png")){
		std::cerr << "couldn't load the texture: glow" << std::endl;
		assert(false);
	}
	if (!boardTexture.loadFromFile("Resources\\board.png")){ 
		std::cerr << "couldn't load the texture: board" << std::endl;
		assert(false);
	}
	if (!fieldTexture.loadFromFile("Resources\\field.png")){
		std::cerr << "couldn't load the texture: field" << std::endl;
		assert(false);
	}
	if (!fieldTextureOccupied.loadFromFile("Resources\\field.png")){
		std::cerr << "couldn't load the texture: field" << std::endl;
		assert(false);
	}

	if (!backgroundTexture.loadFromFile("Resources\\background.png")){
		std::cerr << "couldn't load the texture: field" << std::endl;
		assert(false);
	}
	backgroundTexture.setSmooth(true);

	if (!font.loadFromFile("Resources\\Fonts\\roboto\\roboto-black.ttf"))
	{
		std::cerr << "Couldn't load the Font: roboto-black.ttf" << std::endl;
		assert(false);
	}
}


void Game::setString(std::string message)
{
	text.setString(message);

	sf::FloatRect textRect = text.getLocalBounds();
	text.setOrigin(textRect.left + textRect.width / 2.0f,
		25.0f);
	text.setPosition(sf::Vector2f(WINDOW_WIDTH_TO_CALCULATE / 2.0f, 35.0f));


}


void Game::pollEvents(){
	pressedLeftMouse = false;
	releasedLeftMouse = false;

	sf::Event event;
	sf::Vector2i mousepos = sf::Mouse::getPosition(*window);
	convertedMousePos = window->mapPixelToCoords(mousepos);
	while (window->pollEvent(event)){
		switch (event.type){

			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left){
					pressedLeftMouse = true;
				}
				break;

			case sf::Event::MouseButtonReleased:
				switch (event.mouseButton.button){
				case sf::Mouse::Left:
					releasedLeftMouse = true;
					break;

				default:
					break;
				}
				break;

			case sf::Event::MouseMoved:
				convertedMousePos = window->mapPixelToCoords(mousepos);
				break;

			case sf::Event::Resized:
				handleResizeWindowEvent(window);
				break;

			case sf::Event::Closed:
				window->close();
				break;

			default:
				break;
		}

	}

	
}


void Game::switchPlayers(){
	++activePlayerIndex;
	activePlayerIndex %= 2;
}

