#include <iostream>    // For input/output (cout, cin)
#include <windows.h>   // For Windows-specific functions (console control)
#include <cmath>       // For math functions (sqrt)
#include <chrono>      // For precise timing (frame rate control)
#include <vector>      // For dynamic arrays (screen buffer)
#include <string>      // For string manipulation
#include <functional>  // For storing functions (upgrade effects)

// ========================
// GAME STATE - All variables that track progress
// ========================
int totalPies = 0;         // Player's current pies
int piesPerSecond = 0;     // Pies generated automatically each second

// Buildings - each provides pies per second
int grandmas = 0;          // Basic producers (+1 pie/sec each)
int bakeries = 0;          // Mid-tier producers (+5 pies/sec each)
int factories = 0;         // Advanced producers (+20 pies/sec each)

float prestigeStars = 0;   // Earned by resetting the game
float pendingPies = 0.0f;  // Tracks fractional pies between updates

// Upgrades - each modifies building output
struct Upgrade {
    std::string name;      // Display name (e.g., "Better Ovens")
    int cost;             // Pie cost to purchase
    bool purchased;       // Tracks if player owns this
    std::function<void()> effect; // What happens when bought
};

std::vector<Upgrade> upgrades = {
    {"Better Ovens", 500, false, [] { piesPerSecond += grandmas; }}, // Double grandma output
    {"Industrial Wheat", 1000, false, [] { piesPerSecond += bakeries * 2; }}, // Triple bakeries
    {"Robot Bakers", 5000, false, [] { piesPerSecond += factories * 3; }} // Quadruple factories
};

// ========================
// CONSOLE CONTROL - Functions to manage the text display
// ========================
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Windows console handle

// Hides the blinking cursor for cleaner display
void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// Moves cursor to specific position (x,y) on screen
void setCursorPos(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(hConsole, coord);
}

// ========================
// INPUT HANDLING - Detects key presses
// ========================
bool keyPressed(int key) {
    // Static variables remember values between function calls
    static bool keyStates[256] = { false }; // Tracks all keyboard keys
    
    // Check current key state (0x8000 means key is down)
    bool currentState = (GetAsyncKeyState(key) & 0x8000);
    
    // Only return true on NEW presses (not held keys)
    if (currentState && !keyStates[key]) {
        keyStates[key] = true;
        return true;
    } 
    // Reset when key is released
    else if (!currentState) {
        keyStates[key] = false;
    }
    return false;
}

// ========================
// BUILDING FUNCTIONS - Logic for purchasing producers
// ========================
void buyGrandma() {
    int cost = 10 + (grandmas * 2); // Each grandma costs 2 more than the last
    if (totalPies >= cost) {
        totalPies -= cost;
        grandmas++;
        piesPerSecond += 1; // Each grandma produces +1 pie/sec
        
        // Apply upgrade effects if purchased
        if (upgrades[0].purchased) piesPerSecond += 1; // Better Ovens doubles output
    }
}

void buyBakery() {
    int cost = 50 + (bakeries * 10);
    if (totalPies >= cost) {
        totalPies -= cost;
        bakeries++;
        piesPerSecond += 5; // Base production
        
        if (upgrades[1].purchased) piesPerSecond += 10; // Industrial Wheat triples output
    }
}

void buyFactory() {
    int cost = 200 + (factories * 50);
    if (totalPies >= cost) {
        totalPies -= cost;
        factories++;
        piesPerSecond += 20; // Base production
        
        if (upgrades[2].purchased) piesPerSecond += 60; // Robot Bakers quadruples output
    }
}

// ========================
// PRESTIGE SYSTEM - Reset progress for permanent boosts
// ========================
void resetForPrestige() {
    // Calculate stars earned (square root of pies/1000)
    float starsGained = sqrt(totalPies / 1000.0f);
    prestigeStars += starsGained;
    
    // Reset all progress (except stars)
    totalPies = 0;
    piesPerSecond = 0;
    pendingPies = 0.0f;
    grandmas = bakeries = factories = 0;
    
    // Keep upgrades after reset (optional design choice)
    // for (auto& upgrade : upgrades) upgrade.purchased = false;
}

