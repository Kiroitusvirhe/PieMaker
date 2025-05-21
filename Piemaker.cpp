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
#include <cstdlib>
#include <ctime>

// ========================
// GAME STATE VARIABLES
// ========================
int totalPies = 0;
int piesPerSecond = 0;
int grandmas = 0, bakeries = 0, factories = 0;
float prestigeStars = 0;
float pendingPies = 0.0f;
bool goalAchieved = false;
bool prestigeUnlocked = false;
int piesBakedThisRun = 0;
bool prestigeHintShown = false; // <-- Add this line here
bool forceClearScreen = false;  // <-- Add this line

// ========================
// INTRO SEQUENCE VARIABLES
// ========================
bool inIntro = true;
int introPies = 1;
int introSpacePresses = 0;
int introAnnouncementStep = 0;
std::string introAnnouncement = "";
float introAnnouncementTimer = 0.0f;
const float INTRO_ANNOUNCEMENT_DURATION = 4.0f;
bool introUnlockAvailable = false;
bool introBuildingsUnlocked = false;
bool introClearedAfterFirstSpace = false; // Add this line


// ========================
// PRESTIGE SHOP VARIABLES
// ========================
struct PrestigeUpgrade {
    std::string name;
    std::function<int()> getCost;
    std::function<void()> effect;
    std::function<bool()> isVisible;
    std::function<std::string()> getDescription;
};

int boostPercent = 0;
int milkPurchased = 0;
int catnipLevel = 0;
bool hasGoldenSword = false;
bool inPrestigeShop = false;
std::vector<PrestigeUpgrade> prestigeUpgrades;

// ========================
// CAT SYSTEM VARIABLES
// ========================
int totalCats = 0;

// ========================
// RAT SYSTEM VARIABLES
// ========================
int totalRats = 0;
int ratsEating = 0;
float ratsEatingSingle = 0.0f;
const int RAT_THRESHOLD = 50000;
const float RAT_EAT_RATE = 1.0f;
const int RAT_MAX = 999999;

// Visibility tracking
bool grandmasVisible = false;
bool bakeriesVisible = false;
bool factoriesVisible = false;

// ========================
// ANNOUNCEMENT SYSTEM
// ========================
std::string announcement = "";
float announcementTimer = 0.0f;
const float ANNOUNCEMENT_DURATION = 6.0f;
std::string lastAnnouncement = ""; // <-- Move this here (global scope)

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
std::vector<std::string> pieSteam1 = {
    "             (",
    "              )"
};
std::vector<std::string> pieSteam2 = {
    "            ~(",
    "           ~ )"
};

std::vector<std::string> pieIdle1 = {
    "         __..---..__",
    "     ,-='  /  |  \\  `=-.",
    "    :--..___________..--;",
    "     \\.,_____________,./"
};
std::vector<std::string> pieIdle2 = pieIdle1;

