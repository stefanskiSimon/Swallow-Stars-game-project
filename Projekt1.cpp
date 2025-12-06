/*
    ===================================================================
    SWALLOW STARS - C++ Windows Console Version
    ===================================================================
*/
#define _CRT_SECURE_NO_DEPRECATE 
#include <iostream>
#include <conio.h>      // _getch(), _kbhit()
#include <windows.h>    // SetConsoleCursorPosition, Sleep, Colors
#include <cstdlib>     // rand()

using namespace std;

// -----------------------------
// Configuration Constants
// -----------------------------

#define QUIT        'q'
#define FORWARD     'w'
#define BACKWARD    's'
#define LEFT        'a'
#define RIGHT       'd'
#define SPEED_UP    'o'
#define SPEED_DOWN  'p'
#define FRAME_TIME  50     // Milliseconds

// Window offset
#define OFFY        2
#define OFFX        5

// Array / safety limits
#define MAX_SPRITE_HEIGHT 5
#define MAX_SPRITE_WIDTH 10
#define MAX_HUNTERS 10
#define MAX_STARS 20

// -----------------------------
// Console Helper Functions
// -----------------------------

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

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

struct GameStats {
    int timer;
    int borderColor;
    int StatusBorder;
    int MapWidth;
	int MapHeight;
    int StarSpawnRate;
	int MaxStars;
	int HuntersSpawnRate;
	int MaxHunters;
    DWORD LastTimerTick;
    int initialTimer;
    int baseHunterBounceCount;
    int baseHunterColor;       
    int currentHunterBounceCount;
    int currentHunterColor;
    int baseStarsRequired;
    int starsRequiredForLevelUp;
    int starGoalIncrease;
};

struct GameWindow {
    int x, y;       // Top-left screen position
    //int width, height;
    GameStats* stats;
    int width;
    int height;
    int color;

    void drawBorder() {
        setColor(color);

        gotoxy(x, y);
        cout << "+";
        for (int i = 0; i < width - 2; i++) cout << "-";
        cout << "+";

        for (int i = 1; i < height - 1; i++) {
            gotoxy(x, y + i); cout << "|";
            gotoxy(x + width - 1, y + i); cout << "|";
        }

        gotoxy(x, y + height - 1);
        cout << "+";
        for (int i = 0; i < width - 2; i++) cout << "-";
        cout << "+";
    }

