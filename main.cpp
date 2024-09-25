#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <iomanip>
using namespace std;

// struct used for each new instance of a process screen
struct ProcessScreen {
    string processName;
    int currentLine;
    int totalLines;
    string timeStamp;
};

// function to get local time stamp
string getTimeStamp() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%m/%d/%Y, %I:%M:%S %p", ltm);

    return string(buffer);
}

map<string, ProcessScreen> processScreens; // map used for storing process screens, uses process name as key

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
}

// function for displaying new process screen information after screen -s is entered
// note: the const string& basically allows the process name to still be referenced for screen -r
// ^^ im assuming thought thats how it works lol
void displayScreen(const string& processName) {
    if (processScreens.find(processName) != processScreens.end()) {
        clearScreen();

        ProcessScreen& ps = processScreens[processName];
        cout << "Process: " << ps.processName << endl;
        cout << "Instructions: " << ps.currentLine << "/" << ps.totalLines << endl;
        cout << "Screen created at: " << ps.timeStamp << endl;
    } else {
        cout << "Process '" << processName << "' not found.\n";
    }
}

int main() {
    printHeader();
    bool run = true; // flag for running or ending loop
    string input;
    string currentScreen = "";
    
    // loop for command line choices
    while (run) {
        getline(cin, input);

        if (input == "initialize") {
            cout << "Initialize command recognized. Doing something.\n";
            cout << "Enter a command: "; // replace with actual command logic later
        } else if (input.find("screen -s") == 0) {
            string processName = input.substr(10);
            ProcessScreen newScreen = {processName, 0, 100, getTimeStamp()};
            processScreens[processName] = newScreen;
            currentScreen = processName;
            displayScreen(processName);
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
            if (!currentScreen.empty()) {
                currentScreen = "";
                printHeader();
            } else {
                run = false;
            }
        } else {
            cout << "Command not recognized. Please try again.\n";
            cout << "Enter a command: ";
        }
    }

    return 0;
}