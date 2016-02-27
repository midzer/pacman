#include "highscore.h"

HighscoreEntry::HighscoreEntry(std::string playerName, int score, int level) {
	this->playerName = std::string(playerName);
	this->score      = score;
	this->level      = level;
}

HighscoreEntry::~HighscoreEntry() {
}

void HighscoreEntry::addCharToPlayerName(const char c) {
	playerName += c;
}

void HighscoreEntry::removeLastCharFromPlayerName() {
	if (playerName.length() >= 1) {
		playerName.erase(playerName.length()-1, 1);
	}
}


HighscoreList *HighscoreList::instance = NULL;

HighscoreList *HighscoreList::getInstance() {
	if (instance == NULL)
		instance = new HighscoreList(Constants::HIGHSCORE_LIST_MAX_SIZE);
	return instance;
}

void HighscoreList::cleanUpInstance() {
	if (instance) {
		delete instance;
		instance = NULL;
	}
}

HighscoreList::HighscoreList(uint8_t maxSize):
	idxLastInsertedEntry(-1),
	idxHighlightedEntry(-1),
	sfTitle(NULL),
	sfPosTitle(NULL),
	sfNameTitle(NULL),
	sfScoreTitle(NULL),
	sfLevelTitle(NULL),
	sfBackItem(NULL),
	sfCaret(NULL),
	sfCurrentPos(NULL),
	sfCurrentName(NULL),
	sfCurrentScore(NULL),
	sfCurrentLevel(NULL)
{
	this->maxSize = maxSize;
	entries = new std::vector<HighscoreEntry*>();
	sfPositions   = new SDL_Surface*[maxSize];
	sfPlayerNames = new SDL_Surface*[maxSize];
	sfScores      = new SDL_Surface*[maxSize];
	sfLevels      = new SDL_Surface*[maxSize];
	for (int i = 0; i < maxSize; ++i) {
		sfPositions[i]   = NULL;
		sfPlayerNames[i] = NULL;
		sfScores[i]      = NULL;
		sfLevels[i]      = NULL;
	}
	filePath = std::string(CommandLineOptions::getValue("", "highscore"));
	if (0 == filePath.length()) {
		createGameDir();
		char tmpFilePath[256];
		getGameDirPath(tmpFilePath, "highscore");
		filePath += tmpFilePath;
	}
}

HighscoreList::~HighscoreList() {
	if (entries) {
		int i = 0;
		for (std::vector<HighscoreEntry*>::iterator it = entries->begin(); it != entries->end(); ++it) {
			delete *it;
			if (sfPositions[i])
				SDL_FreeSurface(sfPositions[i]);
			if (sfPlayerNames[i])
				SDL_FreeSurface(sfPlayerNames[i]);
			if (sfScores[i])
				SDL_FreeSurface(sfScores[i]);
			if (sfLevels[i])
				SDL_FreeSurface(sfLevels[i]);
			++i;
		}
		delete entries;
		entries = NULL;
		delete [] sfPositions;
		delete [] sfPlayerNames;
		delete [] sfScores;
		delete [] sfLevels;
	}
	if (sfTitle)
		SDL_FreeSurface(sfTitle);
	if (sfPosTitle)
		SDL_FreeSurface(sfPosTitle);
	if (sfNameTitle)
		SDL_FreeSurface(sfNameTitle);
	if (sfScoreTitle)
		SDL_FreeSurface(sfScoreTitle);
	if (sfLevelTitle)
		SDL_FreeSurface(sfLevelTitle);
	if (sfBackItem)
		SDL_FreeSurface(sfBackItem);
	if (sfCaret)
		SDL_FreeSurface(sfCaret);
	if (sfCurrentPos)
		SDL_FreeSurface(sfCurrentPos);
	if (sfCurrentName)
		SDL_FreeSurface(sfCurrentName);
	if (sfCurrentScore)
		SDL_FreeSurface(sfCurrentScore);
	if (sfCurrentLevel)
		SDL_FreeSurface(sfCurrentLevel);
}

