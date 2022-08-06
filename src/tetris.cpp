#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <algorithm>
#include <map>
#include <ctime>
#include <random>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// 22x10 Array of unsigned chars initialized to 0
std::vector<std::vector<unsigned char>> fieldMat(22, std::vector<unsigned char>(10, 0));

// Tetrominos
struct Tetromino {
    char x;
    char y;
	bool visible = true;

    std::vector<std::vector<unsigned char>> shape;
};

std::map<unsigned char, std::vector<std::vector<unsigned char>>> TETROMINO_SHAPES = {
    {'T', {{0, 1, 0},
           {1, 1, 1},
           {0, 0, 0}}
    },
    {'B', {{2, 2},
           {2, 2}}
    },
    {'S', {{0, 3, 3},
           {3, 3, 0},
           {0, 0, 0}}
    },
    {'Z', {{4, 4, 0},
           {0, 4, 4},
           {0, 0, 0}}
    },
    {'L', {{5, 5, 5},
           {0, 0, 5},
           {0, 0, 0}}
    },
    {'J', {{0, 0, 6},
           {6, 6, 6},
           {0, 0, 0}}
    },
    {'I', {{0, 0, 0, 0},
           {7, 7, 7, 7},
           {0, 0, 0, 0},
           {0, 0, 0, 0}}
    }
};

const std::vector<unsigned char> SHAPES_AVAILABLE = {'T', 'B', 'S', 'Z', 'L', 'J', 'I'};

enum Direction {
    DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
};

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const unsigned char BLOCK_SIZE = 28;  // px
const int OFFSET_X = 40;
const int OFFSET_Y = -40;
const int OFFSET_X_NEXT = SCREEN_WIDTH / 2 - 50;
const int OFFSET_Y_NEXT = 50;
const int INPUT_REPEAT_DELAY = 100;
const unsigned SEED = std::chrono::system_clock::now().time_since_epoch().count();

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Joystick* gGameController = NULL;
TTF_Font *gFont = NULL;

SDL_Texture** gPreRendered = NULL;
SDL_Texture** gPreRenderedGhost = NULL;

class LTexture {
    public:
        LTexture();
        ~LTexture();

        void free();
        void render(int x, int y);

        bool loadFromRenderedText(std::string textureText, SDL_Color textColor);

    private:
        int mWidth;
        int mHeight;
        SDL_Texture* mTexture;
};

LTexture::LTexture(){
   mTexture = NULL;
   mWidth = 0;
   mHeight = 0;
}

LTexture::~LTexture(){
    free();
}

void LTexture::free(){
    if(mTexture != NULL) {
        SDL_DestroyTexture(mTexture);
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}

void LTexture::render(int x, int y){
    // Set rendering space and render to screen
    SDL_SetRenderDrawColor(gRenderer, 0x00, 0xFF, 0x00, 0xFF);
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };

    SDL_RenderCopy(gRenderer, mTexture, NULL, &renderQuad);
}

bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor){
    // Clear previous texture
    free();

    //Render text surface
    SDL_Surface* textSurface = TTF_RenderText_Blended(gFont, textureText.c_str(), textColor);
    if (textSurface == NULL){
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    // Create texture from surface pixels
    mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
    if(mTexture == NULL){
        printf("Unable to create texture from rendered text! SDL Error %s\n", SDL_GetError());
        return false;
    }
    else {
        mWidth = textSurface->w;
        mHeight = textSurface->h;
    }
    
    // The surface is no longer needed 
    SDL_FreeSurface(textSurface);

    // Return success
    return true;

}

LTexture gTextTexture;

Uint32 getColorFromValue(const SDL_PixelFormat* format, unsigned char blockValue){
    switch(blockValue){
        case 1:
            return SDL_MapRGB(format, 0xFF, 0x00, 0x00);
        case 2:
            return SDL_MapRGB(format, 0x00, 0xFF, 0x00);
        case 3:
            return SDL_MapRGB(format, 0x00, 0x00, 0xFF);
        case 4:
            return SDL_MapRGB(format, 0xFF, 0xFF, 0x00);
        case 5:
            return SDL_MapRGB(format, 0xFF, 0x00, 0xFF);
        case 6:
            return SDL_MapRGB(format, 0x00, 0xFF, 0xFF);
        case 7:
            return SDL_MapRGB(format, 0xFF, 0x80, 0x00);
        default:
            return SDL_MapRGB(format, 0xFF, 0xFF, 0xFF);
            break;
    }
}


class InputManager {
	private:
		SDL_Event e;
		
		Direction stateDirection = DIR_NONE;
		bool stateDrop = false;
		bool stateRotate = false;
		bool stateReturn = false;
		bool stateQuit = false;

		bool checkInputTimer();
		Uint32 inputTimer = 0;


	public:
		Direction getStateDirection();
		bool getStateRotate();
		bool getStateDrop();
		bool getStateQuit();

		void processInput();

		InputManager();
		~InputManager();
};

InputManager::InputManager(){
}

InputManager::~InputManager(){
}


Direction InputManager::getStateDirection(){
	static Uint32 dirTimer = 0;
	static Direction prevDir = DIR_NONE;
	
	if (stateDirection == prevDir && prevDir != DIR_NONE)
		if (SDL_GetTicks() - dirTimer > INPUT_REPEAT_DELAY){
			dirTimer = SDL_GetTicks();
			return stateDirection;
		}
		else{
			return DIR_NONE;
		}
	else{
		prevDir = stateDirection;
		dirTimer = SDL_GetTicks();
		return stateDirection;
	}
}

bool InputManager::getStateDrop(){
	static Uint32 dropTimer = 0;
	static bool prevState = false;

	if (prevState != stateDrop || SDL_GetTicks() - dropTimer > 2*INPUT_REPEAT_DELAY){
		dropTimer = SDL_GetTicks();
		prevState = stateDrop;
		return stateDrop;
	}
	else
		return false;
}

bool InputManager::getStateRotate(){
	static Uint32 rotateTimer = 0;
	static bool prevState = false;

	if (prevState != stateRotate || SDL_GetTicks() - rotateTimer > 2*INPUT_REPEAT_DELAY){
		rotateTimer = SDL_GetTicks();
		prevState = stateRotate;
		return stateRotate;
	}
	else
		return false;
}

bool InputManager::getStateQuit(){
	return stateQuit;
}

void InputManager::processInput(){
	while (SDL_PollEvent(&e) != 0){
		// Window close event
		if (e.type == SDL_QUIT)
			stateQuit = true;

		// Keyboard
		else if (e.type == SDL_KEYDOWN){
			switch(e.key.keysym.sym){
				case SDLK_LEFT:
					stateDirection =  DIR_LEFT;
					break;
				case SDLK_RIGHT:
					stateDirection =  DIR_RIGHT;
					break;
				case SDLK_UP:
					stateDirection = DIR_UP;
					break;
				case SDLK_DOWN:
					stateDirection = DIR_DOWN;
					break;
				case SDLK_LSHIFT:
				case SDLK_RETURN:
					stateRotate = true;
					break;
				case SDLK_SPACE:
					stateDrop = true;
					break;
			}
		}

		else if (e.type == SDL_KEYUP){
			switch(e.key.keysym.sym){
				case SDLK_LEFT:
				case SDLK_RIGHT:
				case SDLK_UP:
				case SDLK_DOWN:
					stateDirection = DIR_NONE;
					break;
				case SDLK_LSHIFT:
				case SDLK_RETURN:
					stateRotate = false;
					break;
				case SDLK_SPACE:
					stateDrop = false;
					break;
			}
		}

		
		// Gamepad Buttons
		else if (e.type == SDL_JOYBUTTONDOWN){
			if(e.jbutton.button == 0)
				stateDrop = true;
			else if(e.jbutton.button == 1)
				stateRotate = true;
		}
		else if (e.type == SDL_JOYBUTTONUP){
			if(e.jbutton.button == 0)
				stateDrop = false;
			else if(e.jbutton.button == 1)
				stateRotate = false;
		}

		// Gamepad D-pad
		else if (e.type == SDL_JOYHATMOTION){
			if(e.jhat.value & SDL_HAT_RIGHT)
				stateDirection = DIR_RIGHT;
			else if(e.jhat.value & SDL_HAT_LEFT)
				stateDirection = DIR_LEFT;
			else if(e.jhat.value & SDL_HAT_UP)
				stateDirection = DIR_UP;
			else if(e.jhat.value & SDL_HAT_DOWN)
				stateDirection = DIR_DOWN;
			else {
				stateDirection = DIR_NONE;
			}
		}

	}
}

bool loadFonts(){
    gFont = TTF_OpenFont("consola.ttf", 24);
    if(gFont == NULL){
        printf("Could not load font! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }
    
    return true;
}

bool init(){
    bool success = true;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0){
        printf("SDL Could not initialize! SDL Error: %s\n", SDL_GetError());
        success = false;
    }
    else{
        gWindow = SDL_CreateWindow("Tetris Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
        if(gWindow == NULL){
            printf("Could not create window! SDL Error: %s\n", SDL_GetError());
            success = false;
        }
        else{
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
            if (gRenderer == NULL){
                printf("Could not create renderer: %s\n", SDL_GetError());
                success = false;
            }
            else{
                SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
            }
        }
    }

    if(TTF_Init() == -1){
        printf("SDL_ttf could not be initialized! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    if(!loadFonts()){
        printf("Could not load font! \n");
        return false;
    }

    gGameController = SDL_JoystickOpen(0);
    if(gGameController == NULL){
        printf("No game controllers detected: %s\n", SDL_GetError());
    }

    // Pre render textures of blocks
    gPreRendered = new SDL_Texture*[8];
    for (unsigned char i = 0; i < 8; i++){
        SDL_Surface* surf = SDL_CreateRGBSurface(0, BLOCK_SIZE, BLOCK_SIZE, 32, 0, 0, 0, 0);
        SDL_Rect blockOuter = {0, 0, BLOCK_SIZE, BLOCK_SIZE};
        SDL_Rect blockInner = {2, 2, BLOCK_SIZE - 4, BLOCK_SIZE - 4};

        SDL_FillRect(surf, &blockOuter, getColorFromValue(surf->format, i));
        SDL_FillRect(surf, &blockInner, 0.4*getColorFromValue(surf->format, i));
        gPreRendered[i] = SDL_CreateTextureFromSurface(gRenderer,  surf);

        SDL_FreeSurface(surf);
    }
	
	// Pre render ghost pieces
    gPreRenderedGhost = new SDL_Texture*[8];
    for (unsigned char i = 0; i < 8; i++){
        SDL_Surface* surf = SDL_CreateRGBSurface(0, BLOCK_SIZE, BLOCK_SIZE, 32, 0, 0, 0x00, 0x00);
        SDL_Rect blockOuter = {0, 0, BLOCK_SIZE, BLOCK_SIZE};
        SDL_Rect blockInner = {2, 2, BLOCK_SIZE - 4, BLOCK_SIZE - 4};

        SDL_FillRect(surf, &blockOuter, getColorFromValue(surf->format, i));
		SDL_FillRect(surf, &blockInner, SDL_MapRGB(surf->format, 0x00, 0x00, 0x00));
        gPreRenderedGhost[i] = SDL_CreateTextureFromSurface(gRenderer,  surf);
		SDL_SetTextureBlendMode(gPreRenderedGhost[i], SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(gPreRenderedGhost[i], 0x50);

        SDL_FreeSurface(surf);
    }

    return success;
}

void close(){
    // Free textures and fonts
    gTextTexture.free();
    TTF_CloseFont(gFont);
    gFont = NULL;
	for (unsigned char i = 0; i < 8; i++) {
		SDL_DestroyTexture(gPreRendered[i]);
		SDL_DestroyTexture(gPreRenderedGhost[i]);
	}

    delete[] gPreRendered;
    delete[] gPreRenderedGhost;
    
    // Destroy renderer
    SDL_DestroyRenderer(gRenderer);
    gRenderer = NULL;
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    
    // Close joysticks
    if(gGameController != NULL){
        SDL_JoystickClose(gGameController);
        gGameController = NULL;
    }
    
    // Quit subsystems
    TTF_Quit();
    SDL_Quit();
}

bool collidesWith(const Tetromino& tetromino, const std::vector<std::vector<unsigned char>>& fieldMat){
    // Checks if a tetromino collides with the field or the boundaries of the field
    
    // Loop trough each cell in the tetromino grid
    unsigned int tSize = tetromino.shape.size(); 

    for(unsigned char rt = 0; rt < tSize; rt++)
        for(unsigned char ct = 0; ct < tSize; ct++){
            if(tetromino.shape[rt][ct] == 0)  // Empty space can't collide
                continue;
            
            unsigned char rf = tetromino.y + rt;  // Calculate row in field for this block
            unsigned char cf = tetromino.x + ct;  // Calculate column in field for this block

            // Collides with field boundaries?
            if (rf >= fieldMat.size())
                return true;
            if (cf >= fieldMat[0].size())
                return true;
            
            // Collides with block on the field?
            if(fieldMat[rf][cf] != 0)
                return true;
        }
    
    // Checked all tetromino blocks and found no collision 
    return false;
}

bool move(Tetromino& tetromino, const std::vector<std::vector<unsigned char>>& fieldMat, const char direction){
    /* Attempts to move the tetromino 1 step in provided direction. 
     * Returns true if move is successful, or false if the move is
     * impossible due to a collision. */
     
    // Move the piece
    switch(direction){
        case DIR_UP:
            tetromino.y--;
            break;
        case DIR_DOWN:
            tetromino.y++;
            break;
        case DIR_LEFT:
            tetromino.x--;
            break;
        case DIR_RIGHT:
            tetromino.x++;
            break;
    }

    // Cancel the move on collision
    if(collidesWith(tetromino, fieldMat)){
        switch(direction){
            case DIR_UP:
                tetromino.y++;
                break;
            case DIR_DOWN:
                tetromino.y--;
                break;
            case DIR_LEFT:
                tetromino.x++;
                break;
            case DIR_RIGHT:
                tetromino.x--;
                break;
        }

        return false;
    }
    else
        return true;
}

void renderField(const std::vector<std::vector<unsigned char>>& fieldMat,
        const Tetromino& tetromino, const Tetromino& nextTetromino){

    // Renders blocks in FieldMat and the active tetromino
    SDL_Rect currentBlock = {0, 0, BLOCK_SIZE-1, BLOCK_SIZE-1};
    
	// Render field
    const std::vector<unsigned char>* field = &fieldMat[0];
    for (unsigned int r = 2; r < fieldMat.size(); r++){ // Don't render top 2 lines

        const unsigned char *vRow = &field[r][0];
        for (unsigned int c = 0; c < fieldMat[0].size(); c++){

            currentBlock.x = OFFSET_X + c * BLOCK_SIZE;
            currentBlock.y = OFFSET_Y + r * BLOCK_SIZE;
            
            if(vRow[c] > 0)
                SDL_RenderCopy(gRenderer, gPreRendered[vRow[c]], NULL, &currentBlock);
        }
    }
    
    // Render active tetromino
	if (tetromino.visible) {
		unsigned int tSize = tetromino.shape.size();
		const std::vector <unsigned char>* shape = &tetromino.shape[0];
		for (unsigned int r = 0; r < tSize; r++) {

			const unsigned char* vRowTetromino = &shape[r][0];
			for (unsigned int c = 0; c < tSize; c++) {
				currentBlock.x = OFFSET_X + (tetromino.x + c) * BLOCK_SIZE;
				currentBlock.y = OFFSET_Y + (tetromino.y + r) * BLOCK_SIZE;

				// Don't render the top 2 lines 
				if (tetromino.y + r < 2)
					continue;

				if (vRowTetromino[c] >= 1)
					SDL_RenderCopy(gRenderer, gPreRendered[vRowTetromino[c]], NULL, &currentBlock);
			}
		}
	}


	// Render ghost tetromino
	Tetromino ghost = {tetromino.x, tetromino.y, true, tetromino.shape};

	while (move(ghost, fieldMat, DIR_DOWN));  // Move ghost all the way down

	if (ghost.visible) {
		unsigned int tSize = ghost.shape.size();
		const std::vector <unsigned char>* shape = &ghost.shape[0];
		for (unsigned int r = 0; r < tSize; r++) {

			const unsigned char* vRowGhost = &shape[r][0];
			for (unsigned int c = 0; c < tSize; c++) {
				currentBlock.x = OFFSET_X + (ghost.x + c) * BLOCK_SIZE;
				currentBlock.y = OFFSET_Y + (ghost.y + r) * BLOCK_SIZE;

				// Don't render the top 2 lines 
				if (ghost.y + r < 2)
					continue;

				if (vRowGhost[c] >= 1)
					SDL_RenderCopy(gRenderer, gPreRenderedGhost[vRowGhost[c]], NULL, &currentBlock);
			}
		}
	}


	// Render next tetromino on the right
	unsigned int tSize = nextTetromino.shape.size();
	const std::vector<unsigned char>* shape = &nextTetromino.shape[0];
	for (unsigned int r = 0; r < tSize; r++) {

		const unsigned char* vRowTetromino = &shape[r][0];
		for (unsigned int c = 0; c < tSize; c++) {
			currentBlock.x = OFFSET_X_NEXT + c * BLOCK_SIZE;
			currentBlock.y = OFFSET_Y_NEXT + r * BLOCK_SIZE;

			if(vRowTetromino[c] >= 1)
				SDL_RenderCopy(gRenderer, gPreRendered[vRowTetromino[c]], NULL, &currentBlock);
		}
	}
}

bool rotate(Tetromino& tetromino, const std::vector<std::vector<unsigned char>>& fieldMat){
    /* Attemps to rotate the tetromino by 90 degrees. Returns true
     * if the rotation is successful, or false if the rotation is
     * impossible due to a collision */
    unsigned int tSize = tetromino.shape.size(); 
    std::vector<std::vector<unsigned char>> oldShape(tSize, std::vector<unsigned char>(tSize));
    std::vector<std::vector<unsigned char>> rotShape(tSize, std::vector<unsigned char>(tSize));

	// Keep old shape in case rotation fails
    oldShape = tetromino.shape;  

    for(unsigned char r = 0; r < tSize; r++)
        for(unsigned char c = 0; c < tSize; c++){
            rotShape[r][c] = tetromino.shape[tSize - 1 - c][r];
        }
    
	/* Check rotated tetromino for collisions, try kicks if necessary*/
    tetromino.shape = rotShape;
    if(collidesWith(tetromino, fieldMat)){ 
		tetromino.x++; // Attempt right kick
		if(collidesWith(tetromino, fieldMat)){
			tetromino.x -= 2;  // Attempt left kick
			if (collidesWith(tetromino, fieldMat)) {
				tetromino.x++;
		        tetromino.shape = oldShape;
				return false;  // Rotation failed

			}
		}
    }

	return true;
}

void renderBorder(const std::vector<std::vector<unsigned char>>& fieldMat){
    /* Draw the border of the playing field */
    
    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    int width = fieldMat[0].size() * BLOCK_SIZE;
    int height = (fieldMat.size() -2) * BLOCK_SIZE;  // Top 2 rows are invisible
    int border_offset_y = OFFSET_Y + BLOCK_SIZE * 2;

    SDL_Rect leftBorder = {OFFSET_X-5, border_offset_y, 5, height};
    SDL_RenderFillRect(gRenderer,  &leftBorder);

    SDL_Rect rightBorder = {static_cast<int>(OFFSET_X) + width, border_offset_y, 5, height + 5};
    SDL_RenderFillRect(gRenderer,  &rightBorder);

    SDL_Rect topBorder = {OFFSET_X-5, border_offset_y -5, width + 10, 5};
    SDL_RenderFillRect(gRenderer,  &topBorder);

    SDL_Rect bottomBorder = {OFFSET_X-5, border_offset_y + height, width + 5, 5};
    SDL_RenderFillRect(gRenderer,  &bottomBorder);
}

void updateTextInfo(Uint32 score){
    std::string scoreStr = "Score " + std::to_string(score);

    SDL_Color textColor{ 0, 0xFF, 0};
    gTextTexture.loadFromRenderedText(scoreStr, textColor);

    gTextTexture.render(SCREEN_WIDTH / 2 - 50, OFFSET_Y_NEXT + 125);
}

std::vector<std::vector<unsigned char>> getRandomShape(){
    static std::stack<unsigned char> shapesBag;

    // Add all shapes to the bag if its empty
    if (shapesBag.empty()){
        std::vector<unsigned char> shapeKeys = SHAPES_AVAILABLE;
        std::shuffle(shapeKeys.begin(), shapeKeys.end(), std::default_random_engine(SEED));

        for(unsigned char sk : shapeKeys){
            shapesBag.push(sk);
        }
    }

    // Pick one from the bag  
    unsigned char pickedShape = shapesBag.top();
    shapesBag.pop();
    
    return TETROMINO_SHAPES[pickedShape];
}

void freezeTetromino(const Tetromino& tetromino, std::vector<std::vector<unsigned char>>& fieldMat){
    /* Locks the tetromino into place then spawns a new one */
    
    for (unsigned char r = 0; r < tetromino.shape.size(); r++)
        for (unsigned char c = 0; c < tetromino.shape[0].size(); c++){
            if (tetromino.shape[r][c] == 0)  // Don't copy empty cells;
                continue;
            else
                fieldMat[r + tetromino.y][c + tetromino.x] = tetromino.shape[r][c];
        }
    
}

unsigned char flushFull(std::vector<std::vector<unsigned char>>& fieldMat){
    /* Flushes all full rows in the field. Returns number of lines flushed*/
    
    unsigned char nFlushed = 0;
    std::stack<unsigned char> flushStack;

    // Place rows that must be flushed on the stack
    for (unsigned int r = fieldMat.size() - 1; r > 0; r--){
        bool rowFull = true;
        for (unsigned int c = 0; c < fieldMat[0].size(); c++){
            if (fieldMat[r][c] == 0){
                rowFull = false;
                break;
            }

        }

        // Row can be flushed
        if (rowFull)
            flushStack.push(r);
    }     

    // Flush rows on the stack
    while(!flushStack.empty()){
        for (unsigned char r = flushStack.top(); r > 0; r--)
            for(unsigned char c = 0; c < fieldMat[0].size(); c++)
                fieldMat[r][c] = fieldMat[r - 1][c];

        flushStack.pop();
        nFlushed++;
    }

    return nFlushed;
}

int endTurn(Tetromino& tetromino, Tetromino& nextTetromino, std::vector<std::vector<unsigned char>>& fieldMat){
    /* Ends current turn. Returns points scored in this turn, or returns -1 on gameOver */

    // Freeze tetromino and flush
    freezeTetromino(tetromino, fieldMat);
    unsigned char nFlushed = flushFull(fieldMat);

	// Game over if player tops out
    for (unsigned char c = 0; c < fieldMat[0].size(); c++)
		if (fieldMat[1][c] > 0) {
			tetromino.visible = false;
            return -1;
		}
    
	// Copy shape of next tetromino to the active one
    tetromino.x = 4;
    tetromino.y = 0;
    tetromino.shape = nextTetromino.shape;

	// Obtain next tetromino
	nextTetromino.shape = getRandomShape();
	
	// Game over if the tetromino can't be placed
	if (collidesWith(tetromino, fieldMat))
		return -1;

    int turnScore = (nFlushed * nFlushed) * 100;
    
    return turnScore;
}

void gameLoop(){
	InputManager playerControls;

    bool quitGame = false;
    bool gameOver = false;
    bool shownGameOverMessage = false;
    bool forceTick = false;
    bool resetGame = false;

    Uint32 gameTime = 0;
    Uint32 tickTime = 1000;
	Uint32 maxTickTime = 1000;  // Starting speed is 1 tick/s
	
    Uint32 playerScore = 0;

    Tetromino activeTetromino = { 4, 0, true, getRandomShape() };
	Tetromino nextTetromino = { 4, 0, true, getRandomShape() };

    SDL_Delay(1000);
    while(!quitGame){
        gameTime = SDL_GetTicks();
		playerControls.processInput();
		
		if (playerControls.getStateQuit())
			quitGame = true;

		if(gameOver){
            if (!shownGameOverMessage){
                std::cout << "Game Over!\n";
                std::cout << "Press RETURN try again.\n";
                shownGameOverMessage = true;
            }

			if (playerControls.getStateRotate())
				resetGame = true;

            if (resetGame) {
                std::cout << "Game reset!" << std::endl;
                playerScore = 0;
                gameOver = false;
                shownGameOverMessage = false;
                gameTime = 0;
                activeTetromino = { 4, 0, true, getRandomShape()};
				maxTickTime = 1000;
                
                // Clear field
                for (unsigned char i = 0; i < fieldMat.size(); i++)
                    for (unsigned char j = 0; j < fieldMat[0].size(); j++)
                        fieldMat[i][j] = 0;
                resetGame = false;
            }
		}
		else{
			Direction direction = playerControls.getStateDirection();
			if (direction == DIR_LEFT || direction == DIR_RIGHT || direction == DIR_DOWN)
				move(activeTetromino, fieldMat, direction);

			if (playerControls.getStateRotate())
				rotate(activeTetromino, fieldMat);
			
			if (playerControls.getStateDrop()){
				while(move(activeTetromino, fieldMat, DIR_DOWN));
				forceTick = true;
			}

            // Game logic tick
            if (gameTime - tickTime > maxTickTime || forceTick){
                tickTime = gameTime;
                forceTick = false;
                
                // Try to move tetromino down, end turn if blocked
                if(!move(activeTetromino, fieldMat, DIR_DOWN)){
                    int turnScore = endTurn(activeTetromino, nextTetromino, fieldMat);
                    if(turnScore == -1)
                        gameOver = true;
                    else{
						// Gradually speed up the game after every clear, lower cap at 50ms
						if (turnScore > 0 && maxTickTime > 25)
							maxTickTime -= 10;
                        playerScore += turnScore;
					}
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(gRenderer);

        renderBorder(fieldMat);
        renderField(fieldMat, activeTetromino, nextTetromino);
        updateTextInfo(playerScore);

        SDL_RenderPresent(gRenderer); 
        SDL_Delay(10);  // Don't run too fast
    }
}

int main(int argc, char* args[]){
    // Seed random generator with current time
	std::srand(static_cast<unsigned int>(std::time(0)));
    init();

    gameLoop();
    close();

    printf("Bye!\n");
    return 0;
}