    void printAt(int relX, int relY, const char* text) {
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
	SpriteCell cells[MAX_SPRITE_HEIGHT][MAX_SPRITE_WIDTH];
    int anchorX, anchorY;
};

struct Bird {
    GameWindow* parentWin;
    int x, y;
    int dx, dy;
    int starsCollected;
    int speed;
    int initialSpeed;
    int color;
    DWORD LastMoveTime;
    int life;
    Sprite sprite;
    Sprite spriteUp;
    Sprite spriteDown;
    Sprite spriteLeft;
    Sprite spriteRight;
};

struct Hunters {
    GameWindow* parentWin;
    int x, y;
    int dx, dy;
    Sprite sprite;
    bool active;
    int color;
    int speed;
	DWORD LastMoveTime;
    int BounceCount;
    int damage;
    int targetX, targetY;
};

struct HunterSpawner {
    GameWindow* parentWin;
	Hunters hunters[MAX_HUNTERS];
	int currentHunters;
    int spawnRate;
    int maxHunters;
    DWORD lastSpawnTime;
    DWORD nextSpawnTime;
};

struct Star {
    GameWindow* parentWin;
    int x, y;
    char symbol[32];
    int color;
    int endcolor;
    int newcolor;
    bool active;
    int speed;
    DWORD LastMoveTime;
};

struct StarSpawner {
    GameWindow* parentWin;
	Star stars[MAX_STARS];
	int currentStars;
    int spawnRate;
    int maxStars;
    DWORD lastSpawnTime;
    DWORD nextSpawnTime;
};

struct SpriteDesignLines {
    char* lines[MAX_SPRITE_HEIGHT];
    int count;
};

//------------------------------------------------
//------------  SPRITE FUNCTIONS ------------------
//------------------------------------------------

Sprite CreateSprite(int width, int height, int anchorX = 0, int anchorY = 0) {
    Sprite sprite;
    sprite.width = width;
    sprite.height = height;
    sprite.anchorX = anchorX;
    sprite.anchorY = anchorY;

    for(int i = 0; i < height && i < MAX_SPRITE_HEIGHT; i++) {
        for (int j = 0; j < width && j < MAX_SPRITE_WIDTH; j++) {
            SpriteCell cell;
            cell.character = ' ';
            cell.color = 7;
        }
	}

    return sprite;
}
Sprite CreateBirdSprite(SpriteDesignLines* lines, int default_color, int anchorX = 0, int anchorY = 0) 
{
	int height = lines->count;
    int width = 0;

    for(size_t i = 0; i < height; i++) {
		if (lines->lines[i] == nullptr) continue;
        size_t len = strlen(lines->lines[i]);
        if (len > (size_t)width) {
            width = (int)len;
        }
	}

	Sprite sprite = CreateSprite(width, height, anchorX, anchorY);

    for(int y = 0; y < height; y++) 
        if (lines->lines[y] == nullptr) continue;
        else {
            for (int x = 0; x < (int)strlen(lines->lines[y]); x++) {
                char ch = lines->lines[y][x];
                sprite.cells[y][x].character = ch;
                if (ch != ' ') {
                    sprite.cells[y][x].color = default_color;
                }
            }
        }

    return sprite;
}
Sprite CreateBirdSpriteRight(int color) {
    static char* up_design[] = {
        (char*)">=>"
    };
    SpriteDesignLines lines = { 0 };
    lines.count = sizeof(up_design) / sizeof(up_design[0]);

    for (int i = 0; i < lines.count; i++) {
        lines.lines[i] = up_design[i];
    }

    return CreateBirdSprite(&lines, color, 0, 0);
}
Sprite CreateBirdSpriteLeft(int color) {
    static char* up_design[] = {
        (char*)"<=<"
    };
    SpriteDesignLines lines = { 0 };
    lines.count = sizeof(up_design) / sizeof(up_design[0]);

    for (int i = 0; i < lines.count; i++) {
        lines.lines[i] = up_design[i];
    }

    return CreateBirdSprite(&lines, color, 0, 0);
}
Sprite CreateBirdSpriteUp(int color) {
    static char* up_design[] = {
        (char*)"^",
        (char*)"|",
		(char*)"^"
    };
    SpriteDesignLines lines = { 0 };
    lines.count = sizeof(up_design) / sizeof(up_design[0]);

    for (int i = 0; i < lines.count; i++) {
        lines.lines[i] = up_design[i];
    }

    return CreateBirdSprite(&lines, color, 0, 0);
}
Sprite CreateBirdSpriteDown(int color) {
    static char* up_design[] = {
        (char*)"v",
        (char*)"|",
        (char*)"v"
    };
    SpriteDesignLines lines = { 0 };
    lines.count = sizeof(up_design) / sizeof(up_design[0]);

    for (int i = 0; i < lines.count; i++) {
        lines.lines[i] = up_design[i];
    }

    return CreateBirdSprite(&lines, color, 0, 0);
}

//------------------------------------------------
//------------  LOADING CONFIGURATION  ------------
//------------------------------------------------

GameStats loadGameStats(const char* filename)
{
    GameStats stats;
    FILE* file = fopen(filename, "r");

    if (file == nullptr) {
        cout << "Error opening file: " << filename << endl;
    }

    char line[256];
    char currentSection[50] = "";

    while (fgets(line, sizeof(line), file) != nullptr) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        if (strchr(line, ':') == nullptr)
        {
            strcpy(currentSection, line);
        }
        else
        {
            char header[64];
            int value;
            if (sscanf(line, "%[^:]: %d", header, &value) == 2) {
                if (strcmp(currentSection, "WORLDSETTINGS") == 0)
                {
                    if (strcmp(header, "Timer") == 0) {
                        stats.timer = value;
                    }
                    else if (strcmp(header, "BorderColor") == 0) {
                        stats.borderColor = value;
                    }
                    else if (strcmp(header, "MapWidth") == 0) {
                        stats.MapWidth = value;
                    }
                    else if (strcmp(header, "MapHeight") == 0) {
                        stats.MapHeight = value;
                    }
                    else if (strcmp(header, "StarSpawnRate") == 0) {
                        stats.StarSpawnRate = value;
                    }
                    else if (strcmp(header, "MaxStars") == 0) {
                        stats.MaxStars = value;
                    }
                    else if (strcmp(header, "HuntersSpawnRate") == 0) {
                        stats.HuntersSpawnRate = value;
                    }
                    else if (strcmp(header, "MaxHunters") == 0) {
                        stats.MaxHunters = value;
                    }
                    else if (strcmp(header, "StatusBorder") == 0) {
                        stats.StatusBorder = value;
                    }
                    else if (strcmp(header, "MinStarsToCollect") == 0) {
                        stats.baseStarsRequired = value;
                    }
                    else if (strcmp(header, "StarGoalIncrease") == 0) {
                        stats.starGoalIncrease = value;
                    }
                }
                else {
                    char header2[64];
                    char value2[256];
                    if (sscanf(line, "%[^:]: %s", header2, value2) == 2) {
                        if (strcmp(currentSection, "HUNTERS") == 0)
                        {
                            if (strcmp(header2, "BounceCount") == 0) {
                                stats.baseHunterBounceCount = atoi(value2);
                            }
                            else if (strcmp(header2, "Color") == 0) {
                                stats.baseHunterColor = atoi(value2);
                            }
                        }
                    }
                }
            }
        }
    }
    fclose(file);
    stats.currentHunterBounceCount = stats.baseHunterBounceCount;
    stats.currentHunterColor = stats.baseHunterColor;
    stats.starsRequiredForLevelUp = stats.baseStarsRequired;