int HighscoreList::insertEntry(HighscoreEntry *entry) {
	int i = 0;
	for (std::vector<HighscoreEntry*>::iterator it = entries->begin(); it != entries->end(); ++it) {
		if (entry->getScore()>(*it)->getScore() || (entry->getScore()==(*it)->getScore() && entry->getLevel()>(*it)->getLevel())) {
			entries->insert(it, entry);
			if ((int) entries->size() > maxSize) {
				entries->pop_back();
				if (sfPlayerNames[maxSize-1]) {
					SDL_FreeSurface(sfPlayerNames[maxSize-1]);
					sfPlayerNames[maxSize-1] = NULL;
				}
				if (sfScores[maxSize-1]) {
					SDL_FreeSurface(sfScores[maxSize-1]);
					sfScores[maxSize-1] = NULL;
				}
				if (sfLevels[maxSize-1]) {
					SDL_FreeSurface(sfLevels[maxSize-1]);
					sfLevels[maxSize-1] = NULL;
				}
			}
			for (int j = maxSize-1; j > i; --j) {
				// position will not be moved
				sfPlayerNames[j] = sfPlayerNames[j-1];
				sfScores[j]      = sfScores[j-1];
				sfLevels[j]      = sfLevels[j-1];
			}
			sfPlayerNames[i] = NULL;
			sfScores[i]      = NULL;
			sfLevels[i]      = NULL;
			idxLastInsertedEntry = i;
			return idxLastInsertedEntry;
		}
		++i;
	}
	if ((int) entries->size() < maxSize) {
		entries->push_back(entry);
		idxLastInsertedEntry = entries->size()-1;
	} else {
		idxLastInsertedEntry = -1;
	}
	return idxLastInsertedEntry;
}

void HighscoreList::print() {
	std::cout << "Highscore list:" << std::endl
	          << "Pos.| Name | Score | Level" << std::endl;
	int i = 0;
	for (std::vector<HighscoreEntry*>::iterator it = entries->begin(); it != entries->end(); ++it) {
		std::cout << (i+1) << " | " << (*it)->getPlayerName() << " | " << (*it)->getScore() << " | " << (*it)->getLevel();
		if (i == idxLastInsertedEntry)
			std::cout << " (new)";
		std::cout << std::endl;
		++i;
	}
}

