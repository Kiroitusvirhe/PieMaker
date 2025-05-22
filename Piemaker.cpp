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
#include <memory> // For smart pointers

// ========================
// GAME STATE VARIABLES
// ========================
struct GameState {
    int totalPies = 0;
    int piesPerSecond = 0;
    float prestigeStars = 0;
    float pendingPies = 0.0f;
    bool goalAchieved = false;
    bool prestigeUnlocked = false;
    int piesBakedThisRun = 0;
    bool prestigeHintShown = false;
    bool forceClearScreen = false;
};

GameState game;

struct Announcement {
    std::string text;
    float timer = 0.0f;

    void show(const std::string& msg, float duration = 5.0f) {
        text = msg;
        timer = duration;
    }
    void update(float dt) {
        if (timer > 0.0f) {
            timer -= dt;
            if (timer <= 0.0f) text = "";
        }
    }
    bool active() const { return !text.empty(); }
};

Announcement announcement;

struct IntroState {
    bool inIntro = true;
    int pies = 1;
    int spacePresses = 0;
    int announcementStep = 0;
    std::string announcement = "";
    float announcementTimer = 0.0f;
    bool unlockAvailable = false;
    bool buildingsUnlocked = false;
    bool clearedAfterFirstSpace = false;
};

IntroState intro;
const float INTRO_ANNOUNCEMENT_DURATION = 5.0f;

struct PrestigeUpgrade {
    std::string name;
    std::function<int()> getCost;
    std::function<void()> effect;
    std::function<bool()> isVisible;
    std::function<std::string()> getDescription;
};

struct PrestigeShop {
    int boostPercent = 0;
    int milkPurchased = 0;
    int catnipLevel = 0;
    bool hasGoldenSword = false;
    bool inShop = false;
    std::vector<PrestigeUpgrade> upgrades;

    void initialize() {
        upgrades = {
            {
                "Boost%",
                [this]() { return 1; },
                [this]() { boostPercent++; },
                [this]() { return true; },
                [this]() { return "Increase building outputs and click power by 1% (Current: " + std::to_string(boostPercent) + "%)"; }
            },
            {
                "Milk",
                [this]() { return 10 + (milkPurchased * 2); },
                [this]() { milkPurchased++; },
                [this]() { return true; },
                [this]() {
                    int cats = milkPurchased + (milkPurchased / 9);
                    return "Attracts cats to reduce rats (Owned: " + std::to_string(milkPurchased) +
                           ", Cats: " + std::to_string(cats) + ")";
                }
            },
            {
                "Catnip",
                [this]() { return 1 + (catnipLevel * 1); },
                [this]() { catnipLevel++; },
                [this]() { return true; },
                [this]() { return "Increases cat hungriness by 1% per level (Level: " + std::to_string(catnipLevel) + ")"; }
            },
            {
                "Golden Sword",
                [this]() { return 999; },
                [this]() { hasGoldenSword = true; },
                [this]() { return !hasGoldenSword; },
                [this]() { return "Purely cosmetic flex (Limited edition!)"; }
            }
        };
    }
};
PrestigeShop prestigeShop;

bool inPrestigeShop = false;

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

