#include <iostream>
#include <windows.h>
#include <cmath>
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <conio.h>

// ========================
// GAME STATE VARIABLES
// ========================
int totalPies = 0;
int piesPerSecond = 0;
int grandmas = 0, bakeries = 0, factories = 0;
float prestigeStars = 0;
float pendingPies = 0.0f;

// Game progression flags
bool showGrandmas = false;
bool showBakeries = false;
bool showFactories = false;
bool goalAchieved = false;

// ========================
// UPGRADE SYSTEM
// ========================
struct Upgrade {
    std::string name;
    int cost;
    bool purchased;
    bool visible = false;
    std::function<void()> effect;
};

std::vector<Upgrade> upgrades = {
    {"Better Ovens", 500, false, false, [] { piesPerSecond += grandmas; }},
    {"Industrial Wheat", 1000, false, false, [] { piesPerSecond += bakeries * 2; }},
    {"Robot Bakers", 5000, false, false, [] { piesPerSecond += factories * 3; }}
};

// ========================
// ASCII ART
// ========================
std::vector<std::string> pieIdle1 = {
    "   ~     ~    ",
    "  ~   ~   ~   ",
    "   .-''''''-. ",
    "  /          \\",
    " |  ~~~ ~~ ~~ |",
    "  \\__________/ "
};

std::vector<std::string> pieIdle2 = {
    "     ~   ~    ",
    "  ~     ~   ~ ",
    "   .-''''''-. ",
    "  /          \\",
    " |  ~~~ ~~ ~~ |",
    "  \\__________/ "
};

std::vector<std::string> piePressed = {
    "   \\  ^  ^  / ",
    "    \\ ^  ^ /  ",
    "   .-''''''-. ",
    "  /   O  O   \\",
    " |    ===     |",
    "  \\__________/ "
};

std::vector<std::string> celebrationPie = {
    "    .-~~~~~-.    ",
    "   /         \\   ",
    "  |   *   *   |  ",
    "  |  =======  |  ",
    "   \\ ~~~~~~~ /   ",
    "    `-------`    ",
    "     \\     /     ",
    "      \\   /      ",
    "       \\ /       ",
    "        V        "
};

std::vector<std::string> fireworks = {
    "   *  .  *  .  *   ",
    " .    *    *    .  ",
    "   .  *  *  *      ",
    " *   .     *   .   ",
    "      *  .  *      "
};

bool showPressedPie = false;
int pieAnimTimer = 0;
int idleFrame = 0;
float idleTimer = 0.0f;

// ========================
// CONSOLE HELPERS
// ========================
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void setCursorPos(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(hConsole, coord);
}

// ========================
// INPUT HANDLING
// ========================
bool keyPressed(int key) {
    static bool keyStates[256] = { false };
    bool currentState = (GetAsyncKeyState(key) & 0x8000);

    if (currentState && !keyStates[key]) {
        keyStates[key] = true;
        return true;
    } else if (!currentState) {
        keyStates[key] = false;
    }
    return false;
}

// ========================
// BUILDING FUNCTIONS
// ========================
void buyGrandma() {
    int cost = 10 + (grandmas * 2);
    if (totalPies >= cost) {
        totalPies -= cost;
        grandmas++;
        piesPerSecond += 1;
        if (upgrades[0].purchased) piesPerSecond += 1;
    }
}

void buyBakery() {
    int cost = 50 + (bakeries * 10);
    if (totalPies >= cost) {
        totalPies -= cost;
        bakeries++;
        piesPerSecond += 5;
        if (upgrades[1].purchased) piesPerSecond += 10;
    }
}

void buyFactory() {
    int cost = 200 + (factories * 50);
    if (totalPies >= cost) {
        totalPies -= cost;
        factories++;
        piesPerSecond += 20;
        if (upgrades[2].purchased) piesPerSecond += 60;
    }
}

// ========================
// PRESTIGE SYSTEM
// ========================
void resetForPrestige() {
    float starsGained = sqrt(totalPies / 1000.0f);
    prestigeStars += starsGained;
    totalPies = 0;
    piesPerSecond = 0;
    pendingPies = 0.0f;
    grandmas = bakeries = factories = 0;
    showGrandmas = showBakeries = showFactories = false;
}

// ========================
// CELEBRATION SCREEN
// ========================
void showCelebration() {
    system("cls");
    SetConsoleTextAttribute(hConsole, 14); // Yellow text

    std::cout << "\n\n";
    std::cout << "   CONGRATULATIONS!   \n";
    std::cout << " You baked 1,000,000 pies! \n\n";

    // Display celebration pie
    for (const auto& line : celebrationPie) {
        std::cout << line << "\n";
    }

    // Display fireworks
    SetConsoleTextAttribute(hConsole, 12); // Red
    std::cout << "\n";
    for (const auto& line : fireworks) {
        std::cout << line << "\n";
    }

    SetConsoleTextAttribute(hConsole, 10); // Green
    std::cout << "\n Press any key to continue...";
    _getch();
    goalAchieved = true;
}

// ========================
// RENDERING
// ========================
const int SCREEN_HEIGHT = 30;
std::vector<std::string> screenBuffer(SCREEN_HEIGHT);

