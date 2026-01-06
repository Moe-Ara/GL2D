//
// Created by Mohamad on 03/12/2025.
//
#include "Game.hpp"
#include <iostream>
#include <exception>
#include "Exceptions/Gl2DException.hpp"

int main(){
    try {
        Game the_lost_heroin=Game();
        the_lost_heroin.setup();
        the_lost_heroin.update();
        the_lost_heroin.exit();
        return 0;
    } catch (const Engine::GL2DException &ex) {
        std::cerr << "Engine error: " << ex.what() << std::endl;
        return 3;
    } catch (const std::exception &ex) {
        std::cerr << "Unhandled error: " << ex.what() << std::endl;
        return 3;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 3;
    }
}
