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
bool goalAchieved = false;

// ========================
// UPGRADE SYSTEM
// ========================
struct Upgrade {
    std::string name;
    int cost;
    bool purchased;
    std::function<void()> effect;
};

std::vector<Upgrade> upgrades = {
    {"Better Ovens", 500, false, [] { piesPerSecond += grandmas; }},
    {"Industrial Wheat", 1000, false, [] { piesPerSecond += bakeries * 2; }},
    {"Robot Bakers", 5000, false, [] { piesPerSecond += factories * 3; }}
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
void hideCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void setCursorPos(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
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
}

// ========================
// CELEBRATION SCREEN
// ========================
void showCelebration() {
    system("cls");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14); // Yellow text

    std::cout << "\n\n";
    std::cout << "   CONGRATULATIONS!   \n";
    std::cout << " You baked 1,000,000 pies! \n\n";

    for (const auto& line : celebrationPie) std::cout << line << "\n";

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12); // Red
    std::cout << "\n";
    for (const auto& line : fireworks) std::cout << line << "\n";

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10); // Green
    std::cout << "\n Press any key to continue...";
    _getch();
    goalAchieved = true;
}

// ========================
// RENDERING
// ========================
void renderFrame() {
    setCursorPos(0, 0); // Move cursor to top-left
    std::cout << "=== PIE MAKER IDLE ===" << std::string(57, ' ') << "\n";
    std::cout << "Goal: Bake 1,000,000 pies!" << std::string(57, ' ') << "\n";
    std::cout << "Pies: " << totalPies << std::string(72 - std::to_string(totalPies).length(), ' ') << "\n";
    std::cout << "Per second: " << piesPerSecond << std::string(68 - std::to_string(piesPerSecond).length(), ' ') << "\n";
    std::cout << "Prestige Stars: " << prestigeStars << std::string(65 - std::to_string((int)prestigeStars).length(), ' ') << "\n";
    std::cout << "[SPACE] Bake a pie!" << std::string(62, ' ') << "\n";
    std::cout << "[G] Grandmas: " << grandmas << " (Cost: " << (10 + grandmas * 2) << ")" << std::string(80, ' ').substr(0, 80 - (18 + std::to_string(grandmas).length() + 8 + std::to_string(10 + grandmas * 2).length())) << "\n";
    std::cout << "[B] Bakeries: " << bakeries << " (Cost: " << (50 + bakeries * 10) << ")" << std::string(80, ' ').substr(0, 80 - (18 + std::to_string(bakeries).length() + 8 + std::to_string(50 + bakeries * 10).length())) << "\n";
    std::cout << "[F] Factories: " << factories << " (Cost: " << (200 + factories * 50) << ")" << std::string(80, ' ').substr(0, 80 - (19 + std::to_string(factories).length() + 9 + std::to_string(200 + factories * 50).length())) << "\n";
    for (int i = 0; i < upgrades.size(); i++) {
        if (!upgrades[i].purchased) {
            std::string line = "[" + std::to_string(i + 1) + "] " + upgrades[i].name + " (" + std::to_string(upgrades[i].cost) + " pies)";
            std::cout << line << std::string(80 - line.length(), ' ') << "\n";
        }
    }
    if (totalPies >= 1000) {
        std::string line = "[R] RESET for " + std::to_string(sqrt(totalPies / 1000.0f)) + " prestige stars!";
        std::cout << line << std::string(80 - line.length(), ' ') << "\n";
    }
    // Pie art
    std::vector<std::string> pieToShow = showPressedPie ? piePressed :
                                       (idleFrame == 0 ? pieIdle1 : pieIdle2);
    for (const auto& line : pieToShow)
        std::cout << line << std::string(80 - line.length(), ' ') << "\n";
}

// ========================
// MAIN GAME LOOP
// ========================
int main() {
    hideCursor();
    system("cls");
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

        if (keyPressed('G')) buyGrandma();
        if (keyPressed('B')) buyBakery();
        if (keyPressed('F')) buyFactory();

        // Upgrades
        for (int i = 0; i < upgrades.size(); i++) {
            if (!upgrades[i].purchased &&
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
        renderFrame();
        Sleep(10);
    }

    return 0;
}