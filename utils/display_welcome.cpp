/**
 * @file display_welcome.cpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Implementace funkce pro zobrazeni uvodniho textu a instrukci pro uzivatele
 */

#include "display_welcome.hpp"

void displayWelcome()
{
    std::cout << "===== ===== ===== Expression Parser ===== ===== =====" << std::endl;
    std::cout << "| v     v     v   v    v    v    v   v     v     v  |" << std::endl;
    std::cout << "|    =  =  =  =   Enter expression   =  =  =  =     |" << std::endl;
    std::cout << "|   =   or press enter on an empty expression   =   |" << std::endl;
    std::cout << "|  =     =         to display help         =     =  |" << std::endl;
    std::cout << "| =      =      (or exit --> ctrl+c)       =      = |" << std::endl;
    std::cout << "===== ===== ===== ===== ===== ===== ===== ===== =====" << std::endl;
    std::cout << "\nExpression: ";
}