std::vector<std::string> piePressed = {
    "         __..---..__",
    "     ,-='  /  |  \\  `=-.",
    "    :--..___________..--;",
    "     \\.,_O_O_O_O_O___,./"
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

std::vector<std::string> ratArt = {
    "                   .-------.",
    "              (\\./)     \\.......-",
    "              >' '<  (__.'\"\"\"\"BP",
    "              \" ` \" \""
};

std::vector<std::string> catArt = {
    "      |\\_/|      ",
    "      ( o.o )    ",
    "       > ^ <     "
};

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
// BUILDING FUNCTIONS (WITH BOOST)
// ========================
void buyGrandma() {
    int cost = 10 + (grandmas * 2);
    if (totalPies >= cost) {
        totalPies -= cost;
        grandmas++;
        piesPerSecond += 1 + (1 * boostPercent / 100);
        if (upgrades[0].purchased) piesPerSecond += 1 + (1 * boostPercent / 100);
    }
}

void buyBakery() {
    int cost = 50 + (bakeries * 10);
    if (totalPies >= cost) {
        totalPies -= cost;
        bakeries++;
        piesPerSecond += 5 + (5 * boostPercent / 100);
       
    }
}

void buyFactory() {
    int cost = 200 + (factories * 50);
    if (totalPies >= cost) {
        totalPies -= cost;
        factories++;
        piesPerSecond += 20 + (20 * boostPercent / 100);
        if (upgrades[2].purchased) piesPerSecond += 60 + (60 * boostPercent / 100);
    }
}

// ========================
// PRESTIGE SHOP FUNCTIONS
// ========================
void initializePrestigeShop() {
    prestigeUpgrades = {
        {
            "Boost%",
            []() { return 1; },
            []() { boostPercent++; },
            []() { return true; },
            []() { return "Increase building outputs and click power by 1% (Current: " + std::to_string(boostPercent) + "%)"; }
        },
        {
            "Milk",
            []() { return 10 + (milkPurchased * 5); },
            []() { milkPurchased++; },
            []() { return true; },
            []() { 
                int cats = milkPurchased + (milkPurchased / 9);
                return "Attracts cats to reduce rats (Owned: " + std::to_string(milkPurchased) + 
                       ", Cats: " + std::to_string(cats) + ")"; 
            }
        },
        {
            "Catnip",
            []() { return 2 + (catnipLevel * 2); }, // Cheaper catnip
            []() { catnipLevel++; },
            []() { return true; },
            []() { return "Increases cat hungriness by 1% per level (Level: " + std::to_string(catnipLevel) + ")"; }
        },
        {
            "Golden Sword",
            []() { return 999; },
            []() { hasGoldenSword = true; },
            []() { return !hasGoldenSword; },
            []() { return "Purely cosmetic flex (Limited edition!)"; }
        }
    };
}

// ========================
// INTRO RENDER FUNCTION
// ========================
void renderIntro() {
    const int CONSOLE_WIDTH = 80;
    auto padLine = [&](const std::string& s) -> std::string {
        if (s.length() < CONSOLE_WIDTH)
            return s + std::string(CONSOLE_WIDTH - s.length(), ' ');
        else
            return s.substr(0, CONSOLE_WIDTH);
    };

    std::string frame;
    frame += padLine("=== PIE MAKER IDLE ===") + "\n";
    frame += padLine("Goal: Bake 1,000,000 pies!") + "\n";
    frame += padLine("Pies: " + std::to_string(introPies)) + "\n";
    frame += "\n";
    frame += padLine("[SPACE] Bake a pie!") + "\n";
    frame += "\n";
    if (introUnlockAvailable) {
        frame += padLine("[U] Unlock buildings (Cost: 50 pies)") + "\n";
        frame += "\n"; // <-- Add an extra empty line here
    }

    setCursorPos(0, 0);
    std::cout << frame;
    if (!introAnnouncement.empty()) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << padLine(introAnnouncement) << std::endl;
        SetConsoleTextAttribute(hConsole, 7);  // Reset to default
    }
    else {
        std::cout << std::endl;
    }
    std::cout << std::flush;
}

// ========================
// PRESTIGE SHOP RENDER
// ========================
void renderPrestigeShop() {
    const int CONSOLE_WIDTH = 80;
    std::string frame;

    auto padLine = [&](const std::string& s) -> std::string {
        if (s.length() < CONSOLE_WIDTH)
            return s + std::string(CONSOLE_WIDTH - s.length(), ' ');
        else
            return s.substr(0, CONSOLE_WIDTH);
    };

    frame += padLine("=== PRESTIGE SHOP ===") + "\n";
    frame += padLine("Prestige Stars: " + std::to_string((int)prestigeStars)) + "\n\n";

    for (int i = 0; i < prestigeUpgrades.size(); ++i) {
        if (prestigeUpgrades[i].isVisible()) {
            frame += padLine("[" + std::to_string(i + 1) + "] " + prestigeUpgrades[i].name + 
                           " (" + std::to_string(prestigeUpgrades[i].getCost()) + " stars)") + "\n";
            frame += padLine("   " + prestigeUpgrades[i].getDescription()) + "\n\n";
        }
    }

    frame += padLine("[0] Return to game") + "\n";

    setCursorPos(0, 0);
    std::cout << frame << std::flush;
}

// ========================
// PRESTIGE RESET
// ========================
void resetGameState() {
    totalPies = 0;
    piesPerSecond = 0;
    pendingPies = 0.0f;
    grandmas = bakeries = factories = 0;
    totalRats = 0;
    ratsEating = 0;
    grandmasVisible = bakeriesVisible = factoriesVisible = false;
    upgrades = {
        {"Better Ovens", 500, false, false, [] { piesPerSecond += grandmas; }},
        {"Industrial Wheat", 1000, false, false, [] { piesPerSecond += bakeries * 2; }},
        {"Robot Bakers", 5000, false, false, [] { piesPerSecond += factories * 3; }}
    };
    piesBakedThisRun = 0;
    announcement = "";
    announcementTimer = 0.0f;
    prestigeHintShown = false; // Reset the prestige hint for each new run
}