void HighscoreList::draw(bool nameAlterable, bool highlightLast) {
	if (!sfTitle)
		sfTitle = Screen::getTextSurface(Screen::getVeryLargeFont(), "Highscore List", Constants::WHITE_COLOR);
	if (!sfPosTitle)
		sfPosTitle = Screen::getTextSurface(Screen::getFont(), "Pos.", Constants::WHITE_COLOR);
	if (!sfNameTitle)
		sfNameTitle = Screen::getTextSurface(Screen::getFont(), "Name", Constants::WHITE_COLOR);
	if (!sfScoreTitle)
		sfScoreTitle = Screen::getTextSurface(Screen::getFont(), "Score", Constants::WHITE_COLOR);
	if (!sfLevelTitle)
		sfLevelTitle = Screen::getTextSurface(Screen::getFont(), "Lev.", Constants::WHITE_COLOR);
	if (!sfBackItem)
		sfBackItem = Screen::getTextSurface(Screen::getLargeFont(), "back to menu", Constants::WHITE_COLOR);
	if (nameAlterable && !sfCaret && idxLastInsertedEntry >= 0)
		sfCaret = Screen::getTextSurface(Screen::getVeryLargeFont(), "-", Constants::YELLOW_COLOR);
	char ch_array[8];
	if (idxLastInsertedEntry < 0 || !highlightLast) {
		idxHighlightedEntry = -1;
	} else {
		if (idxLastInsertedEntry != idxHighlightedEntry) {
			idxHighlightedEntry = idxLastInsertedEntry;
			if (sfCurrentPos)
				SDL_FreeSurface(sfCurrentPos);
			sprintf(ch_array, "%d.", idxHighlightedEntry+1);
			sfCurrentPos = Screen::getTextSurface(Screen::getFont(), ch_array, Constants::YELLOW_COLOR);
			if (sfCurrentScore)
				SDL_FreeSurface(sfCurrentScore);
			sprintf(ch_array, "%d", entries->at(idxHighlightedEntry)->getScore());
			sfCurrentScore = Screen::getTextSurface(Screen::getFont(), ch_array, Constants::YELLOW_COLOR);
			if (sfCurrentLevel)
				SDL_FreeSurface(sfCurrentLevel);
			sprintf(ch_array, "%d", entries->at(idxHighlightedEntry)->getLevel());
			sfCurrentLevel = Screen::getTextSurface(Screen::getFont(), ch_array, Constants::YELLOW_COLOR);
		}
		if (sfCurrentName)
			SDL_FreeSurface(sfCurrentName);
		if (entries->at(idxHighlightedEntry)->getPlayerNameLength() == 0) {
			sfCurrentName = NULL;
		} else {
			sfCurrentName = Screen::getTextSurface(Screen::getFont(), entries->at(idxHighlightedEntry)->getPlayerName(), Constants::YELLOW_COLOR);
		}
		if (sfPlayerNames[idxHighlightedEntry])
			SDL_FreeSurface(sfPlayerNames[idxHighlightedEntry]);
		if (entries->at(idxHighlightedEntry)->getPlayerNameLength() == 0) {
			sfPlayerNames[idxHighlightedEntry] = NULL;
		} else {
			sfPlayerNames[idxHighlightedEntry] = Screen::getTextSurface(Screen::getFont(), entries->at(idxHighlightedEntry)->getPlayerName(), Constants::GRAY_COLOR);
		}
	}
	int maxWidthPosition = sfPosTitle->w;
	int maxWidthName     = sfNameTitle->w;
	int maxWidthScore    = sfScoreTitle->w;
	int maxWidthLevel    = sfLevelTitle->w;
	int i = 0;
	for (std::vector<HighscoreEntry*>::iterator it = entries->begin(); it != entries->end(); ++it) {
		if (!sfPositions[i]) {
			sprintf(ch_array, "%d.", i+1);
			sfPositions[i] = Screen::getTextSurface(Screen::getFont(), ch_array, Constants::GRAY_COLOR);
		}
		if (!sfPlayerNames[i] && (*it)->getPlayerNameLength()) {
			sfPlayerNames[i] = Screen::getTextSurface(Screen::getFont(), (*it)->getPlayerName(), Constants::GRAY_COLOR);
		}
		if (!sfScores[i]) {
			sprintf(ch_array, "%d", (*it)->getScore());
			sfScores[i] = Screen::getTextSurface(Screen::getFont(), ch_array, Constants::GRAY_COLOR);
		}
		if (!sfLevels[i]) {
			sprintf(ch_array, "%d", (*it)->getLevel());
			sfLevels[i] = Screen::getTextSurface(Screen::getFont(), ch_array, Constants::GRAY_COLOR);
		}
		if (sfPositions[i]->w > maxWidthPosition)
			maxWidthPosition = sfPositions[i]->w;
		if (nameAlterable && i==idxLastInsertedEntry) {
			if ((sfPlayerNames[i] ? sfPlayerNames[i]->w : 0) + sfCaret->w > maxWidthName)
				maxWidthName = (sfPlayerNames[i] ? sfPlayerNames[i]->w : 0) + sfCaret->w;
		} else {
			if ((sfPlayerNames[i] ? sfPlayerNames[i]->w : 0) > maxWidthName)
				maxWidthName = (sfPlayerNames[i] ? sfPlayerNames[i]->w : 0);
		}
		if (sfScores[i]->w > maxWidthScore)
			maxWidthScore = sfScores[i]->w;
		if (sfLevels[i]->w > maxWidthLevel)
			maxWidthLevel = sfLevels[i]->w;
		++i;
	}
	int totalWidth = maxWidthPosition+maxWidthName+maxWidthScore+maxWidthLevel + 3*Constants::HIGHSCORE_COLUMN_SPACING;
	if (totalWidth > Constants::WINDOW_WIDTH) {
		maxWidthName = maxWidthName - totalWidth + Constants::WINDOW_WIDTH;
		totalWidth   = Constants::WINDOW_WIDTH;
	}
	int x1 = (Constants::WINDOW_WIDTH - totalWidth) >> 1;
	int x2 = x1 + maxWidthPosition + Constants::HIGHSCORE_COLUMN_SPACING;
	int x3 = x2 + maxWidthName + Constants::HIGHSCORE_COLUMN_SPACING;
	int x4 = x3 + maxWidthScore + Constants::HIGHSCORE_COLUMN_SPACING;
	Screen::getInstance()->clear();
	Screen::getInstance()->draw(sfTitle, (Constants::WINDOW_WIDTH-sfTitle->w)>>1, 10);
	Screen::getInstance()->draw(sfPosTitle,   x1, 80);
	Screen::getInstance()->draw(sfNameTitle,  x2, 80);
	Screen::getInstance()->draw(sfScoreTitle, x3, 80);
	Screen::getInstance()->draw(sfLevelTitle, x4, 80);
	for (uint8_t i = 0; i < entries->size(); ++i) {
		int y = 120 + i*30;
		if (i == idxHighlightedEntry) {
			if (sfCurrentPos)
				Screen::getInstance()->draw(sfCurrentPos, x1 + maxWidthPosition - sfCurrentPos->w, y);
			if (sfCurrentName)
				Screen::getInstance()->draw(sfCurrentName, x2, y);
			if (nameAlterable)
				Screen::getInstance()->draw(sfCaret, x2 + (sfCurrentName ? sfCurrentName->w : 0), y-5);
			if (sfCurrentScore)
				Screen::getInstance()->draw(sfCurrentScore, x3 + maxWidthScore - sfCurrentScore->w, y);
			if (sfCurrentLevel)
				Screen::getInstance()->draw(sfCurrentLevel, x4 + maxWidthLevel - sfCurrentLevel->w - 10, y);
		} else {
			if (sfPositions[i])
				Screen::getInstance()->draw(sfPositions[i], x1 + maxWidthPosition - sfPositions[i]->w, y);
			if (sfPlayerNames[i])
				Screen::getInstance()->draw(sfPlayerNames[i], x2, y);
			if (sfScores[i])
				Screen::getInstance()->draw(sfScores[i], x3 + maxWidthScore - sfScores[i]->w, y);
			if (sfLevels[i])
				Screen::getInstance()->draw(sfLevels[i], x4 + maxWidthLevel - sfLevels[i]->w - 10, y);
		}
	}
	if (!nameAlterable)
		Screen::getInstance()->draw(sfBackItem, (Constants::WINDOW_WIDTH-sfBackItem->w)>>1, 430);
	Screen::getInstance()->addTotalUpdateRect();
	Screen::getInstance()->Refresh();
}

