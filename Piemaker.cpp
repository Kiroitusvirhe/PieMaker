#include <iostream>
#include <windows.h>
#include <cmath>
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <conio.h>
#include <sstream>
#include <locale>

// ========================
// GAME STATE VARIABLES
// ========================
int totalPies = 0;
int piesPerSecond = 0;
int grandmas = 0, bakeries = 0, factories = 0;
float prestigeStars = 0;
float pendingPies = 0.0f;
bool goalAchieved = false;
bool prestigeUnlocked = false; // <-- Add this line

// Visibility tracking
bool grandmasVisible = false;
bool bakeriesVisible = false;
bool factoriesVisible = false;

// ========================
// ANNOUNCEMENT SYSTEM
// ========================
std::string announcement = "";  // Current announcement message
float announcementTimer = 0.0f; // How long to show the message
const float ANNOUNCEMENT_DURATION = 6.0f; // Show for 5 seconds

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

std::vector<Upgrade> upgrades;

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

std::vector<std::vector<std::string>> fireworksFrames = {
    {
        "   *  .  *  .  *   ",
        " .    *    *    .  ",
        "   .  *  *  *      ",
        " *   .     *   .   ",
        "      *  .  *      "
    },
    {
        "      .  *  .      ",
        " *   .     *   .   ",
        "   *  .  *  .  *   ",
        " .    *    *    .  ",
        "   .  *  *  *      "
    },
    {
        " .    *    *    .  ",
        "   .  *  *  *      ",
        "      *  .  *      ",
        "   *  .  *  .  *   ",
        " *   .     *   .   "
    }
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

void showUnlockMessage(const std::string& message) {
    system("cls");
    setCursorPos(0, 0);
    std::cout << "=== UNLOCKED ===\n\n";
    std::cout << message << "\n\n";
    std::cout << "Press any key to continue...";
    _getch();
    system("cls");
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
        // No clear screen or message here
    }
}

void buyBakery() {
    int cost = 50 + (bakeries * 10);
    if (totalPies >= cost) {
        totalPies -= cost;
        bakeries++;
        piesPerSecond += 5;
        if (upgrades[1].purchased) piesPerSecond += 10;
        // No clear screen or message here
    }
}

void buyFactory() {
    int cost = 200 + (factories * 50);
    if (totalPies >= cost) {
        totalPies -= cost;
        factories++;
        piesPerSecond += 20;
        if (upgrades[2].purchased) piesPerSecond += 60;
        // No clear screen or message here
    }
}

// ========================
// PRESTIGE RESET
// ========================
void resetGameState() {
    totalPies = 0;
    piesPerSecond = 0;
    pendingPies = 0.0f;
    grandmas = bakeries = factories = 0;
    grandmasVisible = bakeriesVisible = factoriesVisible = false;
    upgrades = {
        {"Better Ovens", 500, false, false, [] { piesPerSecond += grandmas; }},
        {"Industrial Wheat", 1000, false, false, [] { piesPerSecond += bakeries * 2; }},
        {"Robot Bakers", 5000, false, false, [] { piesPerSecond += factories * 3; }}
    };
}