// ========================
// CELEBRATION + PROMPT
// ========================
void showCelebration() {
    system("cls");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    int frameIdx = 0;
    char response = 0;

    const auto& fw = fireworksFrames[0];
    int fireworksWidth = fw[0].size();
    int pieWidth = celebrationPie[0].size();
    int totalWidth = fireworksWidth + 3 + pieWidth + 3 + fireworksWidth;

    std::string congrats = "CONGRATULATIONS!";
    std::string baked = "You baked 1,000,000 pies!";

    while (true) {
        setCursorPos(0, 0);

        const auto& fw = fireworksFrames[frameIdx % fireworksFrames.size()];

        std::cout << "\n\n";

        int leftPad = (totalWidth - (int)congrats.size()) / 2;
        std::cout << std::string(leftPad, ' ') << congrats << "\n";
        leftPad = (totalWidth - (int)baked.size()) / 2;
        std::cout << std::string(leftPad, ' ') << baked << "\n\n";

        for (size_t i = 0; i < celebrationPie.size(); ++i) {
            int color;
            switch (i % 5) {
                case 0: color = 12; break;
                case 1: color = 14; break;
                case 2: color = 10; break;
                case 3: color = 11; break;
                case 4: color = 13; break;
            }
            SetConsoleTextAttribute(hConsole, color);
            std::cout << fw[i % fw.size()];
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << "   " << celebrationPie[i] << "   ";
            SetConsoleTextAttribute(hConsole, color);
            std::cout << fw[i % fw.size()] << "\n";
        }
        SetConsoleTextAttribute(hConsole, 7);

        std::cout << "\nWould you like to play again? (Y/N): ";

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
        system("cls");
        setCursorPos(0, 0);
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << "=== UNLOCKED ===\n\n";
        std::cout << "New Building: Grandmas\n\n";
        std::cout << "Grandmas bake pies with love (and arthritis)!\n";
        std::cout << "Bake 1 pie per second\nCost: 10 pies\n\n";
        SetConsoleTextAttribute(hConsole, 7); // Reset
        std::cout << "Press any key to continue...";
        _getch();
        system("cls");
        lastAnnouncement = ""; // <-- Reset here
        announcement = "Grandmas unlocked! Press G to buy (Cost: 10 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }
    if (!bakeriesVisible && totalPies >= 50) {
        bakeriesVisible = true;
        system("cls");
        setCursorPos(0, 0);
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << "=== UNLOCKED ===\n\n";
        std::cout << "New Building: Bakeries\n\n";
        std::cout << "Bakeries: Now with 100% more gluten and 200% more efficiency!\n";
        std::cout << "Bake 5 pies per second\nCost: 50 pies\n\n";
        SetConsoleTextAttribute(hConsole, 7); // Reset
        std::cout << "Press any key to continue...";
        _getch();
        system("cls");
        lastAnnouncement = ""; // <-- Reset here
        announcement = "Bakeries unlocked! Press B to buy (Cost: 50 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }
    if (!factoriesVisible && totalPies >= 200) {
        factoriesVisible = true;
        system("cls");
        setCursorPos(0, 0);
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << "=== UNLOCKED ===\n\n";
        std::cout << "New Building: Factories\n\n";
        std::cout << "Factories: For when you want more pies than friends.\n";
        std::cout << "Bake 20 pies per second\nCost: 200 pies\n\n";
        SetConsoleTextAttribute(hConsole, 7); // Reset
        std::cout << "Press any key to continue...";
        _getch();
        system("cls");
        lastAnnouncement = ""; // <-- Reset here
        announcement = "Factories unlocked! Press F to buy (Cost: 200 pies)";
        announcementTimer = ANNOUNCEMENT_DURATION;
    }

    for (auto& upgrade : upgrades) {
        if (!upgrade.visible && totalPies >= upgrade.cost / 2) {
            upgrade.visible = true;
            system("cls");
            setCursorPos(0, 0);
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, 14); // Yellow
            std::cout << "=== UNLOCKED ===\n\n";
            std::cout << "New Upgrade Available: " << upgrade.name << "\n\n";
            std::cout << "Cost: " << upgrade.cost << " pies\n";
            std::cout << "Press " << (&upgrade - &upgrades[0] + 1) << " to buy\n\n";
            SetConsoleTextAttribute(hConsole, 7); // Reset
            std::cout << "Press any key to continue...";
            _getch();
            system("cls");
            lastAnnouncement = ""; // <-- Reset here
            announcement = "Upgrade Available! " + upgrade.name + " (" + std::to_string(upgrade.cost) + " pies)";
            announcementTimer = ANNOUNCEMENT_DURATION;
        }
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

    if (totalRats > 0) {
        frame += padLine("Per second: " + std::to_string(piesPerSecond) +
            " (-" + std::to_string(ratsEating) + ")") + "\n";
    } else {
        frame += padLine("Per second: " + std::to_string(piesPerSecond)) + "\n";
    }

    frame += padLine("Prestige Stars: " + std::to_string((int)prestigeStars)) + "\n";

    // Prestige upgrades status
    if (boostPercent > 0) {
        frame += padLine("Boost: " + std::to_string(boostPercent) + "%") + "\n";
    }
    if (milkPurchased > 0) {
        frame += padLine("Milk: " + std::to_string(milkPurchased)) + "\n";
    }
    if (catnipLevel > 0) {
        frame += padLine("Catnip: " + std::to_string(catnipLevel)) + "\n";
    }
    if (totalCats > 0) {
        frame += padLine("Cats: " + std::to_string(totalCats)) + "\n";
    }
    if (hasGoldenSword) {
        frame += padLine("Golden Sword: OWNED") + "\n";
    }

    if (totalRats > 0) {
        frame += padLine("Rats: " + std::to_string(totalRats) +
            (ratsEatingSingle > 0 ? " (Each eating " + std::to_string((int)ratsEatingSingle) + " pies/sec)" : "")) + "\n\n";
    } else {
        frame += "\n";
    }

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

    if (prestigeUnlocked) {
        int piesForDisplay = std::max(piesBakedThisRun, 1000);
        frame += "\n" + padLine("[R] RESET for " + std::to_string(sqrt(piesForDisplay / 1000.0f)) + " prestige stars!") + "\n";
    }

    frame += "\n";

    std::vector<std::string> steamToShow = (idleFrame == 0) ? pieSteam1 : pieSteam2;
    std::vector<std::string> pieToShow = showPressedPie ? piePressed : pieIdle1;

    // Draw steam, then pie, then artist tag
    if (totalRats > 0 || totalCats > 0) {
        size_t steamLines = steamToShow.size();
        size_t pieLines = pieToShow.size();
        size_t ratLines = ratArt.size();
        size_t catLines = catArt.size();
        int gap = 2;

        for (size_t i = 0; i < steamLines; ++i) {
            frame += padLine(steamToShow[i]) + "\n";
        }
        for (size_t i = 0; i < std::max(pieLines, std::max(ratLines, catLines)); ++i) {
            std::string pieLine = (i < pieLines) ? pieToShow[i] : std::string(pieToShow[0].size(), ' ');
            std::string ratLine = (i < ratLines && totalRats > 0) ? ratArt[i] : "";
            std::string catLine = (i < catLines && totalCats > 0) ? catArt[i] : "";
            frame += padLine(pieLine + std::string(gap, ' ') + ratLine + std::string(gap, ' ') + catLine) + "\n";
        }
        frame += padLine("     Riitta Rasimus") + "\n";
        if (totalRats > 0) {
            std::string ratMsg = std::to_string(totalRats) + " rats are stealing your pies!";
            int pieAndGapWidth = (int)pieToShow[0].size() + gap + 19;
            frame += padLine(std::string(pieAndGapWidth, ' ') + ratMsg) + "\n";
        }
        if (totalCats > 0) {
            std::string catMsg = std::to_string(totalCats) + " cats are chasing rats!";
            int pieAndGapWidth = (int)pieToShow[0].size() + gap + 19 + gap + 13;
            frame += padLine(std::string(pieAndGapWidth, ' ') + catMsg) + "\n";
        }
    } else {
        for (const auto& line : steamToShow) frame += padLine(line) + "\n";
        for (const auto& line : pieToShow) frame += padLine(line) + "\n";
        frame += padLine("     Riitta Rasimus") + "\n";
    }

    if (!announcement.empty()) {
        if (announcement != lastAnnouncement) {
            system("cls"); // Only clear when the announcement changes
            lastAnnouncement = announcement;
        }
        setCursorPos(0, 0);
        std::cout << frame;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << "\n" << padLine(announcement) << "\n";
        SetConsoleTextAttribute(hConsole, 7);  // Reset to default
        return;
    } else {
        lastAnnouncement = ""; // Reset when no announcement
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
    srand(static_cast<unsigned int>(time(nullptr)));
    initializePrestigeShop();

    do {
        resetGameState();
        system("cls");
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        std::cout << "=== PIE MAKER IDLE ===\n\n";
        std::cout << "Your goal: Bake ONE MILLION PIES!\n\n";
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << "Start by pressing SPACE to bake your first pie.\n\n";
        SetConsoleTextAttribute(hConsole, 7);  // Reset to default
        _getch();

        // --- Intro sequence ---
        inIntro = true;
        introPies = 1;
        introSpacePresses = 0;
        introAnnouncementStep = 0;
        introAnnouncement = "";
        introAnnouncementTimer = 0.0f;
        introUnlockAvailable = false;
        introBuildingsUnlocked = false;
        introClearedAfterFirstSpace = false;

        auto lastTime = std::chrono::steady_clock::now();

        while (inIntro) {
            if (keyPressed(VK_SPACE)) {
                // Clear the screen after the first spacebar press only
                if (!introClearedAfterFirstSpace) {
                    system("cls");
                    introClearedAfterFirstSpace = true;
                    introPies = 1;           // Reset pies to 1 after first clear
                    introSpacePresses = 1;   // Also reset presses to 1 for consistency
                } else {
                    introPies++;
                    introSpacePresses++;
                }
                showPressedPie = true;
                pieAnimTimer = 5;
            }

            // Announcements at milestones
            if (introSpacePresses >= 10 && introAnnouncementStep < 1) {
                introAnnouncement = "You've boke 10 already!";
                introAnnouncementTimer = INTRO_ANNOUNCEMENT_DURATION;
                introAnnouncementStep = 1;
            }
            if (introSpacePresses >= 20 && introAnnouncementStep < 2) {
                introAnnouncement = "Only 9,999,980 more to go!";
                introAnnouncementTimer = INTRO_ANNOUNCEMENT_DURATION;
                introAnnouncementStep = 2;
            }
            if (introSpacePresses >= 30 && introAnnouncementStep < 3) {
                introAnnouncement = "Isn't this fun?";
                introAnnouncementTimer = INTRO_ANNOUNCEMENT_DURATION;
                introAnnouncementStep = 3;
            }
            if (introSpacePresses >= 40 && introAnnouncementStep < 4) {
                introAnnouncement = "Don't worry, your spacebar can handle a million presses... probably.";
                introAnnouncementTimer = INTRO_ANNOUNCEMENT_DURATION;
                introAnnouncementStep = 4;
            }
            if (introSpacePresses >= 50 && introAnnouncementStep < 5) {
                introAnnouncement = "Fine, buy some grandmas to help you.";
                introAnnouncementTimer = INTRO_ANNOUNCEMENT_DURATION;
                introAnnouncementStep = 5;
                introUnlockAvailable = true;
                system("cls"); // Clear the screen when the unlock line appears
            }

            // Unlock buildings shop option
            if (introUnlockAvailable && keyPressed('U') && introPies >= 50) {
                introPies -= 50;
                introBuildingsUnlocked = true;
                inIntro = false;
                system("cls");
            }

            // Timer for announcement
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
            if (introAnnouncementTimer > 0) {
                introAnnouncementTimer -= deltaTime;
                if (introAnnouncementTimer <= 0) {
                    introAnnouncement = "";
                }
            }

            renderIntro();

            Sleep(33);
        }

        // After the intro, transfer pies and start the main game
        totalPies = introPies;
        piesBakedThisRun = introPies;

        auto lastTimeGame = std::chrono::steady_clock::now();

        do {
            // Input
            if (keyPressed(VK_SPACE)) {
                totalPies += 1 + (1 * boostPercent / 100);
                piesBakedThisRun += 1 + (1 * boostPercent / 100);
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

            // Secret buttons
            if (keyPressed('X')) {
                totalPies = 1000000;
                piesBakedThisRun = 1000000;
            }
            if (keyPressed('Z')) {
                totalPies += 50000;
                piesBakedThisRun += 50000;
            }

            // Unlock prestige permanently once reached
            if (!prestigeUnlocked && piesBakedThisRun >= 1000) {
                prestigeUnlocked = true;
            }

            // Prestige
            if (piesBakedThisRun >= 1000 && keyPressed('R')) {
                prestigeStars += sqrt(piesBakedThisRun / 1000.0f);
                resetGameState();
                inPrestigeShop = true;
                system("cls"); // Clear the screen before entering the prestige shop

                while (inPrestigeShop) {
                    renderPrestigeShop();

                    if (_kbhit()) {
                        char choice = _getch();
                        // Flush any extra buffered input to avoid laggy/stuck keys
                        while (_kbhit()) _getch();

                        if (choice == '0') {
                            inPrestigeShop = false;
                            system("cls");
                            forceClearScreen = true; // <-- Add this line
                        } else if (choice >= '1' && choice <= '0' + prestigeUpgrades.size()) {
                            int index = choice - '1';
                            if (index >= 0 && index < prestigeUpgrades.size() && prestigeUpgrades[index].isVisible()) {
                                int cost = prestigeUpgrades[index].getCost();
                                if (prestigeStars >= cost) {
                                    prestigeStars -= cost;
                                    prestigeUpgrades[index].effect();
                                    // system("cls"); // Removed for smoother experience
                                }
                            }
                        }
                    }
                    Sleep(10); // Responsive input
                }
            }

            // Timing
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTimeGame).count();
            lastTimeGame = now;

            // Apply pies per second
            pendingPies += piesPerSecond * deltaTime;
            if (pendingPies >= 1.0f) {
                int piesToAdd = static_cast<int>(pendingPies);
                totalPies += piesToAdd;
                piesBakedThisRun += piesToAdd;
                pendingPies -= piesToAdd;
            }

            static bool ratsWereVisible = false;

            // --- CAT SYSTEM: Calculate total cats ---
            totalCats = milkPurchased + (milkPurchased / 9);

            // --- RAT SYSTEM with CATS and CATNIP ---
            if (totalPies >= RAT_THRESHOLD) {
                double progress = std::min((double)totalPies / 1000000.0, 1.0);
                double exponent = 1.01 + 0.7 * pow(progress, 2);
                totalRats = static_cast<int>(3 * pow((double)totalPies / RAT_THRESHOLD, exponent));
                if (totalRats > RAT_MAX) totalRats = RAT_MAX;

                // Cats eat rats before rats eat pies
                int ratsEatenPerCat = 2 + int(2 * catnipLevel / 100.0f + 0.5f); // 2 base, +1% per catnip star (rounded)
                int totalRatsEaten = totalCats * ratsEatenPerCat;
                if (totalRatsEaten > totalRats) totalRatsEaten = totalRats;
                totalRats -= totalRatsEaten;
                if (totalRats < 0) totalRats = 0;

                double endgameFactor = pow(progress, 3);
                float singleRatEatRate = RAT_EAT_RATE + (0.005f + 0.025f * endgameFactor) * piesPerSecond;

                int ratsEatingPerSecond = static_cast<int>(totalRats * singleRatEatRate);
                int ratsEatingThisFrame = static_cast<int>(std::min(ratsEatingPerSecond * deltaTime, (float)totalPies));
                totalPies -= ratsEatingThisFrame;

                ratsEating = ratsEatingPerSecond;
                ratsEatingSingle = singleRatEatRate;

                if (!ratsWereVisible) {
                    system("cls");
                    announcement = "";           // <-- Add this line
                    announcementTimer = 0.0f;    // <-- Add this line
                    ratsWereVisible = true;
                }
            } else {
                if (ratsWereVisible) {
                    system("cls");
                    announcement = "";           // <-- Add this line
                    announcementTimer = 0.0f;    // <-- Add this line
                    ratsWereVisible = false;
                }
                totalRats = 0;
                ratsEating = 0;
                ratsEatingSingle = 0;
            }

            // Animation
            if (pieAnimTimer > 0) {
                pieAnimTimer--;
            } else {
                showPressedPie = false;
            }

            idleTimer += deltaTime;
            if (idleTimer > 0.4f) {
                idleFrame = 1 - idleFrame;
                idleTimer = 0.0f;
            }

            // --- PRESTIGE HINT ANNOUNCEMENT ---
            if (!prestigeHintShown && piesBakedThisRun >= 500000) {
                announcement = "Tip: If progress slows down, try PRESTIGE (press R) for permanent upgrades!";
                announcementTimer = ANNOUNCEMENT_DURATION;
                prestigeHintShown = true;
            }

            if (announcementTimer > 0) {
                announcementTimer -= deltaTime;
                if (announcementTimer <= 0) {
                    announcement = "";
                }
            }

            // Before rendering each frame:
            if (forceClearScreen) {
                system("cls");
                forceClearScreen = false;
            }
            // Render
            renderFrame(deltaTime);

            Sleep(33); // ~30 FPS
        } while (!goalAchieved && totalPies < 1000000);

        if (totalPies >= 1000000) {
            showCelebration();
        }

    } while (!goalAchieved);

    return 0;
}