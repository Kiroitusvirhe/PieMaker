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

// Visibility tracking
bool grandmasVisible = false;
bool bakeriesVisible = false;
bool factoriesVisible = false;

// ========================
// ANNOUNCEMENT SYSTEM
// ========================
std::string announcement = "";  // Current announcement message
float announcementTimer = 0.0f; // How long to show the message
const float ANNOUNCEMENT_DURATION = 3.0f; // Show for 3 seconds

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
// PRESTIGE SYSTEM - UPDATED TO RESET VISIBILITY
// ========================
void resetForPrestige() {
    float starsGained = sqrt(totalPies / 1000.0f);
    prestigeStars += starsGained;
    totalPies = 0;
    piesPerSecond = 0;
    pendingPies = 0.0f;
    grandmas = bakeries = factories = 0;

    // Reset visibility on prestige if desired
    grandmasVisible = bakeriesVisible = factoriesVisible = false;
    for (auto& upgrade : upgrades) upgrade.visible = false;
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
// RENDERING - FIXED TO REDUCE FLICKER & KEEP ALIGNMENT
// ========================
void renderFrame(float deltaTime) {
    const int CONSOLE_WIDTH = 80;
    std::string frame;

    // Unlock buildings
    if (!grandmasVisible && totalPies >= 10) {
        grandmasVisible = true;
        announcement = "New! Grandmas - Bake 1 pie/sec (Cost: 10 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }
    if (!bakeriesVisible && totalPies >= 50) {
        bakeriesVisible = true;
        announcement = "New! Bakeries - Bake 5 pies/sec (Cost: 50 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }
    if (!factoriesVisible && totalPies >= 200) {
        factoriesVisible = true;
        announcement = "New! Factories - Bake 20 pies/sec (Cost: 200 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }

    for (auto& upgrade : upgrades) {
        if (!upgrade.visible && totalPies >= upgrade.cost / 2) {
            upgrade.visible = true;
            announcement = "Upgrade Available! " + upgrade.name + " (" + std::to_string(upgrade.cost) + " pies)";
            announcementTimer = ANNOUNCEMENT_DURATION;
        }
    }

    if (announcementTimer > 0) {
        announcementTimer -= deltaTime;
        if (announcementTimer <= 0) announcement = "";
    }

    // Helper lambda to pad lines to fixed width
    auto padLine = [&](const std::string& s) -> std::string {
        if (s.length() < CONSOLE_WIDTH)
            return s + std::string(CONSOLE_WIDTH - s.length(), ' ');
        else
            return s.substr(0, CONSOLE_WIDTH);
    };

    // Build frame line by line
    frame += padLine("=== PIE MAKER IDLE ===") + "\n";
    frame += padLine("Goal: Bake 1,000,000 pies!") + "\n";

    std::string piesLine = "Pies: " + std::to_string(totalPies);
    frame += padLine(piesLine) + "\n";

    std::string ppsLine = "Per second: " + std::to_string(piesPerSecond);
    frame += padLine(ppsLine) + "\n";

    std::string prestigeLine = "Prestige Stars: " + std::to_string((int)prestigeStars);
    frame += padLine(prestigeLine) + "\n";

    frame += "\n";

    frame += padLine("[SPACE] Bake a pie!") + "\n";
    frame += "\n";

    if (grandmasVisible) {
        std::string line = "[G] Grandmas: " + std::to_string(grandmas) + " (Cost: " + std::to_string(10 + grandmas * 2) + ")";
        frame += padLine(line) + "\n";
    }
    if (bakeriesVisible) {
        std::string line = "[B] Bakeries: " + std::to_string(bakeries) + " (Cost: " + std::to_string(50 + bakeries * 10) + ")";
        frame += padLine(line) + "\n";
    }
    if (factoriesVisible) {
        std::string line = "[F] Factories: " + std::to_string(factories) + " (Cost: " + std::to_string(200 + factories * 50) + ")";
        frame += padLine(line) + "\n";
    }

    frame += "\n";

    bool anyUpgrade = false;
    for (int i = 0; i < upgrades.size(); i++) {
        if (upgrades[i].visible && !upgrades[i].purchased) {
            anyUpgrade = true;
            std::string line = "[" + std::to_string(i + 1) + "] " + upgrades[i].name + " (" + std::to_string(upgrades[i].cost) + " pies)";
            frame += padLine(line) + "\n";
        }
    }

    if (anyUpgrade) frame += "\n";

    if (totalPies >= 1000) {
        std::string resetLine = "[R] RESET for " + std::to_string(sqrt(totalPies / 1000.0f)) + " prestige stars!";
        frame += padLine(resetLine) + "\n";
    }

    frame += "\n";

    // Pie art
    std::vector<std::string> pieToShow = showPressedPie ? piePressed :
        (idleFrame == 0 ? pieIdle1 : pieIdle2);

    for (const auto& line : pieToShow) {
        frame += padLine(line) + "\n";
    }

    // Announcement (show at bottom)
    if (!announcement.empty()) {
        frame += "\n" + padLine(announcement) + "\n";
    } else {
        // Print empty line to clear leftover announcement from previous frames
        frame += "\n" + std::string(CONSOLE_WIDTH, ' ') + "\n";
    }

    // Print frame to console
    setCursorPos(0, 0);
    std::cout << frame << std::flush;
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
    _getch();

    auto lastTime = std::chrono::steady_clock::now();

    while (!goalAchieved) {
        // Input
        if (keyPressed(VK_SPACE)) {
            totalPies++;
            showPressedPie = true;
            pieAnimTimer = 5;
        }

        if (grandmasVisible && keyPressed('G')) buyGrandma();
        if (bakeriesVisible && keyPressed('B')) buyBakery();
        if (factoriesVisible && keyPressed('F')) buyFactory();

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
        renderFrame(deltaTime);
        Sleep(10);
    }

    return 0;
}