// ========================
// RENDERING - Build and display the game screen
// ========================
const int SCREEN_HEIGHT = 25; // Number of text lines in console
std::vector<std::string> screenBuffer(SCREEN_HEIGHT); // Stores all text before displaying

// Constructs the current frame in memory
void buildFrame() {
    // Clear previous frame
    for (auto& line : screenBuffer) line.clear();
    
    // Header
    screenBuffer[0] = "=== PIE MAKER IDLE ===";
    
    // Core stats
    screenBuffer[1] = "Pies: " + std::to_string(totalPies);
    screenBuffer[2] = "Per second: " + std::to_string(piesPerSecond);
    screenBuffer[3] = "Prestige Stars: " + std::to_string(prestigeStars);
    
    // Controls
    screenBuffer[5] = "[SPACE] Bake a pie!";
    
    // Buildings menu
    screenBuffer[7] = "[G] Grandmas: " + std::to_string(grandmas) + 
                     " (Cost: " + std::to_string(10 + grandmas * 2) + ")";
    screenBuffer[8] = "[B] Bakeries: " + std::to_string(bakeries) + 
                     " (Cost: " + std::to_string(50 + bakeries * 10) + ")";
    screenBuffer[9] = "[F] Factories: " + std::to_string(factories) + 
                     " (Cost: " + std::to_string(200 + factories * 50) + ")";
    
    // Upgrades menu
    for (int i = 0; i < upgrades.size(); i++) {
        if (!upgrades[i].purchased && totalPies >= upgrades[i].cost / 2) {
            screenBuffer[11 + i] = "[" + std::to_string(i + 1) + "] " + upgrades[i].name + 
                                 " (" + std::to_string(upgrades[i].cost) + " pies)";
        }
    }
    
    // Prestige option
    if (totalPies >= 1000) {
        screenBuffer[15] = "[R] RESET for " + 
                          std::to_string(sqrt(totalPies / 1000.0f)) + 
                          " prestige stars!";
    }
}

// Displays the prepared frame
void renderFrame() {
    setCursorPos(0, 0); // Start at top-left corner
    
    // Print each line, padding with spaces to overwrite old text
    for (const auto& line : screenBuffer) {
        std::cout << line << std::string(80 - line.length(), ' ') << "\n";
    }
}

// ========================
// MAIN GAME LOOP
// ========================
int main() {
    // Initialize console
    hideCursor();
    system("cls"); // Clear screen
    
    // Timing variables
    auto lastTime = std::chrono::steady_clock::now();
    const float MS_PER_TICK = 1000.0f / 10; // 10 updates per second
    
    while (true) { // Infinite game loop
        // --- INPUT HANDLING ---
        if (keyPressed(VK_SPACE)) totalPies++; // Manual click
        
        // Building purchases
        if (keyPressed('G')) buyGrandma();
        if (keyPressed('B')) buyBakery();
        if (keyPressed('F')) buyFactory();
        
        // Upgrade purchases
        for (int i = 0; i < upgrades.size(); i++) {
            if (keyPressed('1' + i) && !upgrades[i].purchased && 
                totalPies >= upgrades[i].cost) {
                totalPies -= upgrades[i].cost;
                upgrades[i].purchased = true;
                upgrades[i].effect(); // Apply upgrade effect
            }
        }
        
        // Prestige reset
        if (totalPies >= 1000 && keyPressed('R')) resetForPrestige();
        
        // --- GAME UPDATES ---
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count() * 1000.0f;
        lastTime = now;
        
        // Accumulate fractional pies
        pendingPies += (piesPerSecond * deltaTime / 1000.0f);
        if (pendingPies >= 1.0f) {
            int piesToAdd = static_cast<int>(pendingPies);
            totalPies += piesToAdd;
            pendingPies -= piesToAdd;
        }
        
        // --- RENDERING ---
        buildFrame();    // Prepare screen in memory
        renderFrame();   // Display it
        Sleep(10);      // Prevent CPU overuse
    }
    
    return 0;
}