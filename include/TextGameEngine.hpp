#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <functional>


const int FONT_SIZE = 24;
const int FONT_WIDTH = 17;
const int FONT_HEIGHT = 34;


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

typedef struct ConsoleInfo{
    int windowWidth;
    int windowHeight;
    int nCharsX;
    int nCharsY;
    char *screenBuffer;
} ConsoleInfo;


LTexture gTextTexture;
SDL_Window *gWindow = nullptr;
SDL_Renderer *gRenderer = nullptr;
TTF_Font *gFont = nullptr;


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

ConsoleInfo *constructConsole(int nCharsX, int nCharsY) {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL initialization failed: " << SDL_GetError();
        return nullptr;
    }
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        std::cout << "Warning: Linear texture filtering not enabled!" << std::endl;
    }
    int windowWidth = nCharsX * FONT_WIDTH + 5;
    int windowHeight = nCharsY * FONT_HEIGHT + 5;
    gWindow = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               windowWidth, windowHeight, SDL_WINDOW_SHOWN); // 5 margin
    if (gWindow == nullptr) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError();
        return nullptr;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == nullptr) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError();
        return nullptr;
    }
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error:" << TTF_GetError();
        return nullptr;
    }
    char *screen = new char[(nCharsX + 1) * (nCharsY)]; // adding one to width for new lines
    ConsoleInfo *consoleInfo = new ConsoleInfo {windowWidth, windowHeight, nCharsX, nCharsY, screen};
    return consoleInfo;

}

bool createResources() {
    gFont = TTF_OpenFont("../res/unispace-rg.ttf", FONT_SIZE);
    if (gFont == nullptr) {
        std::cout << "Failed to load RobotoMono font! SDL_ttf Error: " << TTF_GetError();
        return false;
    }
    return true;
}

bool renderConsole(ConsoleInfo *console) {

    for(int y=0; y<console->nCharsY; y++){
        console->screenBuffer[y * (console->nCharsX + 1) + console->nCharsX] = '\n';
    }

    SDL_Color textColor = {0xFF, 0xFF, 0xFF};
    if (!gTextTexture.loadTextureFromText(console->screenBuffer, textColor)) {
        return false;
    }
    //clear screen
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);

    //render current frame
    gTextTexture.render((console->windowWidth - gTextTexture.getWidth()) / 2,
                        (console->windowHeight - gTextTexture.getHeight()) / 2);
    //update screen
    SDL_RenderPresent(gRenderer);
    return true;
}

void drawScreen(ConsoleInfo *console, int x, int y, char c){
    if(x >= 0 && x < console->nCharsX && y>=0 && y < console->nCharsY){
        console->screenBuffer[y * (console->nCharsX + 1) + x] = c;
    }
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
template <typename ... Args>
void startGameLoop(ConsoleInfo *consoleInfo, bool (*onFrameUpdate)(float, SDL_Event *, ConsoleInfo *, Args... ), Args ... onFrameUpdateArgs){
    bool quit = false;
    if (!createResources()) {
        std::cout << "error while loading resources" << std::endl;
        close();
        quit = true;
    }
    auto prevFrameTime = std::chrono::system_clock::now();
    auto currFrameTime = std::chrono::system_clock::now();

    while(!quit){
        // handle timing
        currFrameTime = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsedTime = currFrameTime - prevFrameTime;
        prevFrameTime = currFrameTime;
        float frameElapsedTime = elapsedTime.count();

        //handle input
        SDL_Event e;
        SDL_Event *userInput = nullptr;
        if (SDL_PollEvent(&e) != 0) {
            //User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
            } else {
                userInput = &e;
            }

        }
        onFrameUpdate(frameElapsedTime, userInput, consoleInfo, onFrameUpdateArgs...);

        // 4. RENDER OUTPUT
        
        if (!renderConsole(consoleInfo)) {
            std::cout << "error while loading texture from text" << std::endl;
            quit = true;
        }

    }
}