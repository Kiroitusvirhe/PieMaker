#include <iostream>      // For input/output streams
#include <windows.h>     // For Windows-specific console manipulation
#include <cmath>         // For math functions like pow, sqrt
#include <chrono>        // For timing and frame rate control
#include <vector>        // For dynamic arrays (STL vector)
#include <string>        // For string manipulation
#include <functional>    // For std::function (used in upgrades)
#include <conio.h>       // For _getch() and _kbhit() (keyboard input)
#include <sstream>       // For string streams
#include <locale>        // For locale-specific formatting (not heavily used)
#include <cstdlib>       // For system(), srand(), rand()
#include <ctime>         // For time()
#include <memory>        // For smart pointers (unique_ptr)

// All game logic, variables, and functions are wrapped in the piegame namespace for organization
namespace piegame {

// ========================
// CAT SYSTEM
// ========================
// Handles all logic and state related to cats and catnip
struct CatSystem {
    int milkPurchased = 0;   // Number of milk upgrades (affects cats)
    int catnipLevel = 0;     // Catnip level (affects cats' rat-eating power)

    // Calculate total cats based on milk upgrades
    int getTotalCats() const {
        return milkPurchased + (milkPurchased / 9);
    }

    // Calculate how many rats a single cat can eat per second
    int ratsEatenPerCat() const {
        return 3 + int(3 * pow(1.5, catnipLevel) / 100.0f);
    }
};

CatSystem catSystem; // The global cat system

// ========================
// RAT SYSTEM
// ========================
// Handles the logic for rats that steal pies if you have too many pies
class RatSystem {
private:
    int totalRats = 0;           // Number of rats currently present
    int ratsEating = 0;          // Number of rats eating pies per second
    float ratsEatingSingle = 0.0f; // How many pies a single rat eats per second
    bool ratsWereVisible = false; // Used to track if rats were visible last frame
    const int RAT_THRESHOLD = 50000; // Minimum pies before rats appear
    const float RAT_EAT_RATE = 1.0f; // Base rate at which rats eat pies
    const int RAT_MAX = 999999;      // Maximum number of rats

public:
    // Updates the rat system each frame
    void update(float deltaTime, int& totalPies, int piesPerSecond) {
        if (totalPies >= RAT_THRESHOLD) {
            // Calculate how many rats should appear based on total pies
            double progress = std::min((double)totalPies / 1000000.0, 1.0);
            double exponent = 1.01 + 0.7 * pow(progress, 2);
            totalRats = static_cast<int>(3 * pow((double)totalPies / RAT_THRESHOLD, exponent));
            if (totalRats > RAT_MAX) totalRats = RAT_MAX;

            // Cats eat rats before rats eat pies
            int totalCats = catSystem.getTotalCats();
            int ratsEatenPerCat = catSystem.ratsEatenPerCat();
            int totalRatsEaten = totalCats * ratsEatenPerCat;
            if (totalRatsEaten > totalRats) totalRatsEaten = totalRats;
            totalRats -= totalRatsEaten;
            if (totalRats < 0) totalRats = 0;

            // Rats eat pies
            float singleRatEatRate = RAT_EAT_RATE + (0.005f + 0.025f * pow(progress, 3)) * piesPerSecond;
            int ratsEatingPerSecond = static_cast<int>(totalRats * singleRatEatRate);
            int ratsEatingThisFrame = static_cast<int>(std::min(ratsEatingPerSecond * deltaTime, (float)totalPies));
            totalPies -= ratsEatingThisFrame;
            ratsEating = ratsEatingPerSecond;
            ratsEatingSingle = singleRatEatRate;
        } else {
            // No rats if under threshold
            totalRats = 0;
            ratsEating = 0;
            ratsEatingSingle = 0;
        }
    }

    // Renders rat ASCII art and rat info to the frame string
    void render(std::string& frame, const std::vector<std::string>& ratArt, int pieWidth, int gap) const {
        if (totalRats > 0) {
            for (const auto& line : ratArt) {
                frame += line + "\n";
            }
            int ratCol = pieWidth + gap + 10;
            std::string ratMsg = std::to_string(totalRats) + " rats are stealing your pies!";
            frame += std::string(ratCol, ' ') + ratMsg + "\n";
        }
    }

