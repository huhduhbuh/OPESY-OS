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
#include <sstream>
#include <deque>
#include <cmath>
#include <mutex>
#include <filesystem>

//#include <ncurses.h> //for mac
//#include <unistd.h> // for mac
using namespace std;

namespace fs = std::filesystem;

void clearDirectory(const std::string& path) {
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path()); 
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void mainMenu();

int cpu_cycles = 0;
int num_cpu = 0;
std::string scheduler;
int quantum_cycles = 0;
int batch_process_freq = 0;
int min_ins = 0;
int max_ins = 0;
int delay_per_exec = 0;
int max_overall_mem = 0;
int mem_per_frame = 0;
int min_mem_per_proc = 0;
int max_mem_per_proc = 0;
int initialized = 0;
int pid = 0;
int min_exp;
int max_exp;
mutex mtx;

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
    int pid;
    string processName;
    int currentLine;
    int totalLines;
    string timeStamp;
    int core;
    int mem;
    int pages;
};

// struct for each core process 
struct CoreProcess {
    ProcessScreen process;  // current process the cpu is handling
    int flagCounter;        // > 0 means cpu is executing something
};

// struct for the memory block
struct MemoryBlock {
    int start; // holds starting address of memory block
    int end; // holds ending address of memory block
    int pid; // process id that is using the memory block
    int mem; // how much mem is in memory block
    int age;
};

deque<MemoryBlock> freeMem; // vector to hold all free memory blocks
deque<MemoryBlock> takenMem; // vector to hold all taken memory blocks
bool flat = false;


// compare by pid
bool compareByPID(pair<string, ProcessScreen> a, pair<string, ProcessScreen> b) {
    return a.second.pid < b.second.pid;
}

// function to get local time stamp
string getTimeStamp() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%m/%d/%Y, %I:%M:%S %p", ltm);

    return string(buffer);
}

deque<ProcessScreen> scheduleQueue;
map<string, ProcessScreen> processScreens; // map used for storing process screens, uses process name as key
string currentScreen = "";
vector<CoreProcess> coreProcesses; // for scheduler to keep track of what each core is doing
int generating = false; // generating dummy processes

void clearScreen() {
    clear();
    refresh();
}