void buildFrame() {
    for (auto& line : screenBuffer) line.clear();

    // Check for unlocks
    if (!showGrandmas && totalPies >= 10) {
        showGrandmas = true;
        screenBuffer[18] = "A million sounds like a lot... here's a Grandma to help!";
    }
    if (!showBakeries && totalPies >= 100) {
        showBakeries = true;
        screenBuffer[19] = "Bakeries unlocked! These can produce more pies!";
    }
    if (!showFactories && totalPies >= 500) {
        showFactories = true;
        screenBuffer[20] = "Factories now available - mass production time!";
    }

    // Main UI
    screenBuffer[0] = "=== PIE MAKER IDLE ===";
    screenBuffer[1] = "Goal: Bake 1,000,000 pies!";
    screenBuffer[2] = "Pies: " + std::to_string(totalPies);
    screenBuffer[3] = "Per second: " + std::to_string(piesPerSecond);
    screenBuffer[4] = "Prestige Stars: " + std::to_string(prestigeStars);
    screenBuffer[6] = "[SPACE] Bake a pie!";

    // Buildings (only show if unlocked)
    if (showGrandmas) {
        screenBuffer[8] = "[G] Grandmas: " + std::to_string(grandmas) +
                         " (Cost: " + std::to_string(10 + grandmas * 2) + ")";
    }
    if (showBakeries) {
        screenBuffer[9] = "[B] Bakeries: " + std::to_string(bakeries) +
                         " (Cost: " + std::to_string(50 + bakeries * 10) + ")";
    }
    if (showFactories) {
        screenBuffer[10] = "[F] Factories: " + std::to_string(factories) +
                          " (Cost: " + std::to_string(200 + factories * 50) + ")";
    }

    // Upgrades (stay visible once shown)
    for (int i = 0; i < upgrades.size(); i++) {
        if (!upgrades[i].purchased) {
            if (totalPies >= upgrades[i].cost / 2) {
                upgrades[i].visible = true;
            }
            if (upgrades[i].visible) {
                screenBuffer[12 + i] = "[" + std::to_string(i + 1) + "] " + upgrades[i].name +
                                      " (" + std::to_string(upgrades[i].cost) + " pies)";
            }
        }
    }

    // Prestige
    if (totalPies >= 1000) {
        screenBuffer[16] = "[R] RESET for " + std::to_string(sqrt(totalPies / 1000.0f)) + " prestige stars!";
    }

    // Pie display
    std::vector<std::string> pieToShow = showPressedPie ? piePressed :
                                       (idleFrame == 0 ? pieIdle1 : pieIdle2);
    for (int i = 0; i < pieToShow.size(); i++) {
        if (22 + i < screenBuffer.size()) {
            screenBuffer[22 + i] = pieToShow[i];
        }
    }
}

void renderFrame() {
    setCursorPos(0, 0); // Move to top-left without clearing
    for (const auto& line : screenBuffer) {
        std::cout << line << std::string(80 - line.length(), ' ') << "\n"; // Pad with spaces
    }
}

// ========================
// LOCK CONSOLE SIZE
// ========================
void lockConsoleSize(int width, int height) {
    SMALL_RECT windowSize = {0, 0, (short)(width - 1), (short)(height - 1)};
    COORD bufferSize = { (short)width, (short)height };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
}

// ========================
// MAIN GAME LOOP
// ========================
int main() {
    hideCursor();
    lockConsoleSize(80, SCREEN_HEIGHT); // Optional, but recommended for stable UI
    system("cls"); // One-time clear

    // Initial message
    std::cout << "=== PIE MAKER IDLE ===\n\n";
    std::cout << "Your goal: Bake ONE MILLION PIES!\n";
    std::cout << "Start by pressing SPACE to bake your first pie.\n\n";
    std::cout << "Press any key to begin...";
    _getch();

    auto lastTime = std::chrono::steady_clock::now();

    while (!goalAchieved) {
        // Input
        if (keyPressed(VK_SPACE)) {
            totalPies++;
            showPressedPie = true;
            pieAnimTimer = 5;
        }

        if (showGrandmas && keyPressed('G')) buyGrandma();
        if (showBakeries && keyPressed('B')) buyBakery();
        if (showFactories && keyPressed('F')) buyFactory();

        // Upgrades
        for (int i = 0; i < upgrades.size(); i++) {
            if (upgrades[i].visible && !upgrades[i].purchased &&
                keyPressed('1' + i) && totalPies >= upgrades[i].cost) {
                totalPies -= upgrades[i].cost;
                upgrades[i].purchased = true;
                upgrades[i].effect();
            }
        }

        // Prestige
        if (totalPies >= 1000 && keyPressed('R')) {
            resetForPrestige();
        }

        // Check for goal
        if (totalPies >= 1000000) {
            showCelebration();
            break;
        }

        // Timing
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        pendingPies += piesPerSecond * deltaTime;
        if (pendingPies >= 1.0f) {
            int piesToAdd = static_cast<int>(pendingPies);
            totalPies += piesToAdd;
            pendingPies -= piesToAdd;
        }

        // Animation
        if (pieAnimTimer > 0) {
            pieAnimTimer--;
            if (pieAnimTimer == 0) showPressedPie = false;
        }

        idleTimer += deltaTime;
        if (idleTimer >= 0.5f) {
            idleFrame = 1 - idleFrame;
            idleTimer = 0.0f;
        }

        // Render
        buildFrame();
        renderFrame();
        Sleep(10);
    }

    return 0;
}