
#include "TextGameEngine.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <functional>

// tetris global variables
unsigned char *pField = nullptr;
std::string tetromino[7];
int score = 0;

enum pFieldEnum {
    EMPTY, BLOCK, BOUNDARY, LINE_TO_REMOVE
};
const std::string pFieldChars = " X#="; //characters corresponding to pFieldEnum

void initPlayingField(int fieldWidth, int fieldHeight) {
    for (int y = 0; y < fieldHeight; y++) {
        for (int x = 0; x < fieldWidth; x++) {
            pField[y * fieldWidth + x] = (x == 0 || x == fieldWidth - 1 || y == fieldHeight - 1) ? BOUNDARY : EMPTY;
        }
    }
}

int Rotate(int px, int py, int r) {
    int pi = 0;
    switch (r % 4) {
        case 0: // 0 degrees			// 0  1  2  3
            pi = py * 4 + px;            // 4  5  6  7
            break;                        // 8  9 10 11
            //12 13 14 15

        case 1: // 90 degrees			//12  8  4  0
            pi = 12 + py - (px * 4);    //13  9  5  1
            break;                        //14 10  6  2
            //15 11  7  3

        case 2: // 180 degrees			//15 14 13 12
            pi = 15 - (py * 4) - px;    //11 10  9  8
            break;                        // 7  6  5  4
            // 3  2  1  0

        case 3: // 270 degrees			// 3  7 11 15
            pi = 3 - py + (px * 4);        // 2  6 10 14
            break;                        // 1  5  9 13
    }                                    // 0  4  8 12
    return pi;
}

int field_width = 12;
int field_height = 18;
int currPiece = 3;
int currRotation = 0;
int currXPos = field_width / 2;
int currYPos = 0;
int newXPos;
int newYPos;
int newRotation;

int tickCounter = 0;
int nTicksToMoveDown = 10;
bool movePieceDown;
const int millisecondsPerTick = 50;
int pieceCount = 0;


bool drawBuffer(ConsoleInfo *console, int currPiece, int currXPos, int currYPos, int currRotation) {
    for (int y = 0; y < console->nCharsY-1; y++) {
        for (int x = 0; x < console->nCharsX; x++) {
            // draw the screen character based on the corresponding value in pField
            drawScreen(console, x, y,  pFieldChars[pField[y * console->nCharsX + x]]);
        }
    }

    // additionally, draw the current piece to screen
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            //draw if rotated block position is not empty
            char pieceCellValue = tetromino[currPiece][Rotate(x, y, currRotation)];
            if (pieceCellValue != '.') {
                drawScreen(console, currXPos + x, y + currYPos, pieceCellValue);
            }
        }
    }
    // draw score to the last line in the screen
    std::string score_chars = "score = " + std::to_string(score);
    for(int i=0; i< score_chars.size(); i++){
        drawScreen(console, i, console->nCharsY-1, score_chars[i]);
    }

    return true;

}


