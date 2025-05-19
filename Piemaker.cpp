#include <iostream>
#include <windows.h>
#include <cmath>
#include <fstream>
#include <chrono>
#include <vector>
#include <string>

// ========================
// Game State
// ========================
int totalPies = 0;
int piesPerSecond = 0;
int grandmas = 0, bakeries = 0, factories = 0;
float prestigeStars = 0;

// ========================
// Timing & Rendering
// ========================
const int TICKS_PER_SECOND = 10;
const float MS_PER_TICK = 1000.0f / TICKS_PER_SECOND;
const int SCREEN_HEIGHT = 25;
std::vector<std::string> screenBuffer(SCREEN_HEIGHT);
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// ========================
// Console Utilities
// ========================
void setCursorPos(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(hConsole, coord);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// ========================
// Input Handling
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
// Building Logic
// ========================
void buyGrandma() {
    int cost = 10 + (grandmas * 2);
    if (totalPies >= cost) {
        totalPies -= cost;
        grandmas++;
        piesPerSecond += 1;
    }
}

void buyBakery() {
    int cost = 50 + (bakeries * 10);
    if (totalPies >= cost) {
        totalPies -= cost;
        bakeries++;
        piesPerSecond += 5;
    }
}

void buyFactory() {
    int cost = 200 + (factories * 50);
    if (totalPies >= cost) {
        totalPies -= cost;
        factories++;
        piesPerSecond += 20;
    }
}

void resetForPrestige() {
    float starsGained = sqrt(totalPies / 1000.0f);
    prestigeStars += starsGained;
    totalPies = 0;
    piesPerSecond = 0;
    grandmas = bakeries = factories = 0;
}

// ========================
// Double-Buffered Rendering
// ========================
void buildFrame() {
    // Clear buffer
    for (auto& line : screenBuffer) line.clear();

    // Build UI in memory
    screenBuffer[0] = "=== PIE MAKER IDLE ===";
    screenBuffer[1] = "Pies: " + std::to_string(totalPies);
    screenBuffer[2] = "Per second: " + std::to_string(piesPerSecond);
    screenBuffer[3] = "Prestige Stars: " + std::to_string(prestigeStars);
    screenBuffer[5] = "[SPACE] Bake a pie!";
    screenBuffer[7] = "[G] Grandmas: " + std::to_string(grandmas) + " (Cost: " + std::to_string(10 + grandmas * 2) + ")";
    screenBuffer[8] = "[B] Bakeries: " + std::to_string(bakeries) + " (Cost: " + std::to_string(50 + bakeries * 10) + ")";
    screenBuffer[9] = "[F] Factories: " + std::to_string(factories) + " (Cost: " + std::to_string(200 + factories * 50) + ")";

    if (totalPies >= 1000) {
        screenBuffer[11] = "[R] RESET for " + std::to_string(sqrt(totalPies / 1000.0f)) + " stars!";
    }
}

void renderFrame() {
    setCursorPos(0, 0);
    for (const auto& line : screenBuffer) {
        std::cout << line << std::string(80 - line.length(), ' ') << "\n";
    }
}

// ========================
// Main Game Loop
// ========================
int main() {
    hideCursor();
    system("cls");

    auto lastTime = std::chrono::steady_clock::now();
    float accumulatedTime = 0.0f;

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(currentTime - lastTime).count() * 1000.0f;
        lastTime = currentTime;
        accumulatedTime += elapsedTime;

        // Input
        if (keyPressed(VK_SPACE)) totalPies++;
        if (keyPressed('G')) buyGrandma();
        if (keyPressed('B')) buyBakery();
        if (keyPressed('F')) buyFactory();
        if (totalPies >= 1000 && keyPressed('R')) resetForPrestige();

        // Fixed Update
        while (accumulatedTime >= MS_PER_TICK) {
            totalPies += (piesPerSecond / TICKS_PER_SECOND);
            accumulatedTime -= MS_PER_TICK;
        }

        // Render
        buildFrame();
        renderFrame();

        Sleep(10); // Prevent CPU overuse
    }

    return 0;
}