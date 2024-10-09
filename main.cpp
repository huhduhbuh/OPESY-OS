#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <iomanip>
#include <curses.h>
#include <windows.h> // for windows
#include <vector> 
#include <algorithm>
#include <cctype> 
#include <thread>
#include <fstream>
//#include <ncurses.h> //for mac
//#include <unistd.h> // for mac
using namespace std;

void mainMenu(); 

// trim from the start (left)
string ltrim(const string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    return (start == string::npos) ? "" : str.substr(start);
}

// trim from the end (right)
string rtrim(const string& str) {
    size_t end = str.find_last_not_of(" \t\n\r");
    return (end == string::npos) ? "" : str.substr(0, end + 1);
}

// trim from both ends 
string trim(const string& str) {
    return rtrim(ltrim(str));
}

// struct used for each new instance of a process screen
struct ProcessScreen {
    string processName;
    int currentLine;
    int totalLines;
    string timeStamp;
    int core;
};

// function to get local time stamp
string getTimeStamp() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%m/%d/%Y, %I:%M:%S %p", ltm);

    return string(buffer);
}

vector<ProcessScreen> scheduleQueue;
map<string, ProcessScreen> processScreens; // map used for storing process screens, uses process name as key
string currentScreen = "";
vector<int> coreLoads {0, 0, 0, 0}; // index 0 = core 0
int stop = false; // stop executing processes

void clearScreen() {
    clear();
    refresh();
}

// for reprinting the header after clearing the screen
void printHeader() {
    clearScreen();
    printw("\nHello, Welcome to CSOPESY commandline!\n");
    printw(R"(  ____ ____   ___  ____  _____ ______   __
 / ___/ ___| / _ \|  _ \| ____/ ___\ \ / /
| |   \___ \| | | | |_) |  _| \___ \\ V / 
| |___ ___) | |_| |  __/| |___ ___) || |  
 \____|____/ \___/|_|   |_____|____/ |_|)");

    attron(COLOR_PAIR(3));
    printw("\nHello, Welcome to CSOPESY commandline!\n");
    attron(COLOR_PAIR(2));
    printw("Type 'exit' to quit, 'clear' to clear the screen\n");
    attron(COLOR_PAIR(1));
}

void logPrint(ProcessScreen& ps, const string& message) {
    ofstream logFile;
    string filePath = "/Users/diego/Downloads/CSOPESY/OPESY-OS/" + ps.processName + "_log.txt"; // Change to your own file path
    logFile.open(filePath, ios::app); // append mode

    if (logFile.is_open()) {
        string timeStamp = getTimeStamp();
        logFile << "(" << timeStamp << ") Core:" << ps.core << " \"" << message << "\"" << endl;
        logFile.close();
        printw("Logged: %s\n", message.c_str());
    } else {
        printw("Error: Unable to open log file.\n");
    }
}

// function for displaying new process screen information after screen -s is entered
// note: the const string& basically allows the process name to still be referenced for screen -r
void displayScreen(const string& processName) {
        clearScreen();

        ProcessScreen& ps = processScreens[processName];
        printw("Process: %s\n", ps.processName.c_str());
        printw("Instructions: %d/%d\n", ps.currentLine, ps.totalLines);
        printw("Screen created at: %s\n", ps.timeStamp.c_str());

    string input;
    char buffer[100];
    while (true) {
        printw("Enter a command: ");
        refresh();
        wgetnstr(stdscr, buffer, sizeof(buffer)-1);
        input = buffer;
        if (input == "exit") {
            currentScreen = "";
            printHeader();
            break;
        } else if (input.find("print") == 0) {
            string message = input.substr(6);
            logPrint(ps, message);
        } else {
            printw("Command not recognized. Please try again.\n");
        }
    }
}