bool HighscoreList::eventloop(bool nameAlterable, bool *redrawNeeded) {
	SDL_Event event;
	bool refreshWindow = false;
	*redrawNeeded = false;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (nameAlterable) {
				bool upper = ((event.key.keysym.mod & KMOD_LSHIFT) | (event.key.keysym.mod & KMOD_RSHIFT)) > 0;
				if (event.key.keysym.sym == SDLK_RETURN) {
					if (entries->at(idxLastInsertedEntry)->getPlayerNameLength() == 0)
						entries->at(idxLastInsertedEntry)->setPlayerName("Pacman");  // default name if none has been entered
					if (sfPlayerNames[idxLastInsertedEntry]) {
						SDL_FreeSurface(sfPlayerNames[idxLastInsertedEntry]);
						sfPlayerNames[idxLastInsertedEntry] = NULL;  // has to be updated when drawn next time
					}
					return false;
				} else if (event.key.keysym.sym == SDLK_BACKSPACE) {
					entries->at(idxLastInsertedEntry)->removeLastCharFromPlayerName();
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_SPACE) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(' ');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_1 || event.key.keysym.sym == SDLK_KP_1) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('1');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_2 || event.key.keysym.sym == SDLK_KP_2) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('2');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_3 || event.key.keysym.sym == SDLK_KP_3) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('3');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_4 || event.key.keysym.sym == SDLK_KP_4) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('4');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_5 || event.key.keysym.sym == SDLK_KP_5) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('5');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_6 || event.key.keysym.sym == SDLK_KP_6) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('6');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_7 || event.key.keysym.sym == SDLK_KP_7) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('7');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_8 || event.key.keysym.sym == SDLK_KP_8) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('8');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_9 || event.key.keysym.sym == SDLK_KP_9) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('9');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_0 || event.key.keysym.sym == SDLK_KP_0) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName('0');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_a) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'A' : 'a');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_b) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'B' : 'b');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_c) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'C' : 'c');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_d) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'D' : 'd');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_e) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'E' : 'e');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_f) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'F' : 'f');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_g) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'G' : 'g');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_h) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'H' : 'h');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_i) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'I' : 'i');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_j) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'J' : 'j');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_k) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'K' : 'k');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_l) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'L' : 'l');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_m) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'M' : 'm');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_n) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'N' : 'n');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_o) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'O' : 'o');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_p) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'P' : 'p');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_q) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'Q' : 'q');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_r) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'R' : 'r');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_s) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'S' : 's');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_t) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'T' : 't');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_u) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'U' : 'u');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_v) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'V' : 'v');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_w) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'W' : 'w');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_x) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'X' : 'x');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_y) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'Y' : 'y');
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_z) {
					entries->at(idxLastInsertedEntry)->addCharToPlayerName(upper ? 'Z' : 'z');
					*redrawNeeded = true;
				}
			} else {
				if (event.key.keysym.sym == SDLK_RETURN) {
					return false;
				} else if (event.key.keysym.sym == SDLK_f) {
					Screen::getInstance()->toggleFullscreen();
					*redrawNeeded = true;
				} else if (event.key.keysym.sym == SDLK_s) {
					Sounds::getInstance()->toggleEnabled();
				} else if ((event.key.keysym.sym == SDLK_q)||(event.key.keysym.sym == SDLK_ESCAPE)) {
					return false;
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT && !nameAlterable) {
				if ((Constants::WINDOW_WIDTH-sfBackItem->w)>>1 <= event.motion.x && event.motion.x <= (Constants::WINDOW_WIDTH+sfBackItem->w)>>1 && 430 <= event.motion.y && event.motion.y <= 430+sfBackItem->h) {
					return false;
				}
			}
			break;
		case SDL_QUIT:
			return false;
		}
		if (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
			refreshWindow = true;
		}
	}
	if (refreshWindow) {
		Screen::getInstance()->addTotalUpdateRect();
		Screen::getInstance()->Refresh();
	}
	return true;
}

