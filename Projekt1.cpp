/*
    ===================================================================
    FLYING BIRD GAME - C++ Windows Console Version
    ===================================================================
*/

#include <iostream>
#include <conio.h>      // _getch(), _kbhit()
#include <windows.h>    // SetConsoleCursorPosition, Sleep, Colors
#include <string>
#include <cstdlib>     // rand()
#include <vector>

using namespace std;

//------------------------------------------------
//------------  CONFIGURATION CONSTANTS ----------
//------------------------------------------------

#define QUIT        'q'
//#define REVERSE     ' '
#define FORWARD     'w'
#define BACKWARD    's'
#define LEFT	   'a'
#define RIGHT	   'd'
#define FRAME_TIME  50     // Milliseconds

// Window Dimensions
#define ROWS        20      // Height
#define COLS        60      // Width
#define OFFY        2       // Distance from top
#define OFFX        5       // Distance from left

// Star Parameters
#define MAX_STARS   5
#define STAR_SPAWN_RATE 10 // Frames
#define STARS_SPEED 350   // Milliseconds per cell
// Bird Parameters
#define BIRD_SPEED 50    // Milliseconds per cell

// Windows Color Codes (Foreground | Background)
// 0=Black, 1=Blue, 2=Green, 3=Cyan, 4=Red, 5=Purple, 6=Yellow, 7=White, 8=Gray...
// Bitwise OR (|) allows mixing, e.g., FOREGROUND_INTENSITY makes it bright
const int COL_MAIN = 15;    // White text
const int COL_BORDER = 3;   // Cyan
const int COL_BIRD = 12;    // Light Red
const int COL_STAT = 14;    // Yellow

//------------------------------------------------
//------------  CONSOLE HELPER FUNCTIONS ---------
//------------------------------------------------

// Move cursor to specific X, Y coordinates
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Change text color
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Hide the blinking cursor for a cleaner look
void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

DWORD GetTime() {
    return GetTickCount();
}

//------------------------------------------------
//------------  DATA STRUCTURES ------------------
//------------------------------------------------

struct GameWindow {
    int x, y;       // Top-left screen position
    int width, height;
    int color;

    // Draw the box border
    void drawBorder() {
        setColor(color);

        // Top Border
        gotoxy(x, y);
        cout << "+";
        for (int i = 0; i < width - 2; i++) cout << "-";
        cout << "+";

        // Side Borders
        for (int i = 1; i < height - 1; i++) {
            gotoxy(x, y + i); cout << "|";
            gotoxy(x + width - 1, y + i); cout << "|";
        }

        // Bottom Border
        gotoxy(x, y + height - 1);
        cout << "+";
        for (int i = 0; i < width - 2; i++) cout << "-";
        cout << "+";
    }

    // Write text inside the window
    void printAt(int relX, int relY, string text) {
        gotoxy(x + relX, y + relY);
        cout << text;
    }
};

struct SpriteCell
{
    char character;
    int color;
};

struct Sprite {
    int width, height;
    vector<vector<SpriteCell>> cells;
    int anchorX, anchorY; // Anchor point for positioning
};

struct Bird {
    GameWindow* parentWin;  // Which window is the bird inside?
    int x, y;               // Position RELATIVE to the window
    int dx, dy;             // Velocity
    int starsCollected;
    int speed;
    DWORD LastMoveTime;
    int life;
    Sprite sprite;          // Current sprite
    Sprite spriteUp;        // Directional sprites
    Sprite spriteDown;
    Sprite spriteLeft;
    Sprite spriteRight;
};

struct Star {
    GameWindow* parentWin;
    int x, y;
    char symbol;
    int color;
    bool active;
    int speed;
    DWORD LastMoveTime;
};

struct StarSpawner {
    GameWindow* parentWin;
    vector<Star> stars;
    int spawnRate;
    int maxStars;
    DWORD lastSpawnTime;
    DWORD nextSpawnTime;
};



Star CreateStar(GameWindow* win) {
    Star s;
    s.parentWin = win;
    s.x = rand() % (win->width - 2) + 1; // Avoid borders
    s.y = 1; // Start at top
    s.symbol = '+';
    s.color = 14; // Yellow
    s.active = true;
    s.speed = STARS_SPEED;
	s.LastMoveTime = GetTime();
    return s;
}