// each cpu = 1 thread, function for each cpu/core
void executeProcess(int cpu) {
    long long unsigned int schedCounter = 0;
    int executing = false;
    while (stop == false) {
        // look for process to execute
        if (executing == false) {
            while (schedCounter < scheduleQueue.size()) {
                schedCounter++;
                if (scheduleQueue[schedCounter-1].core == cpu) {
                    executing = true;
                    break;
                }
            }
        } else { // increment if cpu has a process to execute
            scheduleQueue[schedCounter-1].currentLine++;
        }
        // check for completion
        if (executing == true && scheduleQueue[schedCounter-1].currentLine == scheduleQueue[schedCounter-1].totalLines) {
            executing = false;
        }
        Sleep(100); //for windows
        //usleep(100); // for mac
    }
}

// assign core to a process
int assignCore(){
    auto minElementIt = min_element(begin(coreLoads), end(coreLoads));

    // calculate the index by subtracting the iterator from coreLoads iterator
    int index = distance(begin(coreLoads), minElementIt);
    coreLoads[index] += 1;
    return index;
}


void mainMenu() {
    printHeader();
    bool run = true; // flag for running or ending loop
    char buffer[100];
    string input;

    // loop for command line choices
    while (run) {
        printw("Enter a command: ");
        refresh();

        wgetnstr(stdscr, buffer, sizeof(buffer)-1);
        input = buffer;

        if (input == "initialize") {
            printw("Initialize command recognized. Doing something.\n");
            printw("Enter a command: "); 
        }
        else if (input.find("screen -s") == 0) {
            string processName = input.substr(10);
            if (processScreens.find(processName) == processScreens.end()) {
            	ProcessScreen newScreen = { processName, 0, 100, getTimeStamp(), assignCore() };
	            processScreens[processName] = newScreen;
                scheduleQueue.push_back(newScreen);
	            currentScreen = processName;
	            displayScreen(processName);
			}
            else {
            	printw("Process %s already exists.\n", processName.c_str());
			}
        }
        else if (input.find("screen -r") == 0) {
            string processName = trim(input.substr(9));
            if (processScreens.find(processName) != processScreens.end()) {
                currentScreen = processName;
                displayScreen(processName);
            } else {
                printw("Process '%s' not found.\n", processName.c_str());
                currentScreen = "";
            }
        }
        else if (input.find("screen -ls") == 0) {
            string formatTime;
            ProcessScreen p;
            vector<ProcessScreen> finished;
            printw("Running processes: \n");
            for (long long unsigned int i = 0; i < scheduleQueue.size(); i++) {
                p = scheduleQueue[i];
                formatTime = p.timeStamp;
                formatTime.erase(10, 1);
                if (p.currentLine != p.totalLines)
                    printw("%s\t(%s)\tCore: %d\t\t%d / %d\n", p.processName.c_str(), formatTime.c_str(), p.core, p.currentLine, p.totalLines);
                else 
                    finished.push_back(p);
            }
            printw("Finished processes: \n");
            for (long long unsigned int i = 0; i < finished.size(); i++) {
                p = finished[i];
                printw("%s\t(%s)\tCore: %d\t\t%d / %d\n", p.processName.c_str(), formatTime.c_str(), p.core, p.currentLine, p.totalLines);
            }
        }
        else if (input == "scheduler-test") {
            for (long long unsigned int i = 0; i < coreLoads.size(); i++) {
                thread t(executeProcess, i);
                t.detach();
            }
            printw("Executing processes...\n");
            stop = false;
        }
        else if (input == "scheduler-stop") {
            stop = true;
            printw("Execution of processes stopped...\n");
        }
        else if (input == "report-util") {
            printw("Report-util command recognized. Doing something.\n");
        }
        else if (input == "clear") {
            clearScreen();
            printHeader();
        }
        else if (input == "exit") {
            run = false;
        }
        else {
            printw("Command not recognized. Please try again.\n");
        }
    }
    refresh();
}

int main() {
    initscr();
    start_color();
    scrollok(stdscr,TRUE);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    cbreak();
    mainMenu();
    refresh();
    endwin();
    return 0;
}
