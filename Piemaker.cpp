#include <iostream>        // Used for input/output like std::cout
#include <windows.h>       // Windows-specific functions like GetAsyncKeyState and Sleep
#include <cmath>           // For math operations like sqrt
#include <chrono>          // For measuring time accurately
#include <vector>          // Dynamic array container
#include <string>          // For using strings
#include <functional>      // Allows storing functions inside variables (used for upgrades)

// ========================
// GAME STATE VARIABLES
// ========================

// Core resources
int totalPies = 0;         // Total pies the player has
int piesPerSecond = 0;     // How many pies are automatically made each second

// Buildings owned by the player
int grandmas = 0;          // +1 pie/sec each
int bakeries = 0;          // +5 pies/sec each
int factories = 0;         // +20 pies/sec each

// Prestige system
float prestigeStars = 0;   // Stars earned by resetting progress
float pendingPies = 0.0f;  // Tracks fractional pies for smooth timing

// ========================
// UPGRADE SYSTEM
// ========================

// Each upgrade has a name, cost, purchased state, and an effect function
struct Upgrade {
    std::string name;
    int cost;
    bool purchased;
    std::function<void()> effect;
};

// List of available upgrades
std::vector<Upgrade> upgrades = {
    {"Better Ovens", 500, false, [] { piesPerSecond += grandmas; }},
    {"Industrial Wheat", 1000, false, [] { piesPerSecond += bakeries * 2; }},
    {"Robot Bakers", 5000, false, [] { piesPerSecond += factories * 3; }}
};

// ========================
// PIE ASCII ART & ANIMATION
// ========================

// Default pie (idle frame A)
std::vector<std::string> pieIdle1 = {
    "   (  )   ",
    "  (    )  ",
    " (______) ",
    "  \\====/  ",
    "   \\__/   "
};

// Idle frame B (used for idle animation)
std::vector<std::string> pieIdle2 = {
    "   (  )   ",
    "  ( .. )  ",
    " (------) ",
    "  \\====/  ",
    "   \\__/   "
};

// Pressed pie when clicked
std::vector<std::string> piePressed = {
    "   \\  /   ",
    "  ( oo )  ",
    " (======) ",
    "  /====\\  ",
    "   /__\\   "
};

// Controls which pie to display
bool showPressedPie = false;     // True if pie is currently "clicked"
int pieAnimTimer = 0;            // Countdown to end the click animation
int idleFrame = 0;               // Current idle animation frame
float idleTimer = 0.0f;          // Timer to control idle frame switching

// ========================
// CONSOLE DISPLAY HELPERS
// ========================

// Console handle needed for Windows terminal functions
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// Hide the blinking cursor in the console
void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// Moves the cursor to a specific location in the console window
void setCursorPos(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(hConsole, coord);
}

// ========================
// KEY INPUT DETECTION
// ========================

// Returns true if the given key was just pressed
bool keyPressed(int key) {
    static bool keyStates[256] = { false };  // Remembers key states
    bool currentState = (GetAsyncKeyState(key) & 0x8000);

    if (currentState && !keyStates[key]) {
        keyStates[key] = true;
        return true;  // New key press detected
    } else if (!currentState) {
        keyStates[key] = false;  // Reset on key release
    }
    return false;
}

// ========================
// BUILDING PURCHASE FUNCTIONS
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
// PRESTIGE RESET FUNCTION
// ========================

void resetForPrestige() {
    float starsGained = sqrt(totalPies / 1000.0f);
    prestigeStars += starsGained;

    // Reset everything except stars
    totalPies = 0;
    piesPerSecond = 0;
    pendingPies = 0.0f;
    grandmas = bakeries = factories = 0;
    // You can reset upgrades here too if you want
}

// ========================
// FRAME RENDERING
// ========================

const int SCREEN_HEIGHT = 25;                   // Console lines
std::vector<std::string> screenBuffer(SCREEN_HEIGHT); // Lines to draw