ProcessScreen getProcByPid(int pid) {
    for (auto& [key, screen] : processScreens) { 
        if (screen.pid == pid) {
            return screen;
        }
    }
    ProcessScreen p;
    return p; 
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

void processSMI(ProcessScreen& ps) {
    if (ps.currentLine < ps.totalLines) {
        printw("\nProcess: %s\n", ps.processName.c_str());
        printw("ID: %d\n\n", ps.pid);
        printw("Current instruction line: %d\n", ps.currentLine);
        printw("Lines of code: %d\n\n", ps.totalLines);
    }
    else if (ps.currentLine == ps.totalLines) {
        printw("\nProcess: %s\n", ps.processName.c_str());
        printw("ID: %d\n\n", ps.pid);
        printw("Finished!\n\n");
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
        wgetnstr(stdscr, buffer, sizeof(buffer) - 1);
        input = buffer;

        if (input == "exit") {
            currentScreen = "";
            printHeader();
            break;
        }
        else if (input == "process-smi") {
            processSMI(ps);
        }
        else {
            printw("Command not recognized. Please try again.\n");
        }
    }
}

// function for each CORE
void core(int cpu) {
    int numActualExecs = 0; // how many times cpu was actually able to process smth
    while (true) {
        if (ceil(cpu_cycles / (float)(delay_per_exec + 1)) >= numActualExecs) {
            // sync with cpu_cycles
            numActualExecs++;

            if (coreProcesses[cpu].flagCounter > 0) {
                // update both scheduleQueue and processScreens
                coreProcesses[cpu].process.currentLine += quantum_cycles;
                processScreens[coreProcesses[cpu].process.processName].currentLine += quantum_cycles;

                // only proper executions will count towards quantum slice counter
                if (scheduler == "rr") {
                    coreProcesses[cpu].flagCounter = 0;
                }

                // check if complete
                if (coreProcesses[cpu].process.currentLine == coreProcesses[cpu].process.totalLines) {
                    coreProcesses[cpu].flagCounter = 0;
                }
            }
        }
    }
}


// loads config.txt values onto the global variables
void initializeProgram(const std::string& filename) {
    std::ifstream configFile(filename);

    if (!configFile) {
        std::cerr << "Unable to open " << filename << " file" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        std::istringstream iss(line);
        std::string key;

        if (iss >> key) {
            if (key == "num-cpu") {
                iss >> num_cpu;
            }
            else if (key == "scheduler") {
                iss >> scheduler;
                // Remove the surrounding double quotes from the scheduler
                if (!scheduler.empty() && scheduler[0] == '"') {
                    scheduler = scheduler.substr(1, scheduler.size() - 2);
                }
            }
            else if (key == "quantum-cycles") {
                iss >> quantum_cycles;
            }
            else if (key == "batch-process-freq") {
                iss >> batch_process_freq;
            }
            else if (key == "min-ins") {
                iss >> min_ins;
            }
            else if (key == "max-ins") {
                iss >> max_ins;
            }
            else if (key == "delay-per-exec") {
                iss >> delay_per_exec;
            }
            else if (key == "max-overall-mem") {
                iss >> max_overall_mem;
            }
            else if (key == "mem-per-frame") {
                iss >> mem_per_frame;
            }
            else if (key == "min-mem-per-proc") {
                iss >> min_mem_per_proc;
            } 
            else if (key == "max-mem-per-proc") {
                iss >> max_mem_per_proc;
            }
        }
    }

    configFile.close();
    initialized = 1;
    min_exp = log2(min_mem_per_proc);
    max_exp = log2(max_mem_per_proc);
    if (max_overall_mem == mem_per_frame) {
        flat = true;
        MemoryBlock m = {0, max_overall_mem-1, -1, max_overall_mem, 0};
        freeMem.push_back(m);
    } else {
        flat = false;
    }
    clearDirectory("./backing_store");
}



// add item to backing store
ProcessScreen BSStore(int pid) {
    ProcessScreen p;
    return p;
}

// remove and return process in backing store
ProcessScreen BSRetrieve(int pid) {
    ProcessScreen p;
    return p;
}

// merge blocks in freeMem
void mergeAdjacentBlocks() {
    if (freeMem.empty()) return;

    std::deque<MemoryBlock> mergedBlocks;
    mergedBlocks.push_back(freeMem[0]);

    for (size_t i = 1; i < freeMem.size(); ++i) {
        MemoryBlock& prev = mergedBlocks.back();
        const MemoryBlock& current = freeMem[i];

        if (prev.end + 1 == current.start) {
            prev.end = current.end;
        } else {
            mergedBlocks.push_back(current);
        }
    }
    freeMem = std::move(mergedBlocks);
}

auto compByAddr = [] (MemoryBlock a, MemoryBlock b) {
    return a.start < b.start;
};

// allocation algo for flat 
// search free mem for space, if available, alloc and ret 1, else 0
bool AllocateFlat(ProcessScreen p) {
    for (long long unsigned int i = 0; i < freeMem.size(); i++) {
        MemoryBlock m = freeMem[i];
        if (m.mem >= p.mem) {
            MemoryBlock newTakenBlock = {m.start, m.start+p.mem - 1, p.pid, p.mem, 0};
            MemoryBlock leftoverBlock = {m.start+p.mem, m.end, -1, m.mem-p.mem, 0};
            freeMem.erase(freeMem.begin() + i);
            takenMem.push_back(newTakenBlock);
            freeMem.push_back(leftoverBlock);
            sort(freeMem.begin(), freeMem.end(), compByAddr);
            mergeAdjacentBlocks();
            return 1;
        }
    }
    return 0;
}

// return 1 if proc in main mem
int FlatMemAlloc(ProcessScreen process) {
    // check if proc in mem
    for (const MemoryBlock& m : takenMem ) {
        if (m.pid == process.pid) {
            return true;
        }
    }
    /*// check if in backing store
    ProcessScreen p = BSRetrieve(pid);
    if (p.processName == "") {
        // if not, gen new 
        p = process;
    }*/
    ProcessScreen p = process;
    // try allocating mem, if cant, swap out oldest
    while(!AllocateFlat(p)){
        // remove from takenMem
        MemoryBlock m_oldest = takenMem.front();
        takenMem.pop_front();
        // add to freeMem
        m_oldest.age = 0;
        m_oldest.pid = -1;
        freeMem.push_back(m_oldest);
        sort(freeMem.begin(), freeMem.end(), compByAddr);
        mergeAdjacentBlocks();
        // remove from backing store
        ProcessScreen p_oldest = getProcByPid(pid);
        BSStore(p_oldest.pid);
    }

    return true;
}

// return 1 if all pages are in main mem
int PagingAlloc() {
    return 0;
}


void RRScheduler() {
    for (int i = 0; i < num_cpu; i++) {
        if (coreProcesses[i].flagCounter == 0) {
            // if process is not completed, add back to ready/waiting queue
            if (coreProcesses[i].process.processName != "" && coreProcesses[i].process.currentLine != coreProcesses[i].process.totalLines) {
                scheduleQueue.push_back(coreProcesses[i].process);
          //      deallocateMemory(coreProcesses[i].process.pid);
            }

            // update assigned core on queues
            if (!scheduleQueue.empty()) {
                ProcessScreen p = scheduleQueue.front();
                scheduleQueue.pop_front();
                if ((flat == 1 && FlatMemAlloc(p)) || (flat == 0 && PagingAlloc())) {
                    coreProcesses[i].process = p;
                    coreProcesses[i].process.core = i;
                    processScreens[coreProcesses[i].process.processName].core = i;
                    coreProcesses[i].flagCounter = quantum_cycles;      
                } else {
                    scheduleQueue.push_back(p);
                }
            }
        } 
    }
}

void FCFSScheduler() {
    for (int i = 0; i < num_cpu; i++) {
        if (coreProcesses[i].flagCounter == 0 && !scheduleQueue.empty()) {
            ProcessScreen p = scheduleQueue.front();
            scheduleQueue.pop_front();
            if ((flat == 1 && FlatMemAlloc(p)) || (flat == 0 && PagingAlloc())) {
                coreProcesses[i].process = p;
                coreProcesses[i].process.core = i;
                processScreens[coreProcesses[i].process.processName].core = i;
                coreProcesses[i].flagCounter = quantum_cycles;      
            } else {
                scheduleQueue.push_back(p);
            }
        }
    }
}


void startClock() {
    for (int i = 0; i < num_cpu; i++) {
        thread t(core, i);
        t.detach();
        CoreProcess cp;
        cp.flagCounter = 0;
        coreProcesses.push_back(cp);
    }

    while (true) {
        cpu_cycles++;
        if (scheduler == "fcfs") {
            FCFSScheduler();
        }
        else {
            RRScheduler();
        }

        if (generating == true && batch_process_freq != 0 && cpu_cycles % batch_process_freq == 0) {
            string proposedName = "p" + to_string(pid);
            // in case user uses screen -s with the same name
            while (processScreens.find(proposedName) != processScreens.end()) {
                pid++;
                proposedName = "p" + to_string(pid);
            }
            int M = pow(2, rand() % (max_exp - min_exp + 1) + min_exp);
            ProcessScreen newScreen = { pid, proposedName, 0, rand() % (max_ins - min_ins + 1) + min_ins, getTimeStamp(), -1, M, M/mem_per_frame};
            pid++;
            processScreens[proposedName] = newScreen;
            scheduleQueue.push_back(newScreen);
        }
        napms(100); // sleep, milliseconds
    }
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

        wgetnstr(stdscr, buffer, sizeof(buffer) - 1);
        input = buffer;

        if (initialized == 0 && (input == "initialize" || input == "exit")) {
            if (input == "initialize") {
                initializeProgram("config.txt");
                printw("Program initialized. Obtained data from config.txt.\n");
                thread t(startClock);
                t.detach();
            }
            else {
                run = false;
            }
        }
        else if (initialized == 0 && input != "initialize") {
            printw("Command is invalid. Please initialize the program first.\n");
        }
        else {
            if (input == "initialize") {
                initializeProgram("config.txt");
                printw("Program initialized. Obtained data from config.txt.\n");
            }
            else if (input.find("screen -s") == 0) {
                string processName = trim(input.substr(9));
                if (processName == "") {
                    printw("Can't have a blank process name.\n");
                } else if (processScreens.find(processName) == processScreens.end()) {
                    int M = pow(2, rand() % (max_exp - min_exp + 1) + min_exp);
                    ProcessScreen newScreen = { pid, processName, 0, rand() % (max_ins - min_ins + 1) + min_ins, getTimeStamp(), -1, M, M/mem_per_frame };
                    pid++;
                    processScreens[processName] = newScreen;
                    scheduleQueue.push_back(newScreen);
                    currentScreen = processName;
                    displayScreen(processName);
                } else {
                    printw("Process %s already exists.\n", processName.c_str());
                }
            }
            else if (input.find("screen -r") == 0) {
                string processName = trim(input.substr(9));
                ProcessScreen& ps = processScreens[processName];
                if (processScreens.find(processName) != processScreens.end() && ps.currentLine != ps.totalLines) {
                    currentScreen = processName;
                    displayScreen(processName);
                }
                else if (ps.currentLine == ps.totalLines) {
                    printw("Process '%s' has already finished.\n", processName.c_str());
                    currentScreen = "";
                }
                else {
                    printw("Process '%s' not found.\n", processName.c_str());
                    currentScreen = "";
                }
            }
            else if (input.find("screen -ls") == 0) {
                string formatTime;
                ProcessScreen p;

                int active_cores = 0;
                //int unfinishedProcesses = scheduleQueue.size();
                /*
                for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].process.processName != "" && coreProcesses[i].process.currentLine != coreProcesses[i].process.totalLines) {
                        unfinishedProcesses++;
                    }
                } */

                for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].flagCounter > 0) {
                        active_cores++;
                    }
                }
                //active_cores = unfinishedProcesses < num_cpu ? unfinishedProcesses : num_cpu;
                float utilization = (active_cores / (float)num_cpu) * 100;
                
                printw("CPU utilization: %3.2f%%\n", utilization);
                printw("Cores used: %d\n", active_cores);
                printw("Cores available: %d", num_cpu - active_cores);
                printw("\n\n--------------------------------------");

                printw("\nRunning processes: \n");
                for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].process.processName != "" && coreProcesses[i].process.currentLine != coreProcesses[i].process.totalLines) {
                        p = coreProcesses[i].process;
                        formatTime = p.timeStamp;
                        formatTime.erase(10, 1);
                        printw("%s\t(%s)\tCore: %d\t\t%d / %d\n", p.processName.c_str(), formatTime.c_str(), p.core, p.currentLine, p.totalLines);
                    }
                }
                printw("\nFinished processes: \n");
                vector<pair<string, ProcessScreen>> sortedMap;

                for (const auto& it : processScreens) {
                    sortedMap.push_back(it);
                }
                sort(sortedMap.begin(), sortedMap.end(), compareByPID);
                for (auto& it : sortedMap) {
                    p = it.second;
                    if (p.currentLine == p.totalLines) {
                        formatTime = p.timeStamp;
                        formatTime.erase(10, 1);
                        printw("%s\t(%s)\tCore: %d\t\t%d / %d\n", p.processName.c_str(), formatTime.c_str(), p.core, p.currentLine, p.totalLines);
                    }
                }

                printw("\n--------------------------------------\n\n");
            }
            else if (input == "scheduler-test") {
                if (generating == true) {
                    printw("Already generating dummy processes...\n");
                }
                else {
                    generating = true;
                    printw("Generating dummy processes...\n");
                }
            }
            else if (input == "scheduler-stop") {
                if (generating == false) {
                    printw("Already stopped generating dummy processes...\n");
                }
                else {
                    generating = false;
                    printw("Stopped generating dummy processes...\n");
                }
            }
            else if (input == "report-util") {
                string formatTime;
                ProcessScreen p;

                int active_cores = 0;
                //int unfinishedProcesses = scheduleQueue.size();
                /*
                for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].process.processName != "" && coreProcesses[i].process.currentLine != coreProcesses[i].process.totalLines) {
                        unfinishedProcesses++;
                    }
                }
                */

                 for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].flagCounter > 0) {
                        active_cores++;
                    }
                }
                // active_cores = unfinishedProcesses < num_cpu ? unfinishedProcesses : num_cpu;
                float utilization = (active_cores / (float)num_cpu) * 100;

                std::ofstream logFile("csopesy-log.txt", std::ios::out);

                logFile << "CPU utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
                logFile << "Cores used: " << active_cores << "\n";
                logFile << "Cores available: " << num_cpu - active_cores << "\n";
                logFile << "\n--------------------------------------\n";

                logFile << "Running processes: \n";
                for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].process.processName != "" && coreProcesses[i].process.currentLine != coreProcesses[i].process.totalLines) {
                        p = coreProcesses[i].process;
                        formatTime = p.timeStamp;
                        formatTime.erase(10, 1);
                        logFile << p.processName << "\t(" << formatTime << ")\tCore: " << p.core
                            << "\t\t" << p.currentLine << " / " << p.totalLines << "\n";
                    }
                }

                logFile << "\nFinished processes: \n";
                vector<pair<string, ProcessScreen>> sortedMap;
            
                for (const auto& it : processScreens) {
                    sortedMap.push_back(it);
                }
                sort(sortedMap.begin(), sortedMap.end(), compareByPID);
                for (auto& it : sortedMap) {
                    p = it.second;
                    if (p.currentLine == p.totalLines) {
                        formatTime = p.timeStamp;
                        formatTime.erase(10, 1);
                        logFile << p.processName << "\t(" << p.timeStamp << ")\tCore: " << p.core
                            << "\t\t" << p.currentLine << " / " << p.totalLines << "\n";
                    }
                }

                logFile << "\n--------------------------------------\n\n";

                logFile.close();
            }
            else if (input == "clear") {
                clearScreen();
                printHeader();
            }
            else if (input == "process-smi") {
                int active_cores = 0;
                int mem_used = 0;
                vector<ProcessScreen> running; 
                mtx.lock();
                 for (int i = 0; i < num_cpu; i++) {
                    if (coreProcesses[i].flagCounter > 0) {
                        active_cores++;
                        running.push_back(coreProcesses[i].process);
                        mem_used += coreProcesses[i].process.mem;
                    }
                }
                mtx.unlock();
                float utilization = (active_cores / (float)num_cpu) * 100;

                printw("PROCESS-SMI V01.00: \n");
                printw("CPU Util: %3.2f%%\n", utilization);
                printw("Memory Usage: %d MiB / %d MiB\n", mem_used, max_overall_mem);
                printw("Memory Util: %3.2f%%\n\n", mem_used / (float) max_overall_mem * 100);
                printw("Running processes and memory usage: \n");
                for (auto& p : running) {
                    printw("%s %d MiB\n", p.processName.c_str(), p.mem);
                }

            } else if (input == "vmstat") {

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
}

int main() {
    initscr();
    start_color();
    scrollok(stdscr, TRUE);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    cbreak();
    mainMenu();
    refresh();
    endwin();
    return 0;
}