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

// for reprinting the header after clearing the screen
void printHeader() {
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

    bool run = true; // flag for running or ending loop
    string input;
    
    // loop for command line choices
    while (run) {
        getline(cin, input);

        if (input == "initialize") {
            cout << "Initialize command recognized. Doing something.\n";
            cout << "Enter a command: "; // replace with actual command logic later
        } else if (input == "screen") {
            cout << "Screen command recognized. Doing something.\n";
            cout << "Enter a command: "; // replace with actual command logic later
        } else if (input == "scheduler-test") {
            cout << "Scheduler-test command recognized. Doing something.\n";
            cout << "Enter a command: "; // replace with actual command logic later
        } else if (input == "scheduler-stop") {
            cout << "Scheduler-stop command recognized. Doing something.\n";
            cout << "Enter a command: "; // replace with actual command logic later
        } else if (input == "report-util") {
            cout << "Report-util command recognized. Doing something.\n";
            cout << "Enter a command: "; // replace with actual command logic later
        } else if (input == "clear") {
            clearScreen();
            printHeader();
        } else if (input == "exit") {
            cout << "Exit command recognized. Exiting program.\n";
            run = false;
        } else {
            cout << "Command not recognized. Please try again.\n";
            cout << "Enter a command: ";
        }
    }

    return 0;
}