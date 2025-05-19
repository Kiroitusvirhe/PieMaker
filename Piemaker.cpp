#include <iostream>
#include <windows.h>
#include <cmath>
#include <fstream>
#include <chrono> // For precise timing

// ========================
// Game State
// ========================
int totalPies = 0;
int piesPerSecond = 0;
int grandmas = 0, bakeries = 0, factories = 0;
float prestigeStars = 0;
bool gameLoaded = false; // Keep this for save/load

// ========================
// Timing Variables
// ========================
const int TICKS_PER_SECOND = 10; // How often game logic updates
const float MS_PER_TICK = 1000.0f / TICKS_PER_SECOND;

// ========================
// Input Buffering
// ========================
bool keyPressed(char key) {
    // Returns TRUE only on NEW key press (not held down)
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

// =============================================
// FUNCTION DECLARATIONS
// =============================================
void buyGrandma();
void buyBakery();
void buyFactory();
void resetForPrestige();
void saveGame();
void loadGame();
void renderGame();

// ========================
// Game Loop (Optimized)
// ========================
int main() {
    loadGame();

    auto lastTime = std::chrono::steady_clock::now();
    float accumulatedTime = 0.0f;

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(currentTime - lastTime).count() * 1000.0f;
        lastTime = currentTime;
        accumulatedTime += elapsedTime;

        // ===== INPUT HANDLING (No delay!) =====
        if (keyPressed(VK_SPACE)) totalPies++; // Click
        if (keyPressed('G')) buyGrandma();
        if (keyPressed('B')) buyBakery();
        if (keyPressed('F')) buyFactory();
        if (totalPies >= 1000 && keyPressed('R')) resetForPrestige();

        // ===== FIXED UPDATE (Game logic at consistent rate) =====
        while (accumulatedTime >= MS_PER_TICK) {
            // Auto-production (scaled per tick)
            totalPies += (piesPerSecond / TICKS_PER_SECOND);
            accumulatedTime -= MS_PER_TICK;
        }

        // ===== RENDER (As fast as possible) =====
        renderGame();

        // Small sleep to prevent CPU overuse
        Sleep(1);
    }
}

// =============================================
// BUILDING PURCHASE FUNCTIONS
// =============================================
void buyGrandma() {
    int cost = 10 + (grandmas * 2); // Scaling cost
    if (totalPies >= cost) {
        totalPies -= cost;
        grandmas++;
        piesPerSecond += 1; // Each grandma gives +1 pie/sec
    }
}

void buyBakery() {
    int cost = 50 + (bakeries * 10);
    if (totalPies >= cost) {
        totalPies -= cost;
        bakeries++;
        piesPerSecond += 5; // Each bakery gives +5 pies/sec
    }
}

void buyFactory() {
    int cost = 200 + (factories * 50);
    if (totalPies >= cost) {
        totalPies -= cost;
        factories++;
        piesPerSecond += 20; // Each factory gives +20 pies/sec
    }
}

// =============================================
// PRESTIGE SYSTEM
// =============================================
void resetForPrestige() {
    // Calculate stars gained (square root of pies/1000)
    float starsGained = sqrt(totalPies / 1000.0f);
    prestigeStars += starsGained;

    // Reset all progress (except stars)
    totalPies = 0;
    piesPerSecond = 0;
    grandmas = 0;
    bakeries = 0;
    factories = 0;

    // Optional: Apply prestige bonuses here later
}

// =============================================
// SAVE/LOAD SYSTEM
// =============================================
void saveGame() {
    std::ofstream saveFile("piesave.txt");
    if (saveFile.is_open()) {
        saveFile << totalPies << "\n";
        saveFile << piesPerSecond << "\n";
        saveFile << grandmas << "\n";
        saveFile << bakeries << "\n";
        saveFile << factories << "\n";
        saveFile << prestigeStars << "\n";
        saveFile.close();
    }
}

void loadGame() {
    std::ifstream saveFile("piesave.txt");
    if (saveFile.is_open()) {
        saveFile >> totalPies;
        saveFile >> piesPerSecond;
        saveFile >> grandmas;
        saveFile >> bakeries;
        saveFile >> factories;
        saveFile >> prestigeStars;
        saveFile.close();
        gameLoaded = true;
    }
}

// =============================================
// RENDERING (UI DRAWING)
// =============================================
void renderGame() {
    system("cls"); // Clear screen (Windows only)

    // Header
    std::cout << "=== PIE MAKER IDLE ===\n";
    if (gameLoaded) std::cout << "(Loaded saved game)\n";

    // Main stats
    std::cout << "\nPies: " << totalPies << "\n";
    std::cout << "Per second: " << piesPerSecond << "\n";
    std::cout << "Prestige Stars: " << prestigeStars << "\n\n";

    // Buildings menu
    std::cout << "[SPACE] Bake a pie!\n\n";
    std::cout << "[G] Grandmas: " << grandmas << " (Cost: " << 10 + (grandmas * 2) << " pies)\n";
    std::cout << "[B] Bakeries: " << bakeries << " (Cost: " << 50 + (bakeries * 10) << " pies)\n";
    std::cout << "[F] Factories: " << factories << " (Cost: " << 200 + (factories * 50) << " pies)\n";

    // Prestige option (only shows if eligible)
    if (totalPies >= 1000) {
        float stars = sqrt(totalPies / 1000.0f);
        std::cout << "\n[R] RESET for " << stars << " prestige stars!\n";
    }

    // Save reminder
    std::cout << "\nGame auto-saves when closed.\n";
}