// ========================
// CELEBRATION + PROMPT
// ========================
void showCelebration() {
    system("cls");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    int frameIdx = 0;
    char response = 0;

    // Calculate the width of the fireworks + pie line for centering
    const auto& fw = fireworksFrames[0];
    int fireworksWidth = fw[0].size();
    int pieWidth = celebrationPie[0].size();
    int totalWidth = fireworksWidth + 3 + pieWidth + 3 + fireworksWidth; // 3 spaces between each

    std::string congrats = "CONGRATULATIONS!";
    std::string baked = "You baked 1,000,000 pies!";

    while (true) {
        setCursorPos(0, 0); // Smooth animation: overwrite instead of clearing

        // Get current fireworks frame
        const auto& fw = fireworksFrames[frameIdx % fireworksFrames.size()];

        // Print empty lines above for vertical centering
        std::cout << "\n\n";

        // Center and print the congratulations lines
        int leftPad = (totalWidth - (int)congrats.size()) / 2;
        std::cout << std::string(leftPad, ' ') << congrats << "\n";
        leftPad = (totalWidth - (int)baked.size()) / 2;
        std::cout << std::string(leftPad, ' ') << baked << "\n\n";

        // Print fireworks + pie side by side with fixed colors
        for (size_t i = 0; i < celebrationPie.size(); ++i) {
            int color;
            switch (i % 5) {
                case 0: color = 12; break; // Red
                case 1: color = 14; break; // Yellow
                case 2: color = 10; break; // Green
                case 3: color = 11; break; // Cyan
                case 4: color = 13; break; // Magenta
            }
            SetConsoleTextAttribute(hConsole, color);
            std::cout << fw[i % fw.size()];
            SetConsoleTextAttribute(hConsole, 7); // White for pie
            std::cout << "   " << celebrationPie[i] << "   ";
            SetConsoleTextAttribute(hConsole, color);
            std::cout << fw[i % fw.size()] << "\n";
        }
        SetConsoleTextAttribute(hConsole, 7); // Reset to default

        std::cout << "\nWould you like to play again? (Y/N): ";

        // Check for key press without blocking
        if (_kbhit()) {
            response = _getch();
            if (response == 'Y' || response == 'y') {
                resetGameState();
                goalAchieved = false;
                break;
            }
            if (response == 'N' || response == 'n') {
                goalAchieved = true;
                exit(0);
            }
        }

        Sleep(100);
        frameIdx++;
    }
}