    // Getters for rat stats
    int getTotalRats() const { return totalRats; }
    int getRatsEating() const { return ratsEating; }
    float getRatsEatingSingle() const { return ratsEatingSingle; }
    bool areRatsVisible() const { return totalRats > 0; }
    bool wereRatsVisible() const { return ratsWereVisible; }
    void setRatsWereVisible(bool v) { ratsWereVisible = v; }
};

// ========================
// GAME STATE VARIABLES
// ========================
// Holds all persistent game state for the current run
struct GameState {
    int totalPies = 0;           // Total pies baked
    int piesPerSecond = 0;       // Current pies per second
    float prestigeStars = 0;     // Prestige currency
    float pendingPies = 0.0f;    // For fractional pie accumulation
    bool goalAchieved = false;   // Has the player reached the win condition?
    bool prestigeUnlocked = false; // Has prestige been unlocked?
    int piesBakedThisRun = 0;    // Pies baked in this run (for prestige)
    bool prestigeHintShown = false; // Has the prestige hint been shown?
    bool forceClearScreen = false;  // Should the screen be cleared next frame?
    RatSystem ratSystem;         // The rat system for this game
};

// Overload << to print GameState for debugging
std::ostream& operator<<(std::ostream& os, const GameState& gs) {
    os << "Pies: " << gs.totalPies
       << "\nPrestige: " << gs.prestigeStars
       << "\nRats: " << gs.ratSystem.getTotalRats();
    return os;
}

GameState game; // The main game state object

// ========================
// ANNOUNCEMENT SYSTEM
// ========================
// Used to display temporary messages to the player
struct Announcement {
    std::string text;   // The message text
    float timer = 0.0f; // How long the message should be shown

    // Show a new announcement for a given duration
    void show(const std::string& msg, float duration = 5.0f) {
        text = msg;
        timer = duration;
    }
    // Update the timer each frame
    void update(float dt) {
        if (timer > 0.0f) {
            timer -= dt;
            if (timer <= 0.0f) text = "";
        }
    }
    // Is an announcement currently active?
    bool active() const { return !text.empty(); }
};

Announcement announcement; // The global announcement object

// ========================
// INTRO STATE
// ========================
// Tracks the state of the intro/tutorial sequence
struct IntroState {
    bool inIntro = true;             // Are we in the intro?
    int spacePresses = 0;            // How many times space has been pressed
    int announcementStep = 0;        // Which milestone announcement we're on
    std::string announcement = "";   // Current intro announcement
    float announcementTimer = 0.0f;  // Timer for intro announcement
    bool unlockAvailable = false;    // Can the player unlock buildings?
    bool buildingsUnlocked = false;  // Has the player unlocked buildings?
    bool clearedAfterFirstSpace = false; // Has the screen been cleared after first space?
};

IntroState intro; // The global intro state
const float INTRO_ANNOUNCEMENT_DURATION = 5.0f; // How long intro messages last

// ========================
// PRESTIGE SHOP SYSTEM
// ========================
// Represents a single prestige upgrade
struct PrestigeUpgrade {
    std::string name;
    std::function<int()> getCost;           // Function to get cost
    std::function<void()> effect;           // Function to apply effect
    std::function<bool()> isVisible;        // Function to check visibility
    std::function<std::string()> getDescription; // Function to get description
};

// The prestige shop, holding all upgrades and their state
struct PrestigeShop {
    int boostPercent = 0;       // % boost to all production
    bool hasGoldenSword = false;// Cosmetic upgrade
    bool inShop = false;        // Is the player in the shop?
    std::vector<PrestigeUpgrade> upgrades; // All available upgrades

