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
#define FRAME_TIME  100     // Milliseconds

// Window Dimensions
#define ROWS        20      // Height
#define COLS        60      // Width
#define OFFY        2       // Distance from top
#define OFFX        5       // Distance from left

// Star Parameters
#define MAX_STARS   10
#define STAR_SPAWN_RATE 20  // Frames

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

struct Bird {
    GameWindow* parentWin;  // Which window is the bird inside?
    int x, y;               // Position RELATIVE to the window
    int dx, dy;             // Velocity
    char symbol;
    int color;
    int starsCollected;
    int speed;
    int life;
};

struct Star {
    GameWindow* parentWin;
    int x, y;
    char symbol;
    int color;
    bool active;
    int speed;
};

struct StarSpawner {
	GameWindow* parentWin;
    vector<Star> stars;
	int spawnRate;
	int maxStars;
	DWORD lastSpawnTime;
	DWORD nextSpawnTime;
};

//------------------------------------------------
//------------  GAME LOGIC -----------------------
//------------------------------------------------

void DrawBird(Bird* b) {
    setColor(b->color);
    // We add parentWin->x/y to convert relative coordinates to screen coordinates
    gotoxy(b->parentWin->x + b->x, b->parentWin->y + b->y);
    cout << b->symbol;
}

void ClearBird(Bird* b) {
    // Overwrite with empty space
    gotoxy(b->parentWin->x + b->x, b->parentWin->y + b->y);
    cout << " ";
}

void MoveBird(Bird* b, char direction) {
    // 1. Erase old position
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

Star CreateStar(GameWindow* win) {
    Star s;
    s.parentWin = win;
    s.x = rand() % (win->width - 2) + 1; // Avoid borders
    s.y = 1; // Start at top
    s.symbol = '+';
    s.color = 14; // Yellow
    s.active = true;
    s.speed = 1;
    return s;
}

void InitSpawner(Starspawner* spawner, GameWindow* win) {
    spawner->parentWin = win;
    spawner->spawnRate = STAR_SPAWN_RATE;
    spawner->maxStars = MAX_STARS;
	spawner->stars.clear();
    spawner->lastSpawnTime = GetTime();
	spawner->nextSpawnTime = lastSpawnTime + spawner->spawnRate;
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
    s->x = rand() % (maxX - 1) + 1;
    s->y = 1;
    s->active = true;
}

void UpdateStars(StarSpawner* spawner) {
    // Try to spawn new stars
    TrySpawnStar(spawner);

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

bool CheckCollision(Bird* b, Star* s) {

    return (s->active && b->x == s->x && b->y == s->y);
}

void CheckAllCollisions(Bird* b, StarSpawner* spawner) {
    for (size_t i = 0; i < spawner->stars.size(); i++) {
        if (CheckCollision(b, &spawner->stars[i])) {
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

void MoveStar(Star* s) {
	if (!s->active) return;
    ClearStar(s);
    int minX = 1;
    int maxX = s->parentWin->width - 2;
    int minY = 1;
    int maxY = s->parentWin->height - 2;

	int nextY = s->y + 1;
	s->y = nextY;
    if (nextY > maxY - 1) {
        nextY = maxY;
        RespawnStar(s);
    }
    
    DrawStar(s);
}

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
    bird.symbol = '*';
    bird.color = COL_BIRD;
	bird.starsCollected = 0;
	bird.speed = 1;
    bird.life = 3;

    StarSpawner spawner;
	InitSpawner(&spawner, &playArea);

    DrawBird(&bird);

    // 4. Main Loop
    bool running = true;
	char direction = ' '; // Placeholder for future direction handling
    while (running) {
        DrawStar(&star);
        // Input Handling (Non-blocking)
        if (_kbhit()) {
            char ch = _getch();
            switch(ch) {
                case QUIT:
                    running = false;
                    break;
                case FORWARD:
                    if (bird.dy < 0)
                    {
                        direction = 'F';
                    }
                    else
						bird.dy = -bird.dy;
					    direction = 'F';
                    break;
                case BACKWARD:
                    if (bird.dy > 0)
                    {
                        direction = 'S';
                    }
                    else
						bird.dy = -bird.dy;
					    direction = 'S';
                    break;
                case LEFT:
                    if (bird.dx < 0)
                    { 
                        direction = 'L';
                    }
                    else
                        bird.dx = -bird.dx;
					    direction = 'L';
                    break;
                case RIGHT:
                    if (bird.dx > 0)
                    {
                        direction = 'R';
                    }
                    else
                        bird.dx = -bird.dx;
                        direction = 'R';
                    break;
			}
        }

        // Logic Updates
        MoveBird(&bird, direction);
		updateStars(&spawner);
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