// ========================
// HELPER FUNCTIONS
// ========================
std::string formatWithCommas(int value) {
    std::string num = std::to_string(value);
    int insertPosition = num.length() - 3;
    while (insertPosition > 0) {
        num.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return num;
}

// ========================
// RENDER FRAME
// ========================
void renderFrame(float deltaTime) {
    const int CONSOLE_WIDTH = 80;
    std::string frame;

    // Unlock logic + announcement
    if (!grandmasVisible && totalPies >= 10) {
        grandmasVisible = true;
        // Clear screen and show unlock message
        system("cls");
        setCursorPos(0, 0);
        std::cout << "=== UNLOCKED ===\n\n";
        std::cout << "New Building: Grandmas\n\nBake 1 pie per second\nCost: 10 pies\n\n";
        std::cout << "Press any key to continue...";
        _getch();
        system("cls");
        announcement = "Grandmas unlocked! Press G to buy (Cost: 10 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }
    if (!bakeriesVisible && totalPies >= 50) {
        bakeriesVisible = true;
        system("cls");
        setCursorPos(0, 0);
        std::cout << "=== UNLOCKED ===\n\n";
        std::cout << "New Building: Bakeries\n\nBake 5 pies per second\nCost: 50 pies\n\n";
        std::cout << "Press any key to continue...";
        _getch();
        system("cls");
        announcement = "Bakeries unlocked! Press B to buy (Cost: 50 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }
    if (!factoriesVisible && totalPies >= 200) {
        factoriesVisible = true;
        system("cls");
        setCursorPos(0, 0);
        std::cout << "=== UNLOCKED ===\n\n";
        std::cout << "New Building: Factories\n\nBake 20 pies per second\nCost: 200 pies\n\n";
        std::cout << "Press any key to continue...";
        _getch();
        system("cls");
        announcement = "Factories unlocked! Press F to buy (Cost: 200 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }

    for (auto& upgrade : upgrades) {
        if (!upgrade.visible && totalPies >= upgrade.cost / 2) {
            upgrade.visible = true;
            // Clear screen and show unlock message for upgrade
            system("cls");
            setCursorPos(0, 0);
            std::cout << "=== UNLOCKED ===\n\n";
            std::cout << "New Upgrade Available: " << upgrade.name << "\n\n";
            std::cout << "Cost: " << upgrade.cost << " pies\n";
            std::cout << "Press " << (&upgrade - &upgrades[0] + 1) << " to buy\n\n";
            std::cout << "Press any key to continue...";
            _getch();
            system("cls");
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
    frame += padLine("Pies: " + formatWithCommas(totalPies)) + "\n";
    frame += padLine("Per second: " + std::to_string(piesPerSecond)) + "\n";
    frame += padLine("Prestige Stars: " + std::to_string((int)prestigeStars)) + "\n\n";

    frame += padLine("[SPACE] Bake a pie!") + "\n\n";

    if (grandmasVisible) frame += padLine("[G] Grandmas: " + std::to_string(grandmas) + " (Cost: " + std::to_string(10 + grandmas * 2) + ")") + "\n";
    if (bakeriesVisible) frame += padLine("[B] Bakeries: " + std::to_string(bakeries) + " (Cost: " + std::to_string(50 + bakeries * 10) + ")") + "\n";
    if (factoriesVisible) frame += padLine("[F] Factories: " + std::to_string(factories) + " (Cost: " + std::to_string(200 + factories * 50) + ")") + "\n";

    frame += "\n";

    for (int i = 0; i < upgrades.size(); ++i) {
        if (upgrades[i].visible && !upgrades[i].purchased) {
            frame += padLine("[" + std::to_string(i + 1) + "] " + upgrades[i].name + " (" + std::to_string(upgrades[i].cost) + " pies)") + "\n";
        }
    }

    // Always show prestige reset line once unlocked
    if (prestigeUnlocked) {
        int piesForDisplay = std::max(totalPies, 1000);
        frame += "\n" + padLine("[R] RESET for " + std::to_string(sqrt(piesForDisplay / 1000.0f)) + " prestige stars!") + "\n";
    }

    frame += "\n";

    std::vector<std::string> pieToShow = showPressedPie ? piePressed : (idleFrame == 0 ? pieIdle1 : pieIdle2);
    for (const auto& line : pieToShow) frame += padLine(line) + "\n";

    if (!announcement.empty()) {
        frame += "\n" + padLine(announcement) + "\n";
    } else {
        frame += "\n" + std::string(CONSOLE_WIDTH, ' ') + "\n";
    }

    setCursorPos(0, 0);
    std::cout << frame << std::flush;
}

// ========================
// MAIN GAME LOOP
// ========================
int main() {
    hideCursor();

    do {
        resetGameState();
        system("cls");
        std::cout << "=== PIE MAKER IDLE ===\n\n";
        std::cout << "Your goal: Bake ONE MILLION PIES!\n";
        std::cout << "Start by pressing SPACE to bake your first pie.\n\n";
        _getch();

        auto lastTime = std::chrono::steady_clock::now();

        while (!goalAchieved && totalPies < 1000000) {
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
                    showUnlockMessage("Upgrade Purchased: " + upgrades[i].name + 
                                     "\n\nEffect applied!");
                }
            }

            // --- SECRET BUTTON: Press 'X' to win instantly ---
            if (keyPressed('X')) {
                totalPies = 1000000;
            }

            // Unlock prestige permanently once reached
            if (!prestigeUnlocked && totalPies >= 1000) {
                prestigeUnlocked = true;
            }

            // Prestige
            if (totalPies >= 1000 && keyPressed('R')) {
                prestigeStars += sqrt(totalPies / 1000.0f);
                resetGameState();
                system("cls");
                setCursorPos(0, 0);
                std::cout << "=== PRESTIGE RESET ===\n\n";
                std::cout << "You gained prestige stars! (" << (int)prestigeStars << " total)\n\n";
                std::cout << "Press any key to continue...";
                _getch();
                system("cls");
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

        if (totalPies >= 1000000) {
            showCelebration();
        }

    } while (!goalAchieved);

    return 0;
}