    stats.LastTimerTick = GetTime();
    stats.initialTimer = stats.timer;
    return stats;
}

Bird loadBirdStats(const char* filename, GameWindow* win) {
    Bird b;

    FILE* file = fopen(filename, "r");

    if (file == nullptr) {
        cout << "Error opening file: " << filename << endl;
    }

    char line[256];
	char currentSection[50] = "";

    while (fgets(line, sizeof(line), file) != nullptr) {

		line[strcspn(line, "\n")] = 0;

		if (strlen(line) == 0) continue;

        if (strchr(line, ':') == nullptr)
        {
            strcpy(currentSection, line);
        }
        else
        {

            char header[64];
            int value;

            if (sscanf(line, "%[^:]: %d", header, &value) == 2) {
                if (strcmp(currentSection, "BIRD") == 0)
                {
                    if (strcmp(header, "Health") == 0) {
                        b.life = value;
                    }
                    else if (strcmp(header, "Speed") == 0) {
                        b.speed = value;
                    }
                    else if (strcmp(header, "Color") == 0) {
                        b.color = value;
                    }
                }
                else
                    continue;
            }
        }
    }
    fclose(file);
    b.parentWin = win;
	b.initialSpeed = b.speed;
    b.x = win->stats->MapWidth / 2;
    b.y = win->stats->MapHeight / 2;
    b.dx = 1;
    b.dy = 1;
    b.starsCollected = 0;
    b.LastMoveTime = GetTime();
    b.spriteUp = CreateBirdSpriteUp(b.color);
    b.spriteDown = CreateBirdSpriteDown(b.color);
    b.spriteLeft = CreateBirdSpriteLeft(b.color);
    b.spriteRight = CreateBirdSpriteRight(b.color);
    b.sprite = b.spriteRight;  // Start facing right

    return b;
}