bool doesPieceFit(int tetrominoId, int rotation, int posX, int posY, int field_width, int field_height) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            int tetroIndex = Rotate(x, y, rotation);
            int fieldIndex = field_width * (y + posY) + posX + x;
            if (posX + x >= 0 && posX + x < field_width) {
                if (posY + y >= 0 && posY + y < field_height) {
                    if (tetromino[tetrominoId][tetroIndex] != '.' && pField[fieldIndex] != EMPTY) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void initState(){
    // pField holds the values of the cells in the playing field
    pField = new unsigned char[field_width * field_height];

    initPlayingField(field_width, field_height);


    tetromino[0] = "..X...X...X...X."; // Tetronimos 4x4
    tetromino[1] = "..X..XX...X.....";
    tetromino[2] = ".....XX..XX.....";
    tetromino[3] = "..X..XX..X......";
    tetromino[4] = ".X...XX...X.....";
    tetromino[5] = ".X...X...XX.....";
    tetromino[6] = "..X...X..XX.....";
}

bool onNextFrame(float fElapsedTime, SDL_Event *e, ConsoleInfo *console){
    newXPos = currXPos;
    newYPos = currYPos;
    newRotation = currRotation;


    // 1. GAME TIMING
    std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsPerTick)); // one game tick
    tickCounter++;
    movePieceDown = tickCounter % nTicksToMoveDown == 0;
    tickCounter = (millisecondsPerTick * tickCounter) >= 1000 ? 0 : tickCounter; // reset after one second

    // 2. USER INPUT
    if (e != nullptr && e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
            case SDLK_RIGHT:
                newXPos = currXPos + 1;
                break;
            case SDLK_LEFT:
                newXPos = currXPos - 1;
                break;
            case SDLK_DOWN:
                newYPos = currYPos + 1;
                break;
            case SDLK_z:
                newRotation = currRotation + 1;
                break;
        }
    }


    // 3. GAME LOGIC
    std::vector<int> lineFoundAt;
    if (movePieceDown) {
        // only move in one direction at a time.
        newYPos = currYPos + 1;
        newXPos = currXPos;
        if (doesPieceFit(currPiece, newRotation, newXPos, newYPos, field_width, field_height)) {
            currYPos = newYPos;
        } else {
            // Lock the current piece
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    char pieceVal = tetromino[currPiece][Rotate(x, y, currRotation)];
                    if (pieceVal != '.') {
                        pField[(currYPos + y) * field_width + currXPos + x] = BLOCK;
                    }
                }
            }
            score += 3;
            pieceCount++;
            if(pieceCount % 10 == 0){
                nTicksToMoveDown--;
            }

            // check for horizontal lines, and remove those lines

            for (int y = 0; y < 4; y++) {
                if (y + currYPos < field_height - 1) {
                    bool isThisLine = true;
                    for (int x = 1; x < field_width - 1; x++) {
                        if (pField[(y + currYPos) * field_width + x] == 0) {
                            isThisLine = false;
                            break;
                        }
                    }
                    if (isThisLine) {
                        lineFoundAt.push_back(y + currYPos);
                        // update the field to show the visual of matched line
                        for (int x = 1; x < field_width - 1; x++) {
                            pField[(y + currYPos) * field_width + x] = LINE_TO_REMOVE;
                        }
                    }
                }
            }

            // choose next piece
            currXPos = field_width / 2;
            currYPos = 0;
            currRotation = 0;
            currPiece = rand() % 7;

            // if new piece doesn't fit, game OVER
            if(!doesPieceFit(currPiece, newRotation, currXPos, currYPos, field_width, field_height))
                return false;

        }

    } else {
        // only move in one direction at a time
        if (newYPos != currYPos) {
            newXPos = currXPos;
        }
        if (doesPieceFit(currPiece, newRotation, newXPos, newYPos, field_width, field_height)) {
            currXPos = newXPos;
            currYPos = newYPos;
            currRotation = newRotation;
        }
    }

    // remove the lines to remove from top to bottom match,
    // by shifting down all the lines above it
    if (!lineFoundAt.empty()) {
        for (int yToRemove: lineFoundAt) {
            for (int fY = yToRemove; fY > 1; fY--) {
                for (int fX = field_width - 1; fX >= 0; fX--) {
                    pField[(fY) * (field_width) + fX] = pField[(fY - 1) * (field_width) + fX];
                }
            }
        }
        score += (1 << lineFoundAt.size()) * 10;
    }
    // draw field to screen
    if(!drawBuffer(console, currPiece, currXPos, currYPos, currRotation)){
        return false;
    }
    return true;
}

int main(int argc, char *args[]) {

    ConsoleInfo *consoleInfo = constructConsole(field_width, field_height+1); // add one to height for score
    if (consoleInfo == nullptr) {
        std::cout << "error while initializing" << std::endl;
        close();
        return 0;
    }

    startGameLoop(initState, onNextFrame, consoleInfo);

    delete[] consoleInfo->screenBuffer;
    delete consoleInfo;
    delete[] pField;
    close();

    return 0;
}