// Builds the screen content each frame
void buildFrame() {
    for (auto& line : screenBuffer) line.clear();  // Clear previous frame

    // Stats and menus
    screenBuffer[0] = "=== PIE MAKER IDLE ===";
    screenBuffer[1] = "Pies: " + std::to_string(totalPies);
    screenBuffer[2] = "Per second: " + std::to_string(piesPerSecond);
    screenBuffer[3] = "Prestige Stars: " + std::to_string(prestigeStars);
    screenBuffer[5] = "[SPACE] Bake a pie!";
    screenBuffer[7] = "[G] Grandmas: " + std::to_string(grandmas) + " (Cost: " + std::to_string(10 + grandmas * 2) + ")";
    screenBuffer[8] = "[B] Bakeries: " + std::to_string(bakeries) + " (Cost: " + std::to_string(50 + bakeries * 10) + ")";
    screenBuffer[9] = "[F] Factories: " + std::to_string(factories) + " (Cost: " + std::to_string(200 + factories * 50) + ")";

    // Show available upgrades
    for (int i = 0; i < upgrades.size(); i++) {
        if (!upgrades[i].purchased && totalPies >= upgrades[i].cost / 2) {
            screenBuffer[11 + i] = "[" + std::to_string(i + 1) + "] " + upgrades[i].name +
                                   " (" + std::to_string(upgrades[i].cost) + " pies)";
        }
    }

    // Prestige option appears after 1000 pies
    if (totalPies >= 1000) {
        screenBuffer[15] = "[R] RESET for " + std::to_string(sqrt(totalPies / 1000.0f)) + " prestige stars!";
    }

    // Choose which pie graphic to show
    std::vector<std::string> pieToShow;
    if (showPressedPie) {
        pieToShow = piePressed;
    } else {
        pieToShow = (idleFrame == 0 ? pieIdle1 : pieIdle2);
    }

    // Display pie starting at line 17
    for (int i = 0; i < pieToShow.size(); i++) {
        if (17 + i < screenBuffer.size())
            screenBuffer[17 + i] = pieToShow[i];
    }
}

// Outputs the current screen buffer to the console
void renderFrame() {
    setCursorPos(0, 0);  // Reset cursor to top
    for (const auto& line : screenBuffer) {
        std::cout << line << std::string(80 - line.length(), ' ') << "\n";
    }
}

// ========================
// MAIN GAME LOOP
// ========================

int main() {
    hideCursor();         // Hide cursor for cleaner display
    system("cls");        // Clear screen

    auto lastTime = std::chrono::steady_clock::now();  // Track last update time

    while (true) {
        // --- INPUT ---
        if (keyPressed(VK_SPACE)) {
            totalPies++;                 // Manual pie click
            showPressedPie = true;       // Trigger click animation
            pieAnimTimer = 5;            // Duration for click animation (in frames)
        }

        if (keyPressed('G')) buyGrandma();
        if (keyPressed('B')) buyBakery();
        if (keyPressed('F')) buyFactory();

        // Handle upgrade purchases
        for (int i = 0; i < upgrades.size(); i++) {
            if (keyPressed('1' + i) && !upgrades[i].purchased && totalPies >= upgrades[i].cost) {
                totalPies -= upgrades[i].cost;
                upgrades[i].purchased = true;
                upgrades[i].effect();
            }
        }

        // Handle prestige
        if (totalPies >= 1000 && keyPressed('R')) {
            resetForPrestige();
        }

        // --- TIMING ---
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();  // Seconds
        lastTime = now;

        // Generate pies based on time
        pendingPies += piesPerSecond * deltaTime;
        if (pendingPies >= 1.0f) {
            int piesToAdd = static_cast<int>(pendingPies);
            totalPies += piesToAdd;
            pendingPies -= piesToAdd;
        }

        // Handle click animation timeout
        if (pieAnimTimer > 0) {
            pieAnimTimer--;
            if (pieAnimTimer == 0) showPressedPie = false;
        }

        // Idle animation switching every 0.5 seconds
        idleTimer += deltaTime;
        if (idleTimer >= 0.5f) {
            idleFrame = 1 - idleFrame;  // Flip between 0 and 1
            idleTimer = 0.0f;
        }

        // --- RENDER ---
        buildFrame();
        renderFrame();
        Sleep(10);  // Delay to prevent CPU overload
    }

    return 0;
}