Hunters loadHuntersStats(const char* filename, GameWindow* win, int birdX, int birdY) {
    Hunters h;

    FILE* file = fopen(filename, "r");

    if (file == nullptr) {
        cout << "Error opening file: " << filename << endl;
    }

    char line[256];
    char currentSection[50] = "";
    SpriteDesignLines spriteDesign = { 0 };

    while (fgets(line, sizeof(line), file) != nullptr) {

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;

        if (strchr(line, ':') == nullptr)
        {
            strcpy(currentSection, line);
        }
        else
        {

            char header[64];
            char value[256];

            if (sscanf(line, "%[^:]: %s", header, value) == 2) {
                if (strcmp(currentSection, "HUNTERS") == 0)
                {
                    if (strcmp(header, "HunterSpeed") == 0) {
                        h.speed = atoi(value);
                    }
                    else if(strcmp(header, "Damage") == 0) {
                        h.damage = atoi(value);
				}
                    else if (strcmp(header, "Sprite") == 0)
                    {
                        if (spriteDesign.count < MAX_SPRITE_HEIGHT) 
                        {
                            size_t len = strlen(value) + 1;
                            char* new_line = (char*)malloc(len);

                            if (new_line != NULL) {
                                strcpy(new_line, value);
                                spriteDesign.lines[spriteDesign.count] = new_line; // Store the new line
                                spriteDesign.count++; // Increment only on success
                            }
                        }
                    }
                }
                else
                    continue;
            }
        }
    }
    fclose(file);
    h.BounceCount = win->stats->currentHunterBounceCount;
    h.color = win->stats->currentHunterColor;
    h.damage = h.damage * h.BounceCount;
    h.sprite = CreateBirdSprite(&spriteDesign, h.color, 0, 0);

    for (int i = 0; i < spriteDesign.count; i++) {
        if (spriteDesign.lines[i] != NULL) {
            free(spriteDesign.lines[i]); // Free the memory allocated for the config line
            spriteDesign.lines[i] = NULL;
        }
    }
    
	h.parentWin = win;
	h.active = true;
    h.targetX = birdX;
    h.targetY = birdY;

    int spriteWidth = h.sprite.width;
    int spriteHeight = h.sprite.height;

    int leftSpawnX = 1;
    int rightSpawnX = win->stats->MapWidth - 3 - spriteWidth;

    bool rightSide = false;
    bool leftSide = false;


    if (rand() % 2 == 0) {
        h.x = leftSpawnX;
        //leftSide = true;
    }
    else {
        h.x = rightSpawnX;
        //rightSide = true;
    }
    h.y = rand() % (win->stats->MapHeight - 2 - spriteHeight) + 1;
    
    if (h.targetX > h.x) h.dx = 1;
    else if (h.targetX < h.x) h.dx = -1;
    else h.dx = 0;

    if (h.targetY > h.y) h.dy = 1;
    else if (h.targetY < h.y) h.dy = -1;
    else h.dy = 0;

	h.LastMoveTime = GetTime();

    return h;
}

Star loadStarsStats(const char* filename, GameWindow* win)
{
    Star s;

    FILE* file = fopen(filename, "r");

    if (file == nullptr) {
        cout << "Error opening file: " << filename << endl;
    }

    char line[256];
    char currentSection[50] = "";

    while (fgets(line, sizeof(line), file) != nullptr) {

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;

        if (strchr(line, ':') == nullptr)
        {
            strcpy(currentSection, line);
        }
        else
        {

            char header[64];
            char value[64];

            if (sscanf(line, "%[^:]: %s", header, &value) == 2) {
                if (strcmp(currentSection, "STARS") == 0)
                {
                    if (strcmp(header, "StarSpeed") == 0) {
                        s.speed = atoi(value);
                    }
                    else if (strcmp(header, "StarSprite") == 0) {
                        strcpy(s.symbol, value);
                    }
                    else if (strcmp(header, "StarColor") == 0) {
                        s.color = atoi(value);
                    }
                    else if (strcmp(header, "EndColor") == 0) {
                        s.endcolor = atoi(value);
                    }
                }
                else
                    continue;
            }
        }
    }
    fclose(file);
    s.parentWin = win;
    s.active = true;
	s.newcolor = s.color;
    s.x = rand() % (win->stats->MapWidth - 2) + 1; // Avoid borders
    s.y = 1; // Start at top
    s.LastMoveTime = GetTime();

    return s;
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
            if (screenX > win->x && screenX < win->x + win->stats->MapWidth - 1 &&
                screenY > win->y && screenY < win->y + win->stats->MapHeight - 1) {

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

            // Only clear if within window bounds
            if (screenX > win->x && screenX < win->x + win->stats->MapWidth - 1 &&
                screenY > win->y && screenY < win->y + win->stats->MapHeight - 1) {

                if (sprite->cells[y][x].character != ' ') {
                    gotoxy(screenX, screenY);
                    cout << " ";
                }
            }
        }
    }
}

