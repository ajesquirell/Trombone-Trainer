#define OLC_PGE3_APPLICATION
#include "Game.h"

#include <exception>
#include <iostream>

int main() {
    try {
        trombone::Game game;
        olc::PGEConfig config;
        config.vScreenSize = {1120, 720};
        config.vPixelSize = {1, 1};
        config.sAppName = "Trombone Trainer";
        config.bResizeable = true;
        config.bRetainAspectRatio = false;
        if (!game.Construct(config) || !game.Start()) {
            std::cerr << "Could not start Trombone Trainer. Check the graphics display.\n";
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "Trombone Trainer: " << error.what() << '\n';
        return 1;
    }
}