Sprite CreateSprite(int width, int height, int anchorX = 0, int anchorY = 0) {
    Sprite sprite;
    sprite.width = width;
    sprite.height = height;
    sprite.cells.resize(height);
    sprite.anchorX = anchorX;
    sprite.anchorY = anchorY;

    for(int i = 0; i < height; i++) {
        sprite.cells[i].resize(width);
        for(int j = 0; j < width; j++) {
            sprite.cells[i][j].character = ' ';
            sprite.cells[i][j].color = COL_MAIN;
        }
	}

    return sprite;
}

Sprite CreateBirdSprite(const vector<string>& lines, int default_color, int anchorX = 0, int anchorY = 0) 
{
	int height = lines.size();
    int width = 0;

    for(size_t i = 0; i < lines.size(); i++) {
        if (lines[i].length() > width) {
            width = lines[i].length();
        }
	}

	Sprite sprite = CreateSprite(width, height, anchorX, anchorY);

    for(int y = 0; y < height; y++) {
        for(int x = 0; x < lines[y].length(); x++) {
            char ch = lines[y][x];
            sprite.cells[y][x].character = ch;
            if (ch != ' ') {
                sprite.cells[y][x].color = default_color;
            }
        }
	}

    return sprite;
}

Sprite CreateBirdSpriteRight() {
    vector<string> design = {
        ">=>"
    };
	return CreateBirdSprite(design, 14, 2, 0);
}
Sprite CreateBirdSpriteLeft() {
    vector<string> design = {
        "<=<"
    };
    return CreateBirdSprite(design, 14, 0, 0);
}
Sprite CreateBirdSpriteUp() {
    vector<string> design = {
        "^",
        "|",
        "^"
    };
    return CreateBirdSprite(design, 14, 0, 0);
}
Sprite CreateBirdSpriteDown() {
    vector<string> design = {
        ".",
        "|",
		"'"
    };
    return CreateBirdSprite(design, 14, 0, 2);
}

//------------------------------------------------
//------------  GAME LOGIC -----------------------
//------------------------------------------------

void DrawSprite(GameWindow* win, Sprite* sprite, int posX, int posY) {
    for (int y = 0; y < sprite->height; y++) {
        for (int x = 0; x < sprite->width; x++) {
            // Calculate screen position relative to anchor
            int screenX = win->x + posX + (x - sprite->anchorX);
            int screenY = win->y + posY + (y - sprite->anchorY);

            // Only draw if within window bounds
            if (screenX > win->x && screenX < win->x + win->width - 1 &&
                screenY > win->y && screenY < win->y + win->height - 1) {

                char ch = sprite->cells[y][x].character;
                if (ch != ' ') {  // Don't draw spaces (transparent)
                    setColor(sprite->cells[y][x].color);
                    gotoxy(screenX, screenY);
                    cout << ch;
                }
            }
        }
    }
}

void ClearSprite(GameWindow* win, Sprite* sprite, int posX, int posY) {
    for (int y = 0; y < sprite->height; y++) {
        for (int x = 0; x < sprite->width; x++) {
            int screenX = win->x + posX + (x - sprite->anchorX);
            int screenY = win->y + posY + (y - sprite->anchorY);

            if (screenX > win->x && screenX < win->x + win->width - 1 &&
                screenY > win->y && screenY < win->y + win->height - 1) {

                if (sprite->cells[y][x].character != ' ') {
                    gotoxy(screenX, screenY);
                    cout << " ";
                }
            }
        }
    }
}

void DrawBird(Bird* b) {
    DrawSprite(b->parentWin, &b->sprite, b->x, b->y);
}

void ClearBird(Bird* b) {
	ClearSprite(b->parentWin, &b->sprite, b->x, b->y);
}

void updateBirdSprite(Bird* b, char direction) {
    switch (direction)
    {
    case 'F':
        b->sprite = b->spriteUp;
        break;
    case 'S':
        b->sprite = b->spriteDown;
        break;
    case 'L':
        b->sprite = b->spriteLeft;
        break;
    case 'R':
        b->sprite = b->spriteRight;
        break;
    default:
        break;
    }
}

