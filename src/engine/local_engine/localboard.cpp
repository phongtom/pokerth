/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "localboard.h"

#include "handinterface.h"
#include <game_defs.h>
#include "localexception.h"
#include "engine_msg.h"

using namespace std;

LocalBoard::LocalBoard() : BoardInterface(), /*playerArray(0),*/ currentHand(0), pot(0), sets(0)
{
}


LocalBoard::~LocalBoard()
{
}

void LocalBoard::setPlayerLists(/*std::vector<boost::shared_ptr<PlayerInterface> > sl_old, */PlayerList sl, PlayerList apl, PlayerList rpl) {
// 	playerArray = sl_old; // delete
	seatsList = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void LocalBoard::setHand(HandInterface* br) { currentHand = br; }

void LocalBoard::collectSets() {

	sets = 0;
// 	int i;
// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) sets += playerArray[i]->getMySet();  //delete

	PlayerListConstIterator it_c;
	for(it_c=seatsList->begin(); it_c!=seatsList->end(); it_c++) {
		sets += (*it_c)->getMySet();
	}


}

void LocalBoard::collectPot() { 
// 	int i;
	pot += sets; 
	sets = 0;
// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++){ playerArray[i]->setMySetNull(); }

	PlayerListIterator it;
	for(it=seatsList->begin(); it!=seatsList->end(); it++) {
		(*it)->setMySetNull();
	}

}

void LocalBoard::distributePot() {

	winners.clear();

	size_t i,j,k,l;
	PlayerListIterator it;
	PlayerListConstIterator it_c;

	// filling player sets vector
// 	vector<int> playerSets;
// 	for(i=0; i<(size_t)MAX_NUMBER_OF_PLAYERS; i++) {
// 		if(playerArray[i]->getMyActiveStatus()) {
// 			playerSets.push_back(((playerArray[i]->getMyRoundStartCash())-(playerArray[i]->getMyCash())));
// 		} else {
// 			playerSets.push_back(0);
// 		}
// 	}

	// filling player sets vector
	vector<int> playerSets;
	for(it_c=seatsList->begin(); it_c!=seatsList->end(); it_c++) {
		if((*it_c)->getMyActiveStatus()) {
			playerSets.push_back( ( ((*it_c)->getMyRoundStartCash()) - ((*it_c)->getMyCash()) ) );
		} else {
			playerSets.push_back(0);
		}
	}


	// sort player sets asc
	vector<int> playerSetsSort = playerSets;
	sort(playerSetsSort.begin(), playerSetsSort.end());

	// potLevel[0] = amount, potLevel[1] = sum, potLevel[2..n] = winner
	vector<int> potLevel;

	// temp var
	int highestCardsValue;
	size_t winnerCount;
	size_t mod;
	int winnerPointer;
	bool winnerHit;

	// level loop
	for(i=0; i<playerSetsSort.size(); i++) {

		// restart levelHighestCardsValue
		highestCardsValue = 0;

		// level detection
		if(playerSetsSort[i] > 0) {

			// level amount
			potLevel.push_back(playerSetsSort[i]);

			// level sum
			potLevel.push_back((playerSetsSort.size()-i)*potLevel[0]);

			// determine level highestCardsValue
// 			for(j=0; j<MAX_NUMBER_OF_PLAYERS; j++) {
// 				if(playerArray[j]->getMyCardsValueInt() > highestCardsValue && playerArray[j]->getMyActiveStatus() && playerArray[j]->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) { 
// 					highestCardsValue = playerArray[j]->getMyCardsValueInt(); 
// 				}
// 			}

			// determine level highestCardsValue
			j=0;
			for(it_c=seatsList->begin(); it_c!=seatsList->end(); it_c++,j++) {
				if((*it_c)->getMyActiveStatus() && (*it_c)->getMyCardsValueInt() > highestCardsValue && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) { 
					highestCardsValue = (*it_c)->getMyCardsValueInt(); 
				}
			}

			// level winners
// 			for(j=0; j<MAX_NUMBER_OF_PLAYERS; j++) {
// 				if(highestCardsValue == playerArray[j]->getMyCardsValueInt() && playerArray[j]->getMyActiveStatus() && playerArray[j]->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) {
// 					potLevel.push_back(playerArray[j]->getMyID());
// 				}
// 			}

			// level winners
			j=0;
			for(it_c=seatsList->begin(); it_c!=seatsList->end(); it_c++,j++) {
				if((*it_c)->getMyActiveStatus() && highestCardsValue == (*it_c)->getMyCardsValueInt() && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) {
					potLevel.push_back((*it_c)->getMyUniqueID());
				}
			}

			// determine the number of level winners
			winnerCount = potLevel.size()-2;
			if (!winnerCount) {
				throw LocalException(__FILE__, __LINE__, ERR_NO_WINNER);
			}

			// distribute the pot level sum to level winners
			mod = (potLevel[1])%winnerCount;
			// pot level sum divisible by winnerCount
			if(mod == 0) {
// 				for(j=2; j<potLevel.size(); j++) {
// 					playerArray[potLevel[j]]->setMyCash(playerArray[potLevel[j]]->getMyCash() + ((potLevel[1])/winnerCount));
// 				}

				for(j=2; j<potLevel.size(); j++) {
					it = currentHand->getSeatIt(potLevel[j]);
					if(it == seatsList->end()) {
						throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
					}
					(*it)->setMyCash( (*it)->getMyCash() + ((potLevel[1])/winnerCount));

					// filling winners vector
					winners.push_back((*it)->getMyID());
				}

			}
			// pot level sum not divisible by winnerCount
			// --> distribution after smallBlind
			else {

				winnerPointer = currentHand->getDealerPosition();

				for(j=0; j<winnerCount; j++) {

					winnerHit = false;

					for(k=0; k<MAX_NUMBER_OF_PLAYERS && !winnerHit; k++){

						winnerPointer = (winnerPointer+1)%(MAX_NUMBER_OF_PLAYERS);

						winnerHit = false;

						for(l=2; l<potLevel.size(); l++) {
							if(winnerPointer == potLevel[l]) winnerHit = true;
						}

					}

// 					if(j<mod) {
// 						playerArray[winnerPointer]->setMyCash(playerArray[winnerPointer]->getMyCash() + (int)((potLevel[1])/winnerCount) + 1);
// 					} else {
// 						playerArray[winnerPointer]->setMyCash(playerArray[winnerPointer]->getMyCash() + (int)((potLevel[1])/winnerCount));
// 					}


					it = currentHand->getSeatIt(winnerPointer);
					if(it == seatsList->end()) {
						throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
					}
					if(j<mod) {
						(*it)->setMyCash( (*it)->getMyCash() + (int)((potLevel[1])/winnerCount) + 1);
						// filling winners vector
						winners.push_back((*it)->getMyID());
					} else {
						(*it)->setMyCash( (*it)->getMyCash() + (int)((potLevel[1])/winnerCount));
						// filling winners vector
						winners.push_back((*it)->getMyID());
					}
				}
			}

			// reevaluate the player sets
			for(j=0; j<playerSets.size(); j++) {
				if(playerSets[j]>0) {
					playerSets[j] -= potLevel[0];
				}
			}

			// sort player sets asc
			playerSetsSort = playerSets;
			sort(playerSetsSort.begin(), playerSetsSort.end());

			// pot refresh
			pot -= potLevel[1];

			// clear potLevel
			potLevel.clear();

		}
	}

	// winners sort and unique 
	winners.sort();
	winners.unique();


	// ERROR-Outputs

	if(pot!=0) cout << "distributePot-ERROR: Pot = " << pot << endl;

	int sum = 0;
// 	for(i=0; i<(size_t)MAX_NUMBER_OF_PLAYERS; i++) {
// 		if(playerArray[i]->getMyActiveStatus())
// 			sum += playerArray[i]->getMyCash();
// 	}

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); it_c++) {
		sum += (*it_c)->getMyCash();
	}


	if(sum != (currentHand->getStartQuantityPlayers() * currentHand->getStartCash()))
		cout << "distributePot-ERROR: PlayersSumCash = " << sum << endl;

}