    // Initialize all prestige upgrades
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
                []() { return 10 + (catSystem.milkPurchased * 2); },
                []() { catSystem.milkPurchased++; },
                []() { return true; },
                []() {
                    int cats = catSystem.getTotalCats();
                    return "Attracts cats to reduce rats (Owned: " + std::to_string(catSystem.milkPurchased) +
                           ", Cats: " + std::to_string(cats) + ")";
                }
            },
            {
                "Catnip",
                []() { return 1 + (catSystem.catnipLevel * 1); },
                []() { catSystem.catnipLevel++; },
                []() { return true; },
                []() { return "Increases cat hungriness (Level: " + std::to_string(catSystem.catnipLevel) + ")"; }
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
PrestigeShop prestigeShop; // The global prestige shop

bool inPrestigeShop = false; // Is the player currently in the prestige shop?

// ========================
// REMOVE OLD CAT VARIABLES
// ========================
// int totalCats = 0; // <-- REMOVE THIS LINE

// ========================
// ASCII ART
// ========================
// Various ASCII art assets for the game
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

bool showPressedPie = false; // Should the pressed pie art be shown?
int pieAnimTimer = 0;        // Timer for pie press animation
int idleFrame = 0;           // Which idle frame to show
float idleTimer = 0.0f;      // Timer for idle animation

std::vector<std::string> ratArt = {
    "                        .--.",
    "               (\\./)     \\.......-",
    "              >' '<  (__.'\"\"\"\"BP",
    "              \" ` \" \""
};

// ========================
// CONSOLE HELPERS
// ========================
// Hide the blinking cursor in the console
void hideCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// Set the cursor position in the console (for overwriting output)
void setCursorPos(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// ========================
// INPUT HANDLING
// ========================
// Returns true if the given key was just pressed (not held)
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
// Initializes the prestige shop upgrades
void initializePrestigeShop() {
    prestigeShop.initialize();
}

// ========================
// INTRO RENDER FUNCTION
// ========================
// Renders the intro/tutorial screen
void renderIntro() {
    const int CONSOLE_WIDTH = 80;
    // Helper lambda to pad lines to fixed width
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
    } else {
        std::cout << std::endl;
    }
    std::cout << std::flush;
}

// ========================
// PRESTIGE SHOP RENDER
// ========================
// Renders the prestige shop screen
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
// Resets the game state for a new run (after prestige or win)
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
// Shows the celebration screen and asks if the player wants to play again
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
// Formats an integer with commas (e.g., 1000000 -> 1,000,000)
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

// Abstract base class for all shop items (buildings and upgrades)
class ShopItem {
protected:
    bool wasVisible = false; // Tracks if the item has ever been visible
public:
    virtual ~ShopItem() = default;
    virtual std::string getName() const = 0;
    virtual int getCost() const = 0;
    virtual bool canPurchase(int pies) const = 0;
    virtual void purchase() = 0;
    virtual std::string getDescription() const = 0;
    virtual bool isVisible(int pies) const { return true; }
    virtual int getPiesPerSecond() const { return 0; }

    // Tracks if the item has ever been visible (for display logic)
    bool hasBeenVisible() const { return wasVisible; }
    void setWasVisible() { wasVisible = true; }
};

// Represents a building that produces pies per second
class Building : public ShopItem {
protected:
    std::string name;
    int baseCost;
    int count;
    int piesPerSecond;
    bool visible;
    float multiplier = 1.0f;
public:
    // Main constructor
    Building(const std::string& n, int cost, int pps, bool vis = false)
        : name(n), baseCost(cost), count(0), piesPerSecond(pps), visible(vis) {}

    // Overloaded constructor: only name and cost, default pps and visible
    Building(const std::string& n, int cost)
        : name(n), baseCost(cost), count(0), piesPerSecond(1), visible(false) {}

    // Overloaded constructor: only name, default cost, pps, visible
    Building(const std::string& n)
        : name(n), baseCost(10), count(0), piesPerSecond(1), visible(false) {}

    // Custom destructor for demonstration (can print debug info)
    ~Building() override {
        // std::cout << "Building '" << name << "' destroyed.\n";
    }

    std::string getName() const override { return name; }
    int getCost() const override { return baseCost + count * baseCost / 2; }
    bool canPurchase(int pies) const override { return pies >= getCost(); }
    void purchase() override { count++; }
    std::string getDescription() const override {
        int actualPPS = getPiesPerSecond();
        return name + " (Count: " + std::to_string(count) + ", +" + std::to_string(actualPPS) + " pies/sec)";
    }
    int getCount() const { return count; }
    int getPiesPerSecond() const override {
        return int(piesPerSecond * count * multiplier * (100 + prestigeShop.boostPercent) / 100.0f);
    }
    void multiplyMultiplier(float m) { multiplier *= m; }
    bool isVisible(int pies) const override { return visible || pies >= baseCost; }
    void setVisible(bool v) { visible = v; }
};

// Represents an upgrade that boosts a building's output
class Upgrade : public ShopItem {
    std::string name;
    int cost;
    float multiplier;
    Building* target;
    bool purchased = false;
    Upgrade* prerequisite = nullptr;
public:
    Upgrade(const std::string& n, int c, float m, Building* t, Upgrade* prereq = nullptr)
        : name(n), cost(c), multiplier(m), target(t), prerequisite(prereq) {}

    std::string getName() const override { return name; }
    int getCost() const override { return cost; }
    bool canPurchase(int pies) const override { return !purchased && pies >= cost; }
    void purchase() override {
        if (!purchased && target) {
            target->multiplyMultiplier(multiplier); // Multiply output
            purchased = true;
        }
    }
    std::string getDescription() const override {
        return "Boosts " + target->getName() + " output by x" + std::to_string((int)multiplier);
    }
    bool isVisible(int pies) const override {
        // Only visible if not purchased, you have enough pies, and prerequisite (if any) is purchased
        bool prereqOk = !prerequisite || prerequisite->isPurchased();
        return !purchased && prereqOk && pies >= cost / 2;
    }
    bool isPurchased() const { return purchased; }
};

// All shop items (buildings and upgrades) are stored here
std::vector<std::unique_ptr<ShopItem>> shopItems;

// Calculates the total pies per second from all buildings
int calculatePiesPerSecond() {
    int total = 0;
    for (const auto& item : shopItems) {
        if (auto* b = dynamic_cast<Building*>(item.get())) {
            total += b->getPiesPerSecond();
        }
    }
    return total;
}

// Initializes all shop items (buildings and upgrades)
void initializeShopItems() {
    shopItems.clear();

    // Create buildings
    auto grandma = std::make_unique<Building>("Grandma", 10, 1);
    auto bakery = std::make_unique<Building>("Bakery", 50, 5);
    auto factory = std::make_unique<Building>("Factory", 200, 20);

    Building* grandmaPtr = grandma.get();
    Building* bakeryPtr = bakery.get();
    Building* factoryPtr = factory.get();

    // Add buildings to shop
    shopItems.push_back(std::move(grandma));
    shopItems.push_back(std::move(bakery));
    shopItems.push_back(std::move(factory));

    // First round of upgrades
    auto grandmaUpgrade1 = new Upgrade("Grandma's Secret Recipe", 500, 5.0f, grandmaPtr);
    auto bakeryUpgrade1 = new Upgrade("Bakery Automation", 2000, 3.0f, bakeryPtr);
    auto factoryUpgrade1 = new Upgrade("Turbo Conveyor", 10000, 2.0f, factoryPtr);

    shopItems.push_back(std::unique_ptr<Upgrade>(grandmaUpgrade1));
    shopItems.push_back(std::unique_ptr<Upgrade>(bakeryUpgrade1));
    shopItems.push_back(std::unique_ptr<Upgrade>(factoryUpgrade1));

    // Second round of upgrades, with prerequisites
    shopItems.push_back(std::make_unique<Upgrade>("Grandma's Robot Arms", 5000, 3.0f, grandmaPtr, grandmaUpgrade1));
    shopItems.push_back(std::make_unique<Upgrade>("Bakery Franchise", 15000, 2.5f, bakeryPtr, bakeryUpgrade1));
    shopItems.push_back(std::make_unique<Upgrade>("Factory AI Overlord", 50000, 2.0f, factoryPtr, factoryUpgrade1));
}

// ========================
// RENDER FRAME
// ========================
// Renders the main game screen each frame
void renderFrame(float deltaTime) {
    const int CONSOLE_WIDTH = 80;
    std::string frame;

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

    if (game.ratSystem.getTotalRats() > 0) {
        frame += padLine("Per second: " + std::to_string(game.piesPerSecond) +
            " (-" + std::to_string(game.ratSystem.getRatsEating()) + ")") + "\n";
    } else {
        frame += padLine("Per second: " + std::to_string(game.piesPerSecond)) + "\n";
    }

    frame += padLine("Prestige Stars: " + std::to_string((int)game.prestigeStars)) + "\n";

    // Prestige upgrades status
    if (prestigeShop.boostPercent > 0) {
        frame += padLine("Boost: " + std::to_string(prestigeShop.boostPercent) + "%") + "\n";
    }
    if (catSystem.milkPurchased > 0) {
        frame += padLine("Milk: " + std::to_string(catSystem.milkPurchased)) + "\n";
    }
    if (catSystem.getTotalCats() > 0 || catSystem.milkPurchased > 0) {
        frame += padLine("Cats: " + std::to_string(catSystem.getTotalCats())) + "\n";
    }
    if (catSystem.catnipLevel > 0) {
        frame += padLine("Catnip: " + std::to_string(catSystem.catnipLevel)) + "\n";
    }
    if (prestigeShop.hasGoldenSword) {
        frame += padLine("Golden Sword: OWNED") + "\n";
    }

    if (game.ratSystem.getTotalRats() > 0) {
        frame += padLine("Rats: " + std::to_string(game.ratSystem.getTotalRats()) +
            (game.ratSystem.getRatsEatingSingle() > 0 ? " (Each eating " + std::to_string((int)game.ratSystem.getRatsEatingSingle()) + " pies/sec)" : "")) + "\n\n";
    } else {
        frame += "\n";
    }

    frame += padLine("[SPACE] Bake a pie!") + "\n\n";

    if (game.prestigeUnlocked) {
        int piesForDisplay = std::max(game.piesBakedThisRun, 1000);
        frame += padLine("[R] RESET for " + std::to_string(sqrt(piesForDisplay / 1000.0f)) + " prestige stars!") + "\n\n";
    }

    // Render shop items
    int buildingCount = 3; // Number of buildings at the start of shopItems
    bool upgradesShown = false;
    for (int i = 0; i < shopItems.size(); ++i) {
        // Mark as visible if currently visible
        if (shopItems[i]->isVisible(game.totalPies)) {
            shopItems[i]->setWasVisible();
        }
        // Add a blank line before the first upgrade
        if (i == buildingCount && !upgradesShown) {
            frame += "\n";
            upgradesShown = true;
        }
        // Buildings: show if ever visible
        if (i < buildingCount) {
            if (shopItems[i]->hasBeenVisible()) {
                frame += padLine("[" + std::to_string(i + 1) + "] " + shopItems[i]->getName() +
                    " (" + std::to_string(shopItems[i]->getCost()) + " pies) - " +
                    shopItems[i]->getDescription()) + "\n";
            }
        }
        // Upgrades: show only if not purchased and has been visible
        else {
            Upgrade* upg = dynamic_cast<Upgrade*>(shopItems[i].get());
            if (upg && !upg->isPurchased() && shopItems[i]->hasBeenVisible()) {
                frame += padLine("[" + std::to_string(i + 1) + "] " + shopItems[i]->getName() +
                    " (" + std::to_string(shopItems[i]->getCost()) + " pies) - " +
                    shopItems[i]->getDescription()) + "\n";
            }
        }
    }

    frame += "\n";

    std::vector<std::string> steamToShow = (idleFrame == 0) ? pieSteam1 : pieSteam2;
    std::vector<std::string> pieToShow = showPressedPie ? piePressed : pieIdle1;

    // Draw steam, then pie, then artist tag
    if (game.ratSystem.getTotalRats() > 0 || catSystem.getTotalCats() > 0) {
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
            if (game.ratSystem.getTotalRats() > 0) {
                line += std::string(gap, ' ');
                if (i < ratArt.size()) {
                    line += ratArt[i];
                } else {
                    line += std::string(ratWidth, ' ');
                }
            }
            frame += padLine(line) + "\n";
        }

        // Add this block to show the rats message under the rat art
        if (game.ratSystem.getTotalRats() > 0) {
            int ratCol = pieToShow.empty() ? 0 : pieToShow[0].size();
            ratCol += gap;
            std::string ratMsg = std::string(12, ' ') + std::to_string(game.ratSystem.getTotalRats()) + " rats are stealing your pies!";
            frame += padLine(std::string(ratCol, ' ') + ratMsg) + "\n";
        }

        frame += padLine("     Riitta Rasimus") + "\n";

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
}

} // End of namespace piegame

// ========================
// MAIN GAME LOOP
// ========================
// The main function runs the game loop and handles all user interaction
int main() {
    using namespace piegame; // Use all game logic from the piegame namespace

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

        // Intro/tutorial loop
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

            // Handle shop item purchases in intro
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

            // Announcements at milestones
            if (intro.spacePresses >= 10 && intro.announcementStep < 1) {
                intro.announcement = "You've boke 10 already!";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 1;
            }
            if (intro.spacePresses >= 20 && intro.announcementStep < 2) {
                intro.announcement = "Isn't this so much fun?";
                intro.announcementTimer = INTRO_ANNOUNCEMENT_DURATION;
                intro.announcementStep = 2;
            }
            if (intro.spacePresses >= 30 && intro.announcementStep < 3) {
                intro.announcement = "Only 9,999,980 more to go!";
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
                game.forceClearScreen = true;
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

        // Main game loop
        do {
            // Input: bake pies
            if (keyPressed(VK_SPACE)) {
                game.totalPies += 1 + (1 * prestigeShop.boostPercent / 100);
                game.piesBakedThisRun += 1 + (1 * prestigeShop.boostPercent / 100);
                showPressedPie = true;
                pieAnimTimer = 5;
            }

            // Secret buttons for testing
            if (keyPressed('X')) {
                game.totalPies = 1000000;
                game.piesBakedThisRun = 1000000;
            }
            if (keyPressed('Z')) {
                game.totalPies += 50000;
                game.piesBakedThisRun += 50000;
            }

            // Manual screen clear
            if (keyPressed('C')) {
                system("cls");
            }

            // Handle shop item purchases
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

            // Prestige logic
            if (game.piesBakedThisRun >= 1000 && keyPressed('R')) {
                game.prestigeStars += sqrt(game.piesBakedThisRun / 1000.0f);
                resetGameState();
                initializeShopItems();
                inPrestigeShop = true;
                system("cls");

                // Prestige shop loop
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

            // Timer for frame timing
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTimeGame).count();
            lastTimeGame = now;

            // Announcement update and screen clear logic
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

            // --- CAT SYSTEM: No need to calculate totalCats, use catSystem.getTotalCats() everywhere ---

            // --- RAT SYSTEM ---
            game.ratSystem.update(deltaTime, game.totalPies, game.piesPerSecond);

            // Handle rat appearance/disappearance for screen clearing
            static bool lastRatState = false;
            if (game.ratSystem.areRatsVisible() != lastRatState) {
                system("cls");
                lastRatState = game.ratSystem.areRatsVisible();
            }

            // Animation for pie press
            if (pieAnimTimer > 0) {
                pieAnimTimer--;
            } else {
                showPressedPie = false;
            }

            // Idle animation
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

            // Force clear screen if requested
            if (game.forceClearScreen) {
                system("cls");
                game.forceClearScreen = false;
            }

            renderFrame(deltaTime);

            Sleep(33); // ~30 FPS
        } while (!game.goalAchieved && game.totalPies < 1000000);

        // If the player wins, show celebration
        if (game.totalPies >= 1000000) {
            showCelebration();
        }
    } while (!game.goalAchieved);

    return 0;
}