void ScaleHunters(GameStats* stats) {
    int timeElapsed = stats->initialTimer - stats->timer;
    int totalTime = stats->initialTimer;

    if (stats->timer <= totalTime / 2) {
        if (stats->currentHunterBounceCount == stats->baseHunterBounceCount) {
            stats->currentHunterBounceCount += 1;
            stats->currentHunterColor += 1;
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
    int maxX = b->parentWin->stats->MapWidth - 2;
    int minY = 1;
    int maxY = b->parentWin->stats->MapHeight - 2;

    // 3. Calculate new position
    int nextX = b->x + b->dx;
    int nextY = b->y + b->dy;

	// 4. Collision detection with borders
    if (nextX < minX) {
        nextX = minX;
        b->dx = -b->dx; // Reverse direction
		b->sprite = b->spriteRight;
    }
    else if (nextX > maxX) {
        nextX = maxX;
        b->dx = -b->dx; // Reverse direction
		b->sprite = b->spriteLeft;
	}
    if (nextY < minY) {
        nextY = minY;
        b->dy = -b->dy; // Reverse direction
		b->sprite = b->spriteDown;
    }
    else if (nextY > maxY) {
        nextY = maxY;
        b->dy = -b->dy; // Reverse direction
		b->sprite = b->spriteUp;
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

//------------------------------------------------
//------------  SPAWNER AND UPDATE FUNCTIONS -----------------------
//------------------------------------------------

void InitSpawner(StarSpawner* spawner, GameWindow* win) {
    spawner->parentWin = win;
    spawner->spawnRate = win->stats->StarSpawnRate;
    spawner->maxStars = win->stats->MaxStars;
	spawner->currentStars = 0;
    spawner->lastSpawnTime = GetTime();
	spawner->nextSpawnTime = spawner->spawnRate;
    if (spawner->currentStars < spawner->maxStars && spawner->currentStars < MAX_STARS) {
        spawner->stars[spawner->currentStars] = loadStarsStats("config.txt", win);
        spawner->currentStars++;
    }
}

void InitHunterSpawner(HunterSpawner* spawner, GameWindow* win, Bird* bird) {
    spawner->parentWin = win;
    spawner->spawnRate = win->stats->HuntersSpawnRate;
    spawner->maxHunters = win->stats->MaxHunters;
    spawner->currentHunters = 0;
    spawner->lastSpawnTime = GetTime();
    spawner->nextSpawnTime = spawner->spawnRate;
    if (spawner->currentHunters < spawner->maxHunters && spawner->currentHunters < MAX_HUNTERS) {
        spawner->hunters[spawner->currentHunters] = loadHuntersStats("config.txt", win, bird->x, bird->y);
        spawner->currentHunters++;
    }
}

int CountActiveStars(StarSpawner* spawner) {
    int count = 0;
    for (size_t i = 0; i < spawner->currentStars; i++) {
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
    for (size_t i = 0; i < spawner->currentStars; i++) {
        if (!spawner->stars[i].active) {
            // Reuse this slot
            spawner->stars[i] = loadStarsStats("config.txt",spawner->parentWin);
            foundSlot = true;
            break;
        }
    }
	spawner->lastSpawnTime = currentTime;
    // No inactive slot found, add new star
    if (!foundSlot && spawner->currentStars < MAX_STARS) {
        spawner->stars[spawner->currentStars] = loadStarsStats("config.txt", spawner->parentWin);
        spawner->currentStars++;
    }

}

void UpdateTimer(GameWindow* statwin) {
    DWORD currentTime = GetTime();
    if (currentTime - statwin->stats->LastTimerTick >= 1000) {
        statwin->stats->timer--;
        statwin->stats->LastTimerTick = currentTime;
    }
}

void UpdateStatus(GameWindow* statWin, Bird* b) {
    setColor(statWin->stats->StatusBorder);
    statWin->printAt(2, 1, "                                           ");
    char buffer[64];
	int requiredStars = statWin->stats->starsRequiredForLevelUp;
    sprintf_s(buffer, sizeof(buffer), "Stars: %d/%d life: %d Timer: %d [Q]=Quit", 
        b->starsCollected,requiredStars , b->life, statWin->stats->timer);
    statWin->printAt(2, 1, buffer);
}

// Star Logic

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
    int maxX = s->parentWin->stats->MapWidth - 2;
    s->x = rand() % (maxX - 1) + 1;
    s->y = 1;
    s->active = true;
    s->color = s->newcolor;
}

void MoveStar(Star* s) {
    if (!s->active) return;

	DWORD currentTime = GetTime();
	if (currentTime - s->LastMoveTime < s->speed) return;
	s->LastMoveTime = currentTime;

    ClearStar(s);
    int minX = 1;
    int maxX = s->parentWin->stats->MapWidth - 2;
    int minY = 1;
    int maxY = s->parentWin->stats->MapHeight - 2;

    int nextY = s->y + 1;
    s->y = nextY;

    if (s->y > s->parentWin->stats->MapHeight / 2)
    {
		s->color = s->endcolor;
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
    for (size_t i = 0; i < spawner->currentStars; i++) {
        MoveStar(&spawner->stars[i]);
    }
}

void DrawAllStars(StarSpawner* spawner) {
    for (size_t i = 0; i < spawner->currentStars; i++) {
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
    for (size_t i = 0; i < spawner->currentStars; i++) {
        if (CheckSpriteCollision(b, &spawner->stars[i])) {
            ClearStar(&spawner->stars[i]);
            b->starsCollected++;
            RespawnStar(&spawner->stars[i]);
        }
    }

    GameStats* stats = b->parentWin->stats;
    if (b->starsCollected >= stats->starsRequiredForLevelUp) {
        stats->timer = stats->initialTimer;
        stats->starsRequiredForLevelUp += stats->starGoalIncrease;
        b->starsCollected = 0;
        stats->currentHunterBounceCount++;
        stats->baseHunterBounceCount++;
        stats->currentHunterColor++;
    }
}

//Hunter Logic
void DrawHunter(Hunters* h) {

    char label[16];
    sprintf_s(label, "%d", h->BounceCount);
    int LabelWidth = (int)strlen(label);

    int screenSpriteLeft = h->parentWin->x + h->x - h->sprite.anchorX;
    int screenSpriteTop = h->parentWin->y + h->y - h->sprite.anchorY;
    int screenLabelX = screenSpriteLeft + (h->sprite.width - LabelWidth) / 2;
    int screenLabelY = screenSpriteTop - 1;

    if (screenLabelY > h->parentWin->y && screenLabelX >= h->parentWin->x &&
        screenLabelX + LabelWidth <= h->parentWin->x + h->parentWin->stats->MapWidth) {
        setColor(h->color);
        gotoxy(screenLabelX, screenLabelY);
        std::cout << label;

        DrawSprite(h->parentWin, &h->sprite, h->x, h->y);
    }
}

void ClearHunter(Hunters* h) {

    int screenSpriteLeft = h->parentWin->x + h->x - h->sprite.anchorX;
    int screenSpriteTop = h->parentWin->y + h->y - h->sprite.anchorY;
    int screenLabelX = screenSpriteLeft;
    int screenLabelY = screenSpriteTop - 1;

    if (screenLabelY > h->parentWin->y && screenLabelX >= h->parentWin->x) {
        gotoxy(screenLabelX, screenLabelY);
        for (int i = 0; i < h->sprite.width && (screenLabelX + 1) < (h->parentWin->x + h->parentWin->stats->MapWidth); i++)
        {
			cout << " ";
        }
    }

    ClearSprite(h->parentWin, &h->sprite, h->x, h->y);
}

void MoveHunters(Hunters* h) {
	if (!h->active) return;
	DWORD currentTime = GetTime();
	if (currentTime - h->LastMoveTime < h->speed) return;
	h->LastMoveTime = currentTime;

	ClearHunter(h);

    int minX = 1;
    int maxX = h->parentWin->stats->MapWidth - 1 - h->sprite.width;
    int minY = 1;
    int maxY = h->parentWin->stats->MapHeight - 1 - h->sprite.height;

	int nextX = h->x + h->dx;
	int nextY = h->y + h->dy;

    if (h->BounceCount <= 0) {
		ClearHunter(h);
        h->active = false;
		return;
    }

    if (nextX < minX || nextX > maxX) {
        h->dx = -h->dx;
        h->BounceCount--;
	}
    else if (nextY < minY || nextY > maxY) {
        h->dy = -h->dy;
        h->BounceCount--;
    }

    //Move hunter towards bird?
	h->x += h->dx;
	h->y += h->dy;
	DrawHunter(h);
}

void CheckHunterCollision(Bird* b, Hunters* h) {
    if (!h->active) return;
    Sprite* sprite = &b->sprite;
    for (int y = 0; y < sprite->height; y++) {
        for (int x = 0; x < sprite->width; x++) {
            if (sprite->cells[y][x].character != ' ') {
                // Calculate world position of this sprite cell
                int cellX = b->x + (x - sprite->anchorX);
                int cellY = b->y + (y - sprite->anchorY);
                // Check collision with hunter's bounding box
                if (cellX >= h->x && cellX < h->x + h->sprite.width &&
                    cellY >= h->y && cellY < h->y + h->sprite.height) {
                    // Collision detected
                    b->life = b->life - h->damage;
				h->BounceCount--;
				h->dx = -h->dx;
				h->dy = -h->dy;
                    return;
                }
            }
        }
    }
}

void CheckAllHunterCollisions(Bird* b, HunterSpawner* spawner) {
    for (size_t i = 0; i < spawner->currentHunters; i++) {
        CheckHunterCollision(b, &spawner->hunters[i]);
    }
}

void RespawnHunter(Hunters* h, GameWindow* win, const char* filename, int birdX, int birdY) {
	
    FILE* file = fopen(filename, "r");

    if (file == nullptr) {
        cout << "Error opening file: " << filename << endl;
    }

    char line[256];
    char currentSection[50] = "";
    SpriteDesignLines spriteDesign = { 0 };

    while (fgets(line, sizeof(line), file) != nullptr) {

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;

        if (strchr(line, ':') == nullptr)
        {
            strcpy(currentSection, line);
        }
        else
        {

            char header[64];
            char value[256];

            if (sscanf(line, "%[^:]: %s", header, value) == 2) {
                if (strcmp(currentSection, "HUNTERS") == 0)
                {
                    if (strcmp(header, "HunterSpeed") == 0) {
                        h->speed = atoi(value);
                    }
                    else if (strcmp(header, "Damage") == 0) {
                        h->damage = atoi(value);
                    }
                    else if (strcmp(header, "Sprite") == 0)
                    {
                        if (spriteDesign.count < MAX_SPRITE_HEIGHT)
                        {
                            size_t len = strlen(value) + 1;
                            char* new_line = (char*)malloc(len);

                            if (new_line != NULL) {
                                strcpy(new_line, value);
                                spriteDesign.lines[spriteDesign.count] = new_line; // Store the new line
                                spriteDesign.count++; // Increment only on success
                            }
                        }
                    }
                }
                else
                    continue;
            }
        }
    }
    fclose(file);

    h->BounceCount = win->stats->currentHunterBounceCount;
    h->color = win->stats->currentHunterColor;
    h->sprite = CreateBirdSprite(&spriteDesign, h->color, 0, 0);

    for (int i = 0; i < spriteDesign.count; i++) {
        if (spriteDesign.lines[i] != NULL) {
            free(spriteDesign.lines[i]);
            spriteDesign.lines[i] = NULL;
        }
    }

    h->parentWin = win;
    h->targetX = birdX;
    h->targetY = birdY;

    int spriteWidth = h->sprite.width;
    int spriteHeight = h->sprite.height;

    int leftSpawnX = 1;
    int rightSpawnX = win->stats->MapWidth - 3 - spriteWidth;
    bool rightSide = false;
    bool leftSide= false;

    if (rand() % 2 == 0) {
        h->x = leftSpawnX;
        leftSide = true;
    }
    else {
        h->x = rightSpawnX;
        rightSide = true;
    }
    h->y = rand() % (win->stats->MapHeight - 2 - spriteHeight) + 1;

    if (h->targetX > h->x) h->dx = 1;
    else if (h->targetX < h->x) h->dx = -1;
    else h->dx = 0;

    if (h->targetY > h->y) h->dy = 1;
    else if (h->targetY < h->y) h->dy = -1;
    else h->dy = 0;

    h->active = true;
    h->LastMoveTime = GetTime();
}

void UpdateHunters(HunterSpawner* spawner, Bird* bird) 
{
    DWORD currentTime = GetTime();

    // Count active hunters
    int activeCount = 0;
    for (size_t i = 0; i < spawner->currentHunters; ++i) {
        if (spawner->hunters[i].active) ++activeCount;
    }

    // If it's time to spawn and we have capacity, respawn an inactive slot or add a new hunter
    if ((int)activeCount < spawner->maxHunters && (currentTime - spawner->lastSpawnTime >= spawner->spawnRate)) {
        bool reused = false;
        for (size_t i = 0; i < spawner->currentHunters; ++i) {
            if (!spawner->hunters[i].active) {
                RespawnHunter(&spawner->hunters[i],spawner->parentWin, "config.txt", bird->x, bird->y);
                reused = true;
                break;
            }
        }
        if (!reused && spawner->currentHunters < MAX_STARS) {
            spawner->hunters[spawner->currentHunters] = loadHuntersStats("config.txt", spawner->parentWin, bird->x, bird->y);
            spawner->currentHunters++;
        }
        spawner->lastSpawnTime = currentTime;
    }

    // Move all hunters (active ones will be processed inside MoveHunters)
    for (size_t i = 0; i < spawner->currentHunters; i++) {
        MoveHunters(&spawner->hunters[i]);
    }
}

//------------------------------------------------
//------------  MAIN -----------------------------
//------------------------------------------------

int main() {
    hideCursor();
    system("cls");
    system("title Flying Bird Game");
    srand(time(0));

	GameStats gameStats = loadGameStats("config.txt");

    GameWindow playArea = { OFFX, OFFY, &gameStats, gameStats.MapWidth, gameStats.MapHeight, gameStats.borderColor};
    playArea.drawBorder();

    GameWindow statArea = { OFFX, OFFY + gameStats.MapHeight, &gameStats, gameStats.MapWidth, gameStats.MapHeight / 4, gameStats.StatusBorder};
    statArea.drawBorder();

	Bird bird = loadBirdStats("config.txt", &playArea);
	Hunters hunter = loadHuntersStats("config.txt", &playArea, bird.x, bird.y);

    StarSpawner Starspawner;
	InitSpawner(&Starspawner, &playArea);

    HunterSpawner hunterSpawner;
	InitHunterSpawner(&hunterSpawner, &playArea, &bird);
    
    DrawBird(&bird);

    bool running = true;
	char direction = ' ';
    while (running) {
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
                case SPEED_UP:
                    if (bird.speed <= bird.initialSpeed/2)
                        break;
					else
                        bird.speed -= bird.speed/10;
                    break;
				case SPEED_DOWN:
                    if(bird.speed >= 2*bird.initialSpeed)
                        break;
					else
                        bird.speed += bird.speed/10;
                    break;
			}
        }

        // Logic Updates
        MoveBird(&bird, direction);
		UpdateStars(&Starspawner);
		ScaleHunters(&gameStats);
		UpdateHunters(&hunterSpawner, &bird);
        CheckAllCollisions(&bird, &Starspawner);
		CheckAllHunterCollisions(&bird, &hunterSpawner);
		UpdateTimer(&statArea);
        UpdateStatus(&statArea, &bird);

        if(bird.life <= 0 || statArea.stats->timer <= 0) {
            running = false;
		}
        Sleep(FRAME_TIME);
    }

    system("cls");
    setColor(7);
    cout << "Game Over. Thanks for playing!" << endl;
    Sleep(1000);

    return 0;
}    