void HighscoreList::show(bool nameAlterable, bool highlightLast) {
	if (nameAlterable && idxLastInsertedEntry<0)
		return;  // not a new highscore, so do not allow the player to enter a name
	bool redrawNeeded, first = true;
	while (eventloop(idxLastInsertedEntry>=0 ? nameAlterable : false, &redrawNeeded)) {
		if (redrawNeeded || first) {
			first = false;
			if (idxLastInsertedEntry >= 0) {
				draw(nameAlterable, highlightLast);
			} else {
				draw(false, false);
			}
			redrawNeeded = false;
		}
		SDL_Delay(Constants::MIN_FRAME_DURATION);
	}
}

void HighscoreList::load() {
	int i = 0;
	for (std::vector<HighscoreEntry*>::iterator it = entries->begin(); it != entries->end(); ++it) {
		delete *it;
		if (sfPositions[i]) {
			SDL_FreeSurface(sfPositions[i]);
			sfPositions[i] = NULL;
		}
		if (sfPlayerNames[i]) {
			SDL_FreeSurface(sfPlayerNames[i]);
			sfPlayerNames[i] = NULL;
		}
		if (sfScores[i]) {
			SDL_FreeSurface(sfScores[i]);
			sfScores[i] = NULL;
		}
		if (sfLevels[i]) {
			SDL_FreeSurface(sfLevels[i]);
			sfLevels[i] = NULL;
		}
		++i;
	}
	entries->clear();
	for (int i = 0; i < maxSize; ++i) {
	}
	if (fileExists(filePath.c_str())) {
		std::ifstream f(filePath.c_str());
		if (f.is_open()) {
			std::string line;
			while (std::getline(f, line)) {
				int pos = line.find('|');
				if (pos >= 0) {
					std::string name = line.substr(0, pos);
					line = line.substr(pos+1);
					pos = line.find('|');
					if (pos >= 0) {
						int score = atoi(line.substr(0,pos).c_str());
						int level = atoi(line.substr(pos+1).c_str());
						entries->push_back(new HighscoreEntry(name, score, level));
						if (entries->size() >= maxSize)
							break;
					}
				}
			}
			f.close();
		} else {
			std::cerr << "Unable to read highscore file: " << filePath << std::endl;
		}
	}
}

void HighscoreList::save() {
	std::ofstream f(filePath.c_str());
	if (f.is_open()) {
		for (std::vector<HighscoreEntry*>::iterator it = entries->begin(); it != entries->end(); ++it) {
			f << (*it)->getPlayerName() << "|" << (*it)->getScore() << "|" << (*it)->getLevel() << "\n";
		}
		f.close();
	} else {
		std::cerr << "Unable to write highscore file: " << filePath << std::endl;
	}
}

HighscoreEntry *HighscoreList::getEntry(int index) {
	if (0 <= index && index < (int) entries->size()) {
		return entries->at(index);
	} else {
		return NULL;
	}
}