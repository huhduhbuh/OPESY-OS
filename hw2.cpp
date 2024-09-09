#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;

// command line compiling : 
// g++ -Wall [name].cpp -o [name].exe

void clearScreen() {
    #ifdef _WIN32
        system("cls");  // Windows
    #else
        system("clear");  // Unix/Linux/Mac
    #endif
}

// use ANSI color codes for reference: https://www.geeksforgeeks.org/how-to-change-console-color-in-cpp/
void setColor(int colorCode) {
    cout << "\033[" << colorCode << "m";
}

int main() {
    clearScreen();
    cout << R"(  ____ ____   ___  ____  _____ ______   __
 / ___/ ___| / _ \|  _ \| ____/ ___\ \ / /
| |   \___ \| | | | |_) |  _| \___ \\ V / 
| |___ ___) | |_| |  __/| |___ ___) || |  
 \____|____/ \___/|_|   |_____|____/ |_|)";

    setColor(32);
    cout << "\nHello, Welcome to CSOPESY commandline!\n";
    setColor(33);
    cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
    setColor(37);
    cout << "Enter a command: ";

    string input;
    getline(cin, input);
    cout << input;

    return 0;
}