void MoveBird(Bird* b, char direction) {
    // 1. Erase old position
	DWORD currentTime = GetTime();
	if (currentTime - b->LastMoveTime < b->speed)return;
	b->LastMoveTime = currentTime;
    ClearBird(b);

    // 2. Calculate bounds (inside the border)
    // Valid X is 1 to Width-2 (because 0 and Width-1 are borders)
    // Valid Y is 1 to Height-2
    int minX = 1;
    int maxX = b->parentWin->width - 2;
    int minY = 1;
    int maxY = b->parentWin->height - 2;

    // 3. Calculate new position
    int nextX = b->x + b->dx;
    int nextY = b->y + b->dy;

	// 4. Collision detection with borders
    if (nextX < minX) {
        nextX = minX;
        b->dx = -b->dx; // Reverse direction
    }
    else if (nextX > maxX) {
        nextX = maxX;
        b->dx = -b->dx; // Reverse direction
	}
    if (nextY < minY) {
        nextY = minY;
        b->dy = -b->dy; // Reverse direction
    }
    else if (nextY > maxY) {
        nextY = maxY;
        b->dy = -b->dy; // Reverse direction
	}
	// 5. Update position
    switch (direction)
    {
        case 'F':
            b->y = nextY;
		    break;
        case 'S':
            b->y = nextY;
            break;
        case 'L':
			b->x = nextX;
            break;
		case 'R':
			b->x = nextX;
			break;
    default:
        break;
    }


    // 6. Draw new position
    DrawBird(b);
}

void InitSpawner(StarSpawner* spawner, GameWindow* win) {
    spawner->parentWin = win;
    spawner->spawnRate = STAR_SPAWN_RATE;
    spawner->maxStars = MAX_STARS;
	spawner->stars.clear();
    spawner->lastSpawnTime = GetTime();
	spawner->nextSpawnTime = spawner->spawnRate;
	spawner->stars.push_back(CreateStar(win));
}

int CountActiveStars(StarSpawner* spawner) {
    int count = 0;
    for (size_t i = 0; i < spawner->stars.size(); i++) {
        if (spawner->stars[i].active) count++;
    }
    return count;
}

void TryStar(StarSpawner* spawner) {
    DWORD currentTime = GetTime();
	if (currentTime - spawner->lastSpawnTime < spawner->nextSpawnTime ) return;
    if (CountActiveStars(spawner) >= spawner->maxStars) 
    {
		spawner->lastSpawnTime = currentTime;
		spawner->nextSpawnTime = spawner->lastSpawnTime + spawner->spawnRate;
		return; // Already at max stars
    };

    bool foundSlot = false;
    for (size_t i = 0; i < spawner->stars.size(); i++) {
        if (!spawner->stars[i].active) {
            // Reuse this slot
            spawner->stars[i] = CreateStar(spawner->parentWin);
            foundSlot = true;
            break;
        }
    }

    // No inactive slot found, add new star
    if (!foundSlot) {
        spawner->stars.push_back(CreateStar(spawner->parentWin));
    }

}

void UpdateStatus(GameWindow* statWin, Bird* b) {
    setColor(statWin->color);
    // Clear the status line first (simple way: print spaces)
    statWin->printAt(2, 1, "                                           ");

    char buffer[50];
    sprintf_s(buffer, "Pos: %d,%d Level: 0 Stars: %d life: %d  [Q]=Quit", b->x, b->y, b->starsCollected, b->life);
    statWin->printAt(2, 1, buffer);
}

void DrawStar(Star* s) {
    setColor(s->color);
    gotoxy(s->parentWin->x + s->x, s->parentWin->y + s->y);
    cout << s->symbol;
}

void ClearStar(Star* s) {
    gotoxy(s->parentWin->x + s->x, s->parentWin->y + s->y);
    cout << " ";
}

void RespawnStar(Star* s) {
    int maxX = s->parentWin->width - 2;
	s->color = 14; // Reset color to yellow
    s->x = rand() % (maxX - 1) + 1;
    s->y = 1;
    s->active = true;
}

void MoveStar(Star* s) {
    if (!s->active) return;

	DWORD currentTime = GetTime();
	if (currentTime - s->LastMoveTime < s->speed) return;
	s->LastMoveTime = currentTime;

    ClearStar(s);
    int minX = 1;
    int maxX = s->parentWin->width - 2;
    int minY = 1;
    int maxY = s->parentWin->height - 2;

    int nextY = s->y + 1;
    s->y = nextY;

    if (s->y > ROWS / 2) {
        s->color = 9;
    }
    else if (s->y > ROWS * 2 / 3)
    {
		s->color = 4;
    }

    if (nextY > maxY - 1) {
        nextY = maxY;
        RespawnStar(s);
    }

    DrawStar(s);
}

void UpdateStars(StarSpawner* spawner) {
    // Try to spawn new stars
    TryStar(spawner);

    // Move all active stars
    for (size_t i = 0; i < spawner->stars.size(); i++) {
        MoveStar(&spawner->stars[i]);
    }
}

void DrawAllStars(StarSpawner* spawner) {
    for (size_t i = 0; i < spawner->stars.size(); i++) {
        if (spawner->stars[i].active) {
            DrawStar(&spawner->stars[i]);
        }
    }
}

bool CheckSpriteCollision(Bird* b, Star* s) {
    if (!s->active) return false;

    Sprite* sprite = &b->sprite;

    for (int y = 0; y < sprite->height; y++) {
        for (int x = 0; x < sprite->width; x++) {
            if (sprite->cells[y][x].character != ' ') {
                // Calculate world position of this sprite cell
                int cellX = b->x + (x - sprite->anchorX);
                int cellY = b->y + (y - sprite->anchorY);

                if (cellX == s->x && cellY == s->y) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CheckAllCollisions(Bird* b, StarSpawner* spawner) {
    for (size_t i = 0; i < spawner->stars.size(); i++) {
        if (CheckSpriteCollision(b, &spawner->stars[i])) {
            ClearStar(&spawner->stars[i]);
            b->starsCollected++;
            RespawnStar(&spawner->stars[i]);
        }
    }
}

//void CollectStar(Bird* b, Star* s) {
//    if (CheckCollision(b, s)) {
//        ClearStar(s);
//        b->starsCollected++;
//        RespawnStar(s);
//    }
//}

//------------------------------------------------
//------------  MAIN -----------------------------
//------------------------------------------------

int main() {
    // 1. Setup Console
    hideCursor();
    system("cls"); // Clear standard cmd screen
    system("title Flying Bird Game");
    srand(time(0));

    // 2. Initialize Windows
    GameWindow playArea = { OFFX, OFFY, COLS, ROWS, COL_BORDER };
    playArea.drawBorder();

    GameWindow statArea = { OFFX, OFFY + ROWS, COLS, 3, COL_STAT };
    statArea.drawBorder();

    // 3. Initialize Bird
    // Start in middle, moving down-right
    Bird bird;
    bird.parentWin = &playArea;
    bird.x = COLS / 2;
    bird.y = ROWS / 2;
    bird.dx = 1;
    bird.dy = 1;
	bird.starsCollected = 0;
	bird.speed = BIRD_SPEED;
	bird.LastMoveTime = GetTime();
    bird.life = 3;
    bird.spriteUp = CreateBirdSpriteUp();
    bird.spriteDown = CreateBirdSpriteDown();
    bird.spriteLeft = CreateBirdSpriteLeft();
    bird.spriteRight = CreateBirdSpriteRight();
    bird.sprite = bird.spriteRight;  // Start facing right

    StarSpawner spawner;
	InitSpawner(&spawner, &playArea);

    DrawBird(&bird);

    // 4. Main Loop
    bool running = true;
	char direction = ' '; // Placeholder for future direction handling
    while (running) {
        // Input Handling (Non-blocking)
        if (_kbhit()) {
            char ch = _getch();
            switch(ch) {
                case QUIT:
                    running = false;
                    break;
                case FORWARD:
					ClearBird(&bird);
                    if (bird.dy < 0)
                    {
                        direction = 'F';
						updateBirdSprite(&bird, direction);
                    }
                    else
						bird.dy = -bird.dy;
					    direction = 'F';
						updateBirdSprite(&bird, direction);
                    break;
                case BACKWARD:
                    ClearBird(&bird);
                    if (bird.dy > 0)
                    {
                        direction = 'S';
						updateBirdSprite(&bird, direction);
                    }
                    else
						bird.dy = -bird.dy;
					    direction = 'S';
						updateBirdSprite(&bird, direction);
                    break;
                case LEFT:
                    ClearBird(&bird);
                    if (bird.dx < 0)
                    { 
                        direction = 'L';
						updateBirdSprite(&bird, direction);
                    }
                    else
                        bird.dx = -bird.dx;
					    direction = 'L';
						updateBirdSprite(&bird, direction);
                    break;
                case RIGHT:
                    ClearBird(&bird);
                    if (bird.dx > 0)
                    {
                        direction = 'R';
						updateBirdSprite(&bird, direction);
                    }
                    else
                        bird.dx = -bird.dx;
                        direction = 'R';
						updateBirdSprite(&bird, direction);
                    break;
			}
        }

        // Logic Updates
        MoveBird(&bird, direction);
		UpdateStars(&spawner);
        CheckAllCollisions(&bird, &spawner);
        UpdateStatus(&statArea, &bird);

        // Timing
        Sleep(FRAME_TIME);
    }

    // 5. Cleanup
    system("cls");
    setColor(COL_MAIN);
    cout << "Game Over. Thanks for playing!" << endl;
    Sleep(1000);

    return 0;
}    