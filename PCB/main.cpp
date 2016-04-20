//
//  main.cpp
//  PCB
//  CSCI 340 Project
//
//  Created by Dhananjay Suresh on 4/11/16.
//  Copyright © 2016 Dhananjay Suresh. All rights reserved.
//

#include <iostream>
#include <iomanip>
#include <queue>
#include <vector>
#include <regex>
#include <string>
#include <algorithm>

using namespace std;

//Enum for status of process
enum proces_status {waiting, running, terminated, io};

//Regular expressions for error checking
const regex r_int("[0-9]*");
const regex r_command("[hpPdD][1-9][0-9]*|[AtS]");
const regex r_snapshot("[rim]");
const regex r_string("[a-zA-Z_][a-zA-Z_0-9]*\\.[a-zA-Z0-9]+");

/*
 process
 
 Contains pid, priority,
 starting memory location,
 status, file name and file_size
 */
class process {
public:
    pid_t pid;
    unsigned int priority, memory_base;
    proces_status status;
    string file_name;
    string file_size;
    
};
/*
 memory_range
 
 Contains memory range base and limit, and process pid
 Comparision operator is overriden to compare starting memory address
 */
struct memory_range {
    unsigned int base, limit;
    pid_t pid;
    //Comparision function
    bool operator<(const memory_range& rhs) const {
        return this->base < rhs.base; }
    //Constructor
    memory_range(pid_t pid, unsigned int base, unsigned int limit) : pid(pid), base(base), limit(limit) { }
    
};

/*
 memory
 
 Contains memory max, free and used space
 Vector of memory allocations to keep track of memory usage
 */
class memory {
private:
    unsigned int memory_cap, free, used;
    vector<memory_range> memory_allocations;
    
public:
    //Constructor
    memory(unsigned int memory_cap) {
        this->memory_cap = memory_cap;
        this->free = memory_cap;
    }
    //Allocate memory to process
    int allocate_memory(pid_t pid, unsigned int amount) {
        
        int available_space = 0, new_available_space = 0;
        unsigned int start = 0;
        bool potential_allocation = false;
        
        //Check if there is free space
        if (amount > free)
            return -1;
        
        //Check if memory is empty
        //If memor is empty insert to front of memory
        if (memory_allocations.empty()) {
            memory_allocations.push_back(memory_range(pid, 0, start+amount-1));
            free -= amount;
            return 0;
        }
        
        //Check available space from beginning of memory to first allocation
        memory_range first = memory_allocations.front();
        new_available_space = first.base;
        if(available_space <= new_available_space && new_available_space >=amount) {
            available_space = new_available_space;
            start = 0;
            potential_allocation = true;
        }
        
        //Check available space from last allocation to end of memory
        memory_range last = memory_allocations.back();
        new_available_space = memory_cap - last.limit;
        if(available_space <= new_available_space && new_available_space >=amount) {
            available_space = new_available_space;
            start = last.limit+1;
            potential_allocation = true;
        }
        
        //Check inbetween all memory allocations for available space
        for(int i = 0; i < memory_allocations.size()-1; i+=2) {
            memory_range range1 = memory_allocations[i];
            memory_range range2 = memory_allocations[i+1];
            new_available_space = range2.base - range1.limit;
            if(available_space < new_available_space && new_available_space >=amount) {
                start = range1.limit+1;
                available_space = new_available_space;
                potential_allocation = true;
            }
        }
        //If no potential holes are found return -1 indicating cannot allocate
        if (!potential_allocation)
            return -1;
        else {
            //Push allocation and sort allocations
            memory_allocations.push_back(memory_range(pid, start, start+amount-1));
            free -= amount;
            sort(memory_allocations.begin(), memory_allocations.end());
            return start;
        }
    }
    //Deallocate memory from process
    bool deallocate_memory(pid_t pid) {
        //Find allocation with corrent pid and add amount back to available
        for (vector<memory_range>::iterator i = memory_allocations.begin(); i != memory_allocations.end(); i++) {
            if (i->pid == pid) {
                free+=(i->limit-i->base);
                memory_allocations.erase(i);
                return true;
            }
        }
        return false;
    }
    //Print out memory allocations and information
    void memory_snapshot() {
        for (vector<memory_range>::iterator i = memory_allocations.begin(); i != memory_allocations.end(); i++) {
            cout << setw(5) << left << i->pid
            << setw(5) << left << i->base
            << setw(5) << left << i->limit
            << setw(5) << left << i->limit-i->base+1 << endl;
        }
        cout << "Used Memory: " << memory_cap-free << endl;
        cout << "Free Memory: " << free << endl;
    }
};

/*
 os
 
 Contains queues for ready_queue,
 unused processes, and device queues
 Also has max system memory, pid counter, and
 process running on CPU
 */
class os {
    //Comparison function for priority queue
    struct compare_process : public binary_function<process*, process*, bool> {
        bool operator()(const process* left, const process* right) const {
            return !(left->priority < right->priority);
        }
    };
    
private:
    unsigned int memory_cap, num_printers, num_disks;
    pid_t pid_counter = 0;
    process* running_process = nullptr;
    priority_queue<process*, vector<process*>, compare_process> ready_queue;
    queue<process*> process_table;
    vector<queue<process*>> printer_queue;
    vector<queue<process*>> disk_queue;
    
    
public:
    //Deconstructor
    ~os(){
        delete running_process;
        running_process = nullptr;
        
        while(!ready_queue.empty()) {
            process *p = ready_queue.top();
            ready_queue.pop();
            delete p;
            p = nullptr;
        }
        while (!process_table.empty()) {
            process *p = process_table.front();
            process_table.pop();
            delete p;
            p = nullptr;
        }
        for(int i = 0; i < printer_queue.size(); i++) {
            queue<process*> q = printer_queue[i];
            while (!q.empty()) {
                process *p = q.front();
                q.pop();
                delete p;
                p = nullptr;
            }
        }
        for(int i = 0; i < disk_queue.size(); i++) {
            queue<process*> q = disk_queue[i];
            while (!q.empty()) {
                process *p = q.front();
                q.pop();
                delete p;
                p = nullptr;
            }
        }
    }
    //Setup up OS
    //Will not continue until recieves proper inputs
    void install() {
        const regex r_int("[0-9]*");
        string input;
        cout << "Entering OS Setup....\n\n" << endl;
        
        while (true) {
            cout << "Enter amount of memory: " << endl;
            cin >> input;
            if(regex_match(input, r_int))
                break;
            else
                cout << "Not a valid input\n" << endl;
        }
        memory_cap = stoi(input);
        while (true) {
            cout << "Enter number of printers: " << endl;
            cin >> input;
            if(regex_match(input, r_int))
                break;
            else
                cout << "Not a valid input\n" << endl;
        }
        num_printers = stoi(input);
        while (true) {
            cout << "Enter number of disks: " << endl;
            cin >> input;
            if(regex_match(input, r_int))
                break;
            else
                cout << "Not a valid input\n" << endl;
        }
        num_disks = stoi(input);
    }
    
    void run() {
        //Install enviorment
        install();
        
        //Initilize memory and devices
        memory memory_allocator(memory_cap);
        printer_queue.resize(num_printers);
        disk_queue.resize(num_disks);
        
        //Display available commands
        displayCommands();
        
        while(true) {
            //Get input and check
            string input;
            cin >> input;
            while(!regex_match(input, r_command)) {
                cout << "Not a valid command\n" << endl;
                cin >> input;
            }
            switch (input[0]) {
                //Create process
                case 'A': {
                    string memory_amount, priority;
                    cout << "Enter amount of memory to allocate for process: " << endl;
                    cin >> memory_amount;
                    if(!regex_match(memory_amount, r_int)) {
                        cout << "ERROR: Not a valid input. Cancelling....\n" << endl;
                        break;
                    }
                    cout << "Enter priority level for process: " << endl;
                    cin >> priority;
                    if(!regex_match(priority, r_int)) {
                        cout << "ERROR: Not a valid input. Cancelling....\n" << endl;
                        break;
                    }
                    process* p;
                    //Check if there is reuseable pcb
                    if(process_table.empty())
                        p = new process();
                    else {
                        p = process_table.front();
                        p->status = waiting;
                        process_table.pop();
                    }
                    //Reset process with fresh pid
                    p->pid = pid_counter+1;
                    p->status = waiting;
                    p->priority = stoi(priority);
                    //Allocate memory for process
                    p->memory_base = memory_allocator.allocate_memory(p->pid, stoi(memory_amount));
                    if(p->memory_base == -1) {
                        cout << "ERROR: Not enough memory for process. Cancelling....\n" << endl;
                        process_table.push(p);
                        break;
                    }
                    //Process sucessfully created
                    pid_counter++;
                    cout << "Created process with pid: " << p->pid << endl;
                    ready_queue.push(p);
                    //Update CPU
                    updateCPU();
                    break;
                }
                //Terminate running process
                case 't': {
                    //Check if any process is running
                    if(!running_process) {
                        cout << "ERROR :No process to terminated" << endl;
                        break;
                    }
                    process *terminated_process = running_process;
                    terminated_process->status = terminated;
                    //Deallocate memory
                    memory_allocator.deallocate_memory(terminated_process->pid);
                    cout << "Terminated process with pid: " << terminated_process->pid << endl;
                    //Push used pcb to process_table
                    process_table.push(terminated_process);
                    //Check if any processes on ready_queue
                    if(ready_queue.empty())
                        running_process = nullptr;
                    else {
                        running_process = ready_queue.top();
                        ready_queue.pop();
                    }
                    break;
                }
                //Printer interrupt
                case 'P': {
                    input = input.substr(1,input.size());
                    unsigned opt = stoi(input)-1;
                    if(opt+1 > printer_queue.size()) {
                        cout << "Requested Printer: " << opt+1 << " Available Printers: " << printer_queue.size() << endl;
                        cout << "ERROR: Not valid printer" << endl;
                        break;
                    }
                    if(printer_queue[opt].empty()) {
                        cout << "ERROR: Printer queue is empty" << endl;
                        break;
                    }
                    //Send process finished on printer back to ready_queue
                    process *p = printer_queue[opt].front();
                    printer_queue[opt].pop();
                    ready_queue.push(p);
                    cout << "Process " << p->pid << " completed on Printer " << input << endl;
                    updateCPU();
                    break;
                }
                //System call for a printer
                case 'p': {
                    input = input.substr(1,input.size());
                    unsigned int opt = stoi(input)-1;
                    if(!running_process) {
                        cout << "ERROR: No running process" << endl;
                        break;
                    }
                    if(opt+1 > printer_queue.size()) {
                        cout << "Requested Printer: " << opt+1 << " Available Printers: " << printer_queue.size() << endl;
                        cout << "ERROR: Not valid printer" << endl;
                        break;
                    }
                    //Get running process and send next process to CPU
                    process *p = running_process;
                    if(ready_queue.empty()) {
                        running_process = nullptr;
                    } else {
                        running_process = ready_queue.top();
                        ready_queue.pop();
                    }
                    string file_name, file_size;
                    cout << "Enter file name: " << endl;
                    cin >> file_name;
                    while (true) {
                        cout << "Enter file size: " << endl;
                        cin >> file_size;
                        if(regex_match(file_size, r_int))
                            break;
                        else
                            cout << "Not a valid file size\n" << endl;
                    }
                    //Set process information and send to printer queue
                    p->file_name = file_name;
                    p->file_size = file_size;
                    printer_queue[opt].push(p);
                    cout << "Process " << p->pid << " queued for Printer " << input << endl;
                    break;
                }
                //Disk interrupt
                case 'D': {
                    input = input.substr(1,input.size());
                    unsigned opt = stoi(input)-1;
                    if(opt+1 > disk_queue.size()) {
                        cout << "Requested Disk: " << opt+1 << " Available Disks: " << disk_queue.size() << endl;
                        cout << "ERROR: Not valid disk" << endl;
                        break;
                    }
                    if(disk_queue[opt].empty()) {
                        cout << "ERROR: Disk queue is empty" << endl;
                        break;
                    }
                    //Send process finished on disk back to ready_queue
                    process *p = disk_queue[opt].front();
                    disk_queue[opt].pop();
                    ready_queue.push(p);
                    cout << "Process " << p->pid << " completed on Disk " << input << endl;
                    updateCPU();
                    break;
                }
                //System call for a disk
                case 'd': {
                    input = input.substr(1,input.size());
                    unsigned opt = stoi(input)-1;
                    if(!running_process) {
                        cout << "ERROR: ERROR: No running process" << endl;
                        break;
                    }
                    if(opt+1 > disk_queue.size()) {
                        cout << "Requested Disk: " << opt+1 << " Available Disks: " << disk_queue.size() << endl;
                        cout << "ERROR: Not valid disk" << endl;
                        break;
                    }
                    //Get running process and send next process to CPU
                    process *p = running_process;
                    if(ready_queue.empty()) {
                        running_process = nullptr;
                    } else {
                        running_process = ready_queue.top();
                        ready_queue.pop();
                    }
                    string file_name, file_size;
                    cout << "Enter file name: " << endl;
                    cin >> file_name;
                    while (true) {
                        cout << "Enter file size: " << endl;
                        cin >> file_size;
                        if(regex_match(file_size, r_int))
                            break;
                        else
                            cout << "Not a valid file size\n" << endl;
                    }
                    //Set process information and send to disk queue
                    p->file_name = file_name;
                    p->file_size = file_size;
                    disk_queue[opt].push(p);
                    cout << "Process " << p->pid << " queued for Disk " << input << endl;
                    break;
                }
                //Snapshot interrupt
                case 'S': {
                    string snapshot_input;
                    cin >> snapshot_input;
                    if(!regex_match(snapshot_input, r_snapshot)) {
                        cout << "Not a valid snapshot command\n" << endl;
                        break;
                    }
                    switch(snapshot_input[0]) {
                        //Print out process on CPU and ready_queue processes
                        case 'r': {
                            cout << "Ready-queue status: " << endl;
                            cout << setw(5) << left << "pid"
                            << setw(10) << left << "Priority"
                            << setw(6) << left << "On CPU"<< endl;
                            if(running_process) {
                                cout << setw(5) << left << running_process->pid
                                << setw(10) << left << running_process->priority
                                << setw(6) << left << "*"<< endl;
                            }
                            priority_queue<process*, vector<process*>, compare_process> temp_queue = ready_queue;
                            while (!temp_queue.empty()) {
                                process *p = temp_queue.top();
                                temp_queue.pop();
                                cout << setw(5) << left << p->pid
                                << setw(10) << left << p->priority << endl;
                            }
                            break;
                        }
                        //Print out device queues and information
                        case 'i': {
                            cout << setw(15) << left << "Device"
                            << setw(5) << left << "pid"
                            << setw(20) << left << "Filename"
                            << setw(5) << "Filesize" << endl;
                            
                            for(int i = 0; i < printer_queue.size(); i++) {
                                queue<process*> device = printer_queue[i];
                                string d_id = "printer " + to_string(i+1);
                                while (!device.empty()) {
                                    process *p = device.front();
                                    device.pop();
                                    cout << setw(15) << left << d_id
                                    << setw(5) << left << p->pid
                                    << setw(20) << left << p->file_name
                                    << setw(5) << p->file_size << endl;
                                }
                            }
                            for(int i = 0; i < disk_queue.size(); i++) {
                                queue<process*> device = disk_queue[i];
                                string d_id = "disk" + to_string(i+1);
                                while (!device.empty()) {
                                    process *p = device.front();
                                    device.pop();
                                    cout << setw(15) << left << d_id
                                    << setw(5) << left << p->pid
                                    << setw(20) << left << p->file_name
                                    << setw(5) << p->file_size << endl;
                                }
                            }
                            break;
                        }
                        //Print out memory allocations
                        case 'm': {
                            cout << "Memory Snapshot:" << endl;
                            cout << setw(5) << left << "pid"
                            << setw(6) << left << "Start"
                            << setw(5) << left << "End"
                            << setw(5) << left << "Usage" << endl;
                            memory_allocator.memory_snapshot();
                            break;
                        }
                    }
                    break;
                }
                default:
                    displayCommands();
            }
        }
    }
    //Display available commands
    void displayCommands() {
        cout << "Available Commands" << endl;
        cout << setw(15) << left << "A Ex: A will create new process" << endl;
        cout << setw(15) << left << "t Ex: t will terminate running process" << endl;
        cout << setw(15) << left << "P<device id> Ex: P3 will terminate process on printer 3" << endl;
        cout << setw(15) << left << "p<device id> Ex: p3 will send running process to printer 3" << endl;
        cout << setw(15) << left << "D<device id> Ex: D6 will terminate process on disk 6" << endl;
        cout << setw(15) << left << "d<device id> Ex: d6 will send running process to disk 6" << endl;
        cout << setw(15) << left << "S Ex: Snapshot" << endl;
    }
    //Update CPU with highest priority process
    void updateCPU() {
        if(!running_process) {
            running_process = ready_queue.top();
            ready_queue.pop();
            running_process->status = running;
        }
        else {
            process* highest_priority_process = ready_queue.top();
            if (highest_priority_process < running_process) {
                ready_queue.pop();
                ready_queue.push(running_process);
                running_process = highest_priority_process;
            }
        }
    }
};

int main(int argc, const char * argv[]) {
    os os;
    os.run();
    return 0;
}
