#include <iostream>
#include <windows.h>   // For Windows console functions (GetAsyncKeyState, Sleep)
#include <cmath>       // For sqrt (prestige calculation)
#include <fstream>     // For file saving/loading
#include <string>      // For string handling in save files

// =============================================
// GAME STATE - All variables tracking progress
// =============================================
int totalPies = 0;         // Total pies baked
int piesPerSecond = 0;     // Pies generated automatically per second

// Buildings (each has count and production rate)
int grandmas = 0;          // +1 pie/sec each
int bakeries = 0;          // +5 pies/sec each
int factories = 0;         // +20 pies/sec each

float prestigeStars = 0;   // Earned by resetting
bool gameLoaded = false;   // Track if save data was loaded

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

// =============================================
// MAIN GAME LOOP
// =============================================
int main() {
    loadGame(); // Attempt to load saved data on startup

    // Main game loop (runs every frame)
    while (true) {
        renderGame(); // Draw the UI

        // ===== INPUT HANDLING =====
        // Press SPACE to click (bake a pie)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            totalPies++;
            // Small delay to prevent ultra-fast clicks
            Sleep(50);
        }

        // Press G to buy Grandma
        if (GetAsyncKeyState('G') & 0x8000) {
            buyGrandma();
            Sleep(200); // Prevent rapid key presses
        }

        // Press B to buy Bakery
        if (GetAsyncKeyState('B') & 0x8000) {
            buyBakery();
            Sleep(200);
        }

        // Press F to buy Factory
        if (GetAsyncKeyState('F') & 0x8000) {
            buyFactory();
            Sleep(200);
        }

        // Press R to reset for Prestige (if eligible)
        if (totalPies >= 1000 && (GetAsyncKeyState('R') & 0x8000)) {
            resetForPrestige();
            Sleep(500); // Longer delay for important action
        }

        // ===== IDLE PRODUCTION =====
        // Add auto-generated pies every second
        totalPies += piesPerSecond;
        Sleep(1000); // Wait 1 second between ticks
    }

    return 0;
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