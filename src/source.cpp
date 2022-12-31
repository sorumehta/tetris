
#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>
#include <thread>

const int WINDOW_WIDTH = 340;
const int WINDOW_HEIGHT = 680;
const int FONT_SIZE = 24;
const int FIELD_WIDTH = 12;
const int FIELD_HEIGHT = 18;

class LTexture {
private:
    SDL_Texture *mTexture;
    int mWidth;
    int mHeight;

public:
    LTexture();

    ~LTexture();

    bool loadTextureFromText(const std::string text, SDL_Color color);

    void render(int x, int y);

    int getWidth() const;

    int getHeight() const;

    void free();
};

LTexture gTextTexture;
SDL_Window *gWindow = nullptr;
SDL_Renderer *gRenderer = nullptr;
TTF_Font *gFont = nullptr;
unsigned char *pField = nullptr;
std::string tetromino[7];

LTexture::LTexture() {
    mTexture = nullptr;
    mWidth = 0;
    mHeight = 0;
}

int LTexture::getHeight() const { return mHeight; }

int LTexture::getWidth() const { return mWidth; }

bool LTexture::loadTextureFromText(const std::string text, SDL_Color color) {
    //free existing texture
    free();

    SDL_Surface *textSurface = TTF_RenderUTF8_Solid_Wrapped(gFont, text.c_str(), color, 0);
    if (textSurface == nullptr) {
        std::cout << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
    if (mTexture == nullptr) {
        std::cout << "Unable to create texture from rendered text! SDL Error:" << SDL_GetError() << std::endl;
        return false;
    }
    mWidth = textSurface->w;
    mHeight = textSurface->h;
    SDL_FreeSurface(textSurface);
    return true;
}

void LTexture::render(int x, int y) {
    SDL_Rect rect = {x, y, mWidth, mHeight};
    SDL_RenderCopy(gRenderer, mTexture, NULL, &rect);
}

void LTexture::free() {
    if (mTexture != nullptr) {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
        mWidth = 0;
        mHeight = 0;
    }
}

LTexture::~LTexture() {
    //Deallocate
    free();
}

bool sdl_init() {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL initialization failed: " << SDL_GetError();
        return false;
    }
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        std::cout << "Warning: Linear texture filtering not enabled!" << std::endl;
    }

    gWindow = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == nullptr) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError();
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == nullptr) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError();
        return false;
    }
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error:" << TTF_GetError();
        return false;
    }

    return true;

}

bool loadFont() {
    gFont = TTF_OpenFont("res/RobotoMono-Regular.ttf", FONT_SIZE);
    if (gFont == nullptr) {
        std::cout << "Failed to load RobotoMono font! SDL_ttf Error: " << TTF_GetError();
        return false;
    }
    return true;
}

bool loadText(const std::string text) {
    SDL_Color textColor = {0xFF, 0xFF, 0xFF};
    if (!gTextTexture.loadTextureFromText(text, textColor)) {
        return false;
    }
    return true;
}

void close() {
    //Free loaded images
    gTextTexture.free();

    //Free global font
    TTF_CloseFont(gFont);
    gFont = nullptr;

    //Destroy window
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = nullptr;
    gRenderer = nullptr;

    //Quit SDL subsystems
    TTF_Quit();
    SDL_Quit();

}

void drawFieldToScreen(char *screen, int screenWidth, int screenHeight) {
    for (int y = 0; y < screenHeight; y++) {
        for (int x = 0; x < screenWidth; x++) {
            if (x == screenWidth - 1) {
                screen[y * screenWidth + x] = '\n';
            } else {
                // draw the screen character based on the corresponding value in pField
                screen[y * screenWidth + x] = " X#"[pField[y * FIELD_WIDTH + x]];
            }
        }
    }
    screen[(screenHeight - 1) * screenWidth + screenWidth] = '\0';
}

void initPlayingField() {
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            pField[y * FIELD_WIDTH + x] = (x == 0 || x == FIELD_WIDTH - 1 || y == FIELD_HEIGHT - 1) ? 2 : 0;
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

void _printTetro(char *tetro) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            std::cout << tetro[y * 4 + x];
        }
        std::cout << std::endl;
    }
}

bool doesPieceFit(int tetrominoId, int rotation, int posX, int posY) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            int tetroIndex = Rotate(x, y, rotation);
            int fieldIndex = FIELD_WIDTH * (y + posY) + posX + x;
            if (posX + x >= 0 && posX + x < FIELD_WIDTH) {
                if (posY + y >= 0 && posY + y < FIELD_HEIGHT) {
                    if (tetromino[tetrominoId][tetroIndex] != '.' && pField[fieldIndex] != 0) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

int main(int argc, char *args[]) {
    pField = new unsigned char[FIELD_WIDTH * FIELD_HEIGHT];
    int screenWidth = FIELD_WIDTH + 1; //add 1 to FIELD_WIDTH for new line
    int screenHeight = FIELD_HEIGHT;
    char *screen = new char[screenWidth * screenHeight + 1]; // add 1 for null termination

    initPlayingField();


    tetromino[0] = "..X...X...X...X."; // Tetronimos 4x4
    tetromino[1] = "..X..XX...X.....";
    tetromino[2] = ".....XX..XX.....";
    tetromino[3] = "..X..XX..X......";
    tetromino[4] = ".X...XX...X.....";
    tetromino[5] = ".X...X...XX.....";
    tetromino[6] = "..X...X..XX.....";

    if (!sdl_init()) {
        std::cout << "error while initializing" << std::endl;
        close();
        return 0;
    }

    if (!loadFont()) {
        std::cout << "error while loading font" << std::endl;
        close();
        return 0;
    }

    int currPiece = 3;
    int currRotation = 0;
    int currXPos = FIELD_WIDTH / 2;
    int currYPos = 0;
    int newXPos;
    int newYPos;
    int newRotation;

    bool quit = false;
    int tickCounter = 0;
    int nTicksToMoveDown = 10;
    bool movePieceDown = false;
    const int millisecondsPerTick = 50;

    SDL_Event e;
    while (!quit) {
        // 0. INIT
        newXPos = currXPos;
        newYPos = currYPos;
        newRotation = currRotation;


        // 1. GAME TIMING
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsPerTick)); // one game tick
        tickCounter++;
        movePieceDown = tickCounter % nTicksToMoveDown == 0;
        tickCounter = (millisecondsPerTick * tickCounter) == 1000 ? 0 : tickCounter; // reset after one second

        // 2. USER INPUT
        while (SDL_PollEvent(&e) != 0) {
            //User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                //Select surfaces based on key press
                switch (e.key.keysym.sym) {
                    case SDLK_RIGHT:
                        newXPos++;
                        break;
                    case SDLK_LEFT:
                        newXPos--;
                        break;
                    case SDLK_DOWN:
                        newYPos++;
                        break;
                    case SDLK_z:
                        newRotation++;
                        break;
                }
            }
        }

        // 3. GAME LOGIC

        if (doesPieceFit(currPiece, newRotation, newXPos, newYPos)) {
            currXPos = newXPos;
            currYPos = newYPos;
            currRotation = newRotation;
        }
        if (movePieceDown) {
            newYPos++;
            if (doesPieceFit(currPiece, newRotation, newXPos, newYPos)) {
                currYPos = newYPos;
            } else {
                // Lock the current piece
                for (int y = 0; y < 4; y++) {
                    for (int x = 0; x < 4; x++) {
                        char pieceVal = tetromino[currPiece][Rotate(x, y, currRotation)];
                        if (pieceVal != '.') {
                            pField[(currYPos + y) * FIELD_WIDTH + currXPos + x] = 1;
                        }
                    }
                }

                // check for horizontal lines, and remove those lines
                bool lineFound[4];
                // start removing the lines from top to down match, by shifting all the lines lines above the match one line down
                for (int y = 0; y < 4; y++) {
                    if (y + currYPos < FIELD_HEIGHT - 1) {
                        bool isThisLine = true;
                        for (int x = 1; x < FIELD_WIDTH - 1; x++) {
                            if (pField[(y + currYPos) * FIELD_WIDTH + x] == 0) {
                                isThisLine = false;
                                break;
                            }
                        }
                        lineFound[y] = isThisLine;
                    }


                }
                for (int y = 0; y < 4; y++) {
                    if (lineFound[y]) {
                        std::cout << "Line Found at y position " << currYPos + y << std::endl;
                        for(int fY = y + currYPos; fY >= 2; fY--){
                            for (int fX = FIELD_WIDTH - 1; fX >= 0; fX--) {
                                int valToOverWrite = pField[(fY - 1) * (FIELD_WIDTH) + fX];
                                pField[(fY) * (FIELD_WIDTH) + fX] = valToOverWrite;
                            }
                        }

                    }
                }

                // choose next piece
                currXPos = FIELD_WIDTH / 2;
                currYPos = 0;
                currRotation = 0;
                currPiece = rand() % 7;

                // if new piece doesn't fit, game OVER
                quit = !doesPieceFit(currPiece, newRotation, currXPos, currYPos);

            }

        }

        // 4. RENDER OUTPUT
        // draw field to screen
        drawFieldToScreen(screen, screenWidth, screenHeight);

        // additionally, draw the current piece to screen
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                //draw if rotated block position is not empty
                char pieceCellValue = tetromino[currPiece][Rotate(x, y, currRotation)];
                if (pieceCellValue != '.') {
                    screen[(screenWidth) * (y + currYPos) + currXPos + x] = pieceCellValue;
                }
            }
        }

        if (!loadText(screen)) {
            std::cout << "error while loading texture from text" << std::endl;
            quit = true;
        }


        //clear screen
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gRenderer);

        //render current frame
        gTextTexture.render((WINDOW_WIDTH - gTextTexture.getWidth()) / 2,
                            (WINDOW_HEIGHT - gTextTexture.getHeight()) / 2);

        //update screen
        SDL_RenderPresent(gRenderer);

    }
    delete[] pField;
    delete[] screen;
    close();

    return 0;
}