bool ratsJustAppeared = false;
bool ratsJustDisappeared = false;

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
    "                        .--.",
    "               (\\./)     \\.......-",
    "              >' '<  (__.'\"\"\"\"BP",
    "              \" ` \" \""
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
// PRESTIGE SHOP FUNCTIONS
// ========================
void initializePrestigeShop() {
    prestigeShop.initialize();
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
    frame += padLine("Pies: " + std::to_string(game.totalPies)) + "\n";
    frame += "\n";
    frame += padLine("[SPACE] Bake a pie!") + "\n";
    frame += "\n";
    if (intro.unlockAvailable) {
        frame += padLine("[U] Unlock buildings (Cost: 50 pies)") + "\n";
        frame += "\n";
    }

    setCursorPos(0, 0);
    std::cout << frame;
    if (!intro.announcement.empty()) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << padLine(intro.announcement) << std::endl;
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
    frame += padLine("Prestige Stars: " + std::to_string((int)game.prestigeStars)) + "\n\n";

    for (int i = 0; i < prestigeShop.upgrades.size(); ++i) {
        if (prestigeShop.upgrades[i].isVisible()) {
            frame += padLine("[" + std::to_string(i + 1) + "] " + prestigeShop.upgrades[i].name + 
                           " (" + std::to_string(prestigeShop.upgrades[i].getCost()) + " stars)") + "\n";
            frame += padLine("   " + prestigeShop.upgrades[i].getDescription()) + "\n\n";
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
    game.totalPies = 0;
    game.piesPerSecond = 0;
    game.pendingPies = 0.0f;
    game.piesBakedThisRun = 0;
    game.prestigeHintShown = false; // Reset the prestige hint for each new run
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
                game.goalAchieved = false;
                break;
            }
            if (response == 'N' || response == 'n') {
                game.goalAchieved = true;
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
// GAME CLASSES
// ========================
class ShopItem {
public:
    virtual ~ShopItem() = default;
    virtual std::string getName() const = 0;
    virtual int getCost() const = 0;
    virtual bool canPurchase(int pies) const = 0;
    virtual void purchase() = 0;
    virtual std::string getDescription() const = 0;
    virtual bool isVisible(int pies) const { return true; }
    virtual int getPiesPerSecond() const { return 0; }
};

class Building : public ShopItem {
protected:
    std::string name;
    int baseCost;
    int count;
    int piesPerSecond;
    bool visible;
    float multiplier = 1.0f;
public:
    Building(const std::string& n, int cost, int pps, bool vis = false)
        : name(n), baseCost(cost), count(0), piesPerSecond(pps), visible(vis) {}
    std::string getName() const override { return name; }
    int getCost() const override { return baseCost + count * baseCost / 2; }
    bool canPurchase(int pies) const override { return pies >= getCost(); }
    void purchase() override { count++; }
    std::string getDescription() const override {
        return name + " (Count: " + std::to_string(count) + ", +" + std::to_string(piesPerSecond) + " pies/sec)";
    }
    int getCount() const { return count; }
    // Apply prestige boost to all building production
    int getPiesPerSecond() const override {
        return int(piesPerSecond * count * multiplier * (100 + prestigeShop.boostPercent) / 100.0f);
    }
    void setMultiplier(float m) { multiplier = m; } // <-- Add this method
    bool isVisible(int pies) const override { return visible || pies >= baseCost; }
    void setVisible(bool v) { visible = v; }
};

class BuildingUpgrade : public Building {
    std::string upgradeName;
    int unlockThreshold;
    bool unlocked;
public:
    BuildingUpgrade(const std::string& n, int cost, int pps, const std::string& upg, int threshold)
        : Building(n, cost, pps, false), upgradeName(upg), unlockThreshold(threshold), unlocked(false) {}
    bool isUnlocked(int pies) { return !unlocked && pies >= unlockThreshold; }
    void unlock() { unlocked = true; }
    std::string getDescription() const override {
        return Building::getDescription() + (unlocked ? " [Upgrade: " + upgradeName + " unlocked!]" : "");
    }
    bool isVisible(int pies) const override { return pies >= unlockThreshold; }
};

class Upgrade : public ShopItem {
    std::string name;
    int cost;
    float multiplier;
    Building* target;
    bool purchased = false;
public:
    Upgrade(const std::string& n, int c, float m, Building* t)
        : name(n), cost(c), multiplier(m), target(t) {}

    std::string getName() const override { return name; }
    int getCost() const override { return cost; }
    bool canPurchase(int pies) const override { return !purchased && pies >= cost; }
    void purchase() override {
        if (!purchased && target) {
            target->setMultiplier(multiplier);
            purchased = true;
        }
    }
    std::string getDescription() const override {
        return "Boosts " + target->getName() + " output by x" + std::to_string((int)multiplier) + (purchased ? " [OWNED]" : "");
    }
    bool isVisible(int pies) const override { return pies >= cost / 2; }
};

std::vector<std::unique_ptr<ShopItem>> shopItems;

int calculatePiesPerSecond() {
    int total = 0;
    for (const auto& item : shopItems) {
        if (auto* b = dynamic_cast<Building*>(item.get())) {
            total += b->getPiesPerSecond();
        }
    }
    return total;
}

void initializeShopItems() {
    shopItems.clear();

    // Create buildings
    auto grandma = std::make_unique<Building>("Grandma", 10, 1);
    auto bakery = std::make_unique<Building>("Bakery", 50, 5);
    auto factory = std::make_unique<Building>("Factory", 200, 20);

    // Keep raw pointers for upgrades
    Building* grandmaPtr = grandma.get();
    Building* bakeryPtr = bakery.get();
    Building* factoryPtr = factory.get();

    // Add buildings to shop
    shopItems.push_back(std::move(grandma));
    shopItems.push_back(std::move(bakery));
    shopItems.push_back(std::move(factory));

    // Add upgrades to shop
    shopItems.push_back(std::make_unique<Upgrade>("Grandma's Secret Recipe", 500, 5.0f, grandmaPtr));
    shopItems.push_back(std::make_unique<Upgrade>("Bakery Automation", 2000, 3.0f, bakeryPtr));
    shopItems.push_back(std::make_unique<Upgrade>("Turbo Conveyor", 10000, 2.0f, factoryPtr));
}


// ========================
// RENDER FRAME
// ========================
void renderFrame(float deltaTime) {
    const int CONSOLE_WIDTH = 80;
    std::string frame;

    static bool lastRatState = false;
    if (lastRatState != (totalRats > 0)) {
        system("cls");
        lastRatState = (totalRats > 0);
        game.forceClearScreen = false;
    }

    bool shouldClearScreen = ratsJustAppeared || ratsJustDisappeared || game.forceClearScreen;
    if (shouldClearScreen) {
        system("cls");
        ratsJustAppeared = false;
        ratsJustDisappeared = false;
        game.forceClearScreen = false;
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
    frame += padLine("Pies: " + formatWithCommas(game.totalPies)) + "\n";

    if (totalRats > 0) {
        frame += padLine("Per second: " + std::to_string(game.piesPerSecond) +
            " (-" + std::to_string(ratsEating) + ")") + "\n";
    } else {
        frame += padLine("Per second: " + std::to_string(game.piesPerSecond)) + "\n";
    }

    frame += padLine("Prestige Stars: " + std::to_string((int)game.prestigeStars)) + "\n";

    // Prestige upgrades status
    if (prestigeShop.boostPercent > 0) {
        frame += padLine("Boost: " + std::to_string(prestigeShop.boostPercent) + "%") + "\n";
    }
    if (prestigeShop.milkPurchased > 0) {
        frame += padLine("Milk: " + std::to_string(prestigeShop.milkPurchased)) + "\n";
    }
    if (totalCats > 0 || prestigeShop.milkPurchased > 0) {
    frame += padLine("Cats: " + std::to_string(totalCats)) + "\n";
    }
    if (prestigeShop.catnipLevel > 0) {
        frame += padLine("Catnip: " + std::to_string(prestigeShop.catnipLevel)) + "\n";
    }
    if (prestigeShop.hasGoldenSword) {
        frame += padLine("Golden Sword: OWNED") + "\n";
    }

    if (totalRats > 0) {
        frame += padLine("Rats: " + std::to_string(totalRats) +
            (ratsEatingSingle > 0 ? " (Each eating " + std::to_string((int)ratsEatingSingle) + " pies/sec)" : "")) + "\n\n";
    } else {
        frame += "\n";
    }

    frame += padLine("[SPACE] Bake a pie!") + "\n\n";

    frame += "\n";

    if (game.prestigeUnlocked) {
        int piesForDisplay = std::max(game.piesBakedThisRun, 1000);
        frame += "\n" + padLine("[R] RESET for " + std::to_string(sqrt(piesForDisplay / 1000.0f)) + " prestige stars!") + "\n";
    }

    frame += "\n";

     // Render shop items
    for (int i = 0; i < shopItems.size(); ++i) {
        if (shopItems[i]->isVisible(game.totalPies)) {
            frame += padLine("[" + std::to_string(i + 1) + "] " + shopItems[i]->getName() +
                " (" + std::to_string(shopItems[i]->getCost()) + " pies) - " +
                shopItems[i]->getDescription()) + "\n";
        }
    }

    frame += "\n";

    std::vector<std::string> steamToShow = (idleFrame == 0) ? pieSteam1 : pieSteam2;
    std::vector<std::string> pieToShow = showPressedPie ? piePressed : pieIdle1;

    // Draw steam, then pie, then artist tag
    if (totalRats > 0 || totalCats > 0) {
        size_t steamLines = steamToShow.size();
        size_t pieLines = pieToShow.size();
        size_t ratLines = ratArt.size();
        const int gap = 2; // Space between elements

        // Draw steam
        for (size_t i = 0; i < steamToShow.size(); ++i) {
            frame += padLine(steamToShow[i]) + "\n";
        }

        // Calculate widths
        const int pieWidth = pieToShow.empty() ? 0 : pieToShow[0].size();
        const int ratWidth = ratArt.empty() ? 0 : ratArt[0].size();
        int maxLines = std::max(pieToShow.size(), ratArt.size());

        // Draw combined ASCII art (pie | rat)
        for (int i = 0; i < maxLines; ++i) {
            std::string line;

            // Pie
            if (i < pieToShow.size()) {
                line += pieToShow[i];
            } else {
                line += std::string(pieWidth, ' ');
            }

            // Rat (column always reserved if rats exist)
            if (totalRats > 0) {
                line += std::string(gap, ' ');
                if (i < ratArt.size()) {
                    line += ratArt[i];
                } else {
                    line += std::string(ratWidth, ' ');
                }
            }

            frame += padLine(line) + "\n";
        }
        frame += padLine("     Riitta Rasimus") + "\n";

        // Info message for rats only
        if (totalRats > 0) {
            int ratCol = pieWidth + gap + 10; 
            std::string ratMsg = std::to_string(totalRats) + " rats are stealing your pies!";
            frame += padLine(std::string(ratCol, ' ') + ratMsg) + "\n";
        }
    } else {
        for (const auto& line : steamToShow) frame += padLine(line) + "\n";
        for (const auto& line : pieToShow) frame += padLine(line) + "\n";
        frame += padLine("     Riitta Rasimus") + "\n";
    }

    setCursorPos(0, 0);
    std::cout << frame;
    if (announcement.active()) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 14);
        std::cout << "\n" << padLine(announcement.text) << "\n";
        SetConsoleTextAttribute(hConsole, 7);
    }
    std::cout << std::flush;
    setCursorPos(0, 0);
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
        initializeShopItems();
        system("cls");
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        std::cout << "=== PIE MAKER IDLE ===\n\n";
        std::cout << "Your goal: Bake ONE MILLION PIES!\n\n";
        SetConsoleTextAttribute(hConsole, 14); // Yellow
        std::cout << "Start by pressing SPACE to bake your first pie.\n\n";
        SetConsoleTextAttribute(hConsole, 7);  // Reset to default
        _getch();

        // --- Intro sequence ---
        intro.inIntro = true;
        game.totalPies = 1;
        intro.spacePresses = 0;
        intro.announcementStep = 0;
        intro.announcement = "";
        intro.announcementTimer = 0.0f;
        intro.unlockAvailable = false;
        intro.buildingsUnlocked = false;
        intro.clearedAfterFirstSpace = false;

        auto lastTime = std::chrono::steady_clock::now();

        while (intro.inIntro) {
            if (keyPressed(VK_SPACE)) {
                if (!intro.clearedAfterFirstSpace) {
                    system("cls");
                    intro.clearedAfterFirstSpace = true;
                    game.totalPies = 1;
                    intro.spacePresses = 1;
                } else {
                    game.totalPies++;
                    intro.spacePresses++;
                }
                showPressedPie = true;
                pieAnimTimer = 5;
            }

             // Handle shop item purchases
        for (int i = 0; i < shopItems.size(); ++i) {
            if (keyPressed('1' + i)) {
                if (shopItems[i]->canPurchase(game.totalPies)) {
                    game.totalPies -= shopItems[i]->getCost();
                    shopItems[i]->purchase();
                    announcement.show(shopItems[i]->getName() + " purchased!");
                }
            }
        }

        // Update pies per second
        game.piesPerSecond = calculatePiesPerSecond();

        // Unlock upgrades if available
        for (auto& item : shopItems) {
            if (auto* upg = dynamic_cast<BuildingUpgrade*>(item.get())) {
                if (upg->isUnlocked(game.totalPies)) upg->unlock();
            }
        }

            // Announcements at milestones
            if (intro.spacePresses >= 10 && intro.announcementStep < 1) {
                intro.announcement = "You've boke 10 already!";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 1;
            }
            if (intro.spacePresses >= 20 && intro.announcementStep < 2) {
                intro.announcement = "Only 9,999,980 more to go!";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 2;
            }
            if (intro.spacePresses >= 30 && intro.announcementStep < 3) {
                intro.announcement = "Isn't this fun?";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 3;
            }
            if (intro.spacePresses >= 40 && intro.announcementStep < 4) {
                intro.announcement = "Don't worry, your spacebar can handle a million presses... probably.";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 4;
            }
            if (intro.spacePresses >= 50 && intro.announcementStep < 5) {
                intro.announcement = "Fine, buy some grandmas to help you.";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 5;
                intro.unlockAvailable = true;
                system("cls");
            }

            // Unlock buildings shop option
            if (intro.unlockAvailable && keyPressed('U') && game.totalPies >= 50) {
                game.totalPies -= 50;
                intro.buildingsUnlocked = true;
                intro.inIntro = false;
                system("cls");
            }

            // Timer for announcement
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
            if (intro.announcementTimer > 0) {
                intro.announcementTimer -= deltaTime;
                if (intro.announcementTimer <= 0) {
                    intro.announcement = "";
                }
            }

            renderIntro();

            Sleep(33);
        }

        auto lastTimeGame = std::chrono::steady_clock::now();

        do {
            // Input
            if (keyPressed(VK_SPACE)) {
                game.totalPies += 1 + (1 * prestigeShop.boostPercent / 100);
                game.piesBakedThisRun += 1 + (1 * prestigeShop.boostPercent / 100);
                showPressedPie = true;
                pieAnimTimer = 5;
            }

            // Secret buttons
            if (keyPressed('X')) {
                game.totalPies = 1000000;
                game.piesBakedThisRun = 1000000;
            }
            if (keyPressed('Z')) {
                game.totalPies += 50000;
                game.piesBakedThisRun += 50000;
            }

            if (keyPressed('C')) {
                system("cls");
            }

            for (int i = 0; i < shopItems.size(); ++i) {
                if (keyPressed('1' + i)) {
                if (shopItems[i]->canPurchase(game.totalPies)) {
                    game.totalPies -= shopItems[i]->getCost();
                    shopItems[i]->purchase();
                    announcement.show(shopItems[i]->getName() + " purchased!");
                    game.piesPerSecond = calculatePiesPerSecond();
                 }
                }
            }

            // Unlock prestige permanently once reached
            if (!game.prestigeUnlocked && game.piesBakedThisRun >= 1000) {
                game.prestigeUnlocked = true;
            }

            // Prestige
            if (game.piesBakedThisRun >= 1000 && keyPressed('R')) {
                game.prestigeStars += sqrt(game.piesBakedThisRun / 1000.0f);
                resetGameState();
                inPrestigeShop = true;
                system("cls");

                while (inPrestigeShop) {
                    renderPrestigeShop();

                    if (_kbhit()) {
                        char choice = _getch();
                        while (_kbhit()) _getch();

                        if (choice == '0') {
                            inPrestigeShop = false;
                            system("cls");
                            game.forceClearScreen = true;
                        } else if (choice >= '1' && choice <= '0' + prestigeShop.upgrades.size()) {
                            int index = choice - '1';
                            if (index >= 0 && index < prestigeShop.upgrades.size() && prestigeShop.upgrades[index].isVisible()) {
                                int cost = prestigeShop.upgrades[index].getCost();
                                if (game.prestigeStars >= cost) {
                                    game.prestigeStars -= cost;
                                    prestigeShop.upgrades[index].effect();
                                }
                            }
                        }
                    }
                    Sleep(10);
                }
            }

            // Timing
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTimeGame).count();
            lastTimeGame = now;

         // announcement.update(deltaTime);

            // Track previous announcement state
            static bool wasAnnouncementActive = false;
            bool isAnnouncementActive = announcement.active();

            announcement.update(deltaTime);

            // If the announcement just disappeared, clear the screen
            if (wasAnnouncementActive && !announcement.active()) {
                system("cls");
            }

            wasAnnouncementActive = announcement.active();

            // Apply pies per second
            game.pendingPies += game.piesPerSecond * deltaTime;
            if (game.pendingPies >= 1.0f) {
                int piesToAdd = static_cast<int>(game.pendingPies);
                game.totalPies += piesToAdd;
                game.piesBakedThisRun += piesToAdd;
                game.pendingPies -= piesToAdd;
            }

            static bool ratsWereVisible = false;

            // --- CAT SYSTEM: Calculate total cats ---
            totalCats = prestigeShop.milkPurchased + (prestigeShop.milkPurchased / 9);

            // --- RAT SYSTEM with CATS and CATNIP ---
            if (game.totalPies >= RAT_THRESHOLD) {
                double progress = std::min((double)game.totalPies / 1000000.0, 1.0);
                double exponent = 1.01 + 0.7 * pow(progress, 2);
                totalRats = static_cast<int>(3 * pow((double)game.totalPies / RAT_THRESHOLD, exponent));
                if (totalRats > RAT_MAX) totalRats = RAT_MAX;

                // Cats eat rats before rats eat pies
                int ratsEatenPerCat = 3 + int(3 * pow(1.5, prestigeShop.catnipLevel) / 100.0f);
                int totalRatsEaten = totalCats * ratsEatenPerCat;
                if (totalRatsEaten > totalRats) totalRatsEaten = totalRats;
                totalRats -= totalRatsEaten;
                if (totalRats < 0) totalRats = 0;
                
                int ratsBeingEaten = totalRatsEaten;  // Track how many rats cats are eating
                float catsEfficiency = (totalCats > 0) ? (float)ratsEatenPerCat : 0.0f;  // Rats per cat
                double endgameFactor = pow(progress, 3);
                float singleRatEatRate = RAT_EAT_RATE + (0.005f + 0.025f * endgameFactor) * game.piesPerSecond;

                int ratsEatingPerSecond = static_cast<int>(totalRats * singleRatEatRate);
                int ratsEatingThisFrame = static_cast<int>(std::min(ratsEatingPerSecond * deltaTime, (float)game.totalPies));
                game.totalPies -= ratsEatingThisFrame;
                ratsEating = ratsEatingPerSecond;
                ratsEatingSingle = singleRatEatRate;

                if (!ratsWereVisible) {
                    ratsJustAppeared = true;
                    ratsWereVisible = true;
                }
            } else {
                if (ratsWereVisible) {
                    ratsJustDisappeared = true;
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
            if (!game.prestigeHintShown && game.piesBakedThisRun >= 500000) {
                announcement.show("Tip: If progress slows down, try PRESTIGE (press R) for permanent upgrades!");
                game.prestigeHintShown = true;
            }

            if (game.forceClearScreen) {
                system("cls");
                game.forceClearScreen = false;
            }

            renderFrame(deltaTime);

            Sleep(33);
        } while (!game.goalAchieved && game.totalPies < 1000000);

        if (game.totalPies >= 1000000) {
            showCelebration();
        }
    } while (!game.goalAchieved);

    return 0;
}