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

//Class for process
//Stores porcess pid
class process {
public:
    pid_t pid;
    unsigned int priority, memory_base;
    proces_status status;
    string file_name;
    string file_size;
    
};

class memory {
    struct memory_range {
        unsigned int base, limit;
        pid_t pid;
        bool operator<(const memory_range& rhs) const {
            return this->base < rhs.base; }
        memory_range(pid_t pid, unsigned int base, unsigned int limit) : pid(pid), base(base), limit(limit) { }
        
    };
    
private:
    unsigned int memory_cap, free, used;
    vector<memory_range> memory_allocations;
    
public:
    memory(unsigned int memory_cap) {
        this->memory_cap = memory_cap;
        this->free = memory_cap;
    }
    int allocate_memory(pid_t pid, unsigned int amount) {
        int available_space = 0, new_available_space = 0;
        unsigned int start = 0;
        bool potential_allocation = false;
        if (amount > free)
            return -1;
        if (memory_allocations.empty()) {
            memory_allocations.push_back(memory_range(pid, 0, start+amount));
            free-=amount;
            return 0;
        }
        memory_range first = memory_allocations.front();
        new_available_space = first.base;
        if(available_space <= new_available_space && new_available_space >=amount) {
            available_space = new_available_space;
            start = 0;
            potential_allocation = true;
        }
        memory_range last = memory_allocations.back();
        new_available_space = memory_cap - last.limit;
        if(available_space <= new_available_space && new_available_space >=amount) {
            available_space = new_available_space;
            start = last.limit+1;
            potential_allocation = true;
        }
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
        if (!potential_allocation)
            return -1;
        else {
            memory_allocations.push_back(memory_range(pid, start, start+amount));
            free-=amount;
            sort(memory_allocations.begin(), memory_allocations.end());
            return start;
        }
    }
    
    bool dellocate_memory(pid_t pid) {
        for (vector<memory_range>::iterator i = memory_allocations.begin(); i != memory_allocations.end(); i++) {
            if (i->pid == pid) {
                free+=(i->limit-i->base);
                memory_allocations.erase(i);
                return true;
            }
        }
        return false;
    }
    
    void memory_snapshot() {
        for (vector<memory_range>::iterator i = memory_allocations.begin(); i != memory_allocations.end(); i++) {
            cout << setw(5) << left << i->base
            << setw(5) << left << i->limit
            << setw(5) << left << i->pid << endl;
        }
        cout << "Used Memory: " << memory_cap-free << endl;
        cout << "Free Memory: " << free << endl;
    }
};

void install(unsigned int &memory_cap, unsigned int &num_printers, unsigned int &num_disks) {
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
void displayCommands() {
    cout << "Available Commands" << endl;
    cout << "P<device id>\t Ex: P3 will terminate process on printer 3" << endl;
    cout << "p<device id>\t Ex: p3 will send running process to printer 3" << endl;
    cout << "D<device id>\t Ex: D6 will terminate process on disk 6" << endl;
    cout << "d<device id>\t Ex: d6 will send running process to disk 6\n" << endl;
}

int main(int argc, const char * argv[]) {
    struct compare_process : public binary_function<process*, process*, bool> {
        bool operator()(const process* left, const process* right) const {
            return !(left->priority < right->priority);
        }
    };
    
    unsigned int memory_cap;
    pid_t pid_counter = 0;
    process* running_process = nullptr;
    priority_queue<process*, vector<process*>, compare_process> ready_queue;
    queue<process*> process_table;
    vector<queue<process*>> printer_queue;
    vector<queue<process*>> disk_queue;
    
    const regex r_int("[0-9]*");
    const regex r_command("[hpPdD][1-9][0-9]*|[AtS]");
    const regex r_snapshot("[rim]");
    const regex r_string("[a-zA-Z_][a-zA-Z_0-9]*\\.[a-zA-Z0-9]+");
    
    unsigned int num_printers, num_disks;
    
    install(memory_cap, num_printers, num_disks);
    
    memory memory_allocator(memory_cap);
    printer_queue.resize(num_printers);
    disk_queue.resize(num_disks);
    
    displayCommands();
    
    while(true) {
        string input;
        cin >> input;
        while(!regex_match(input, r_command)) {
            cout << "Not a valid command\n" << endl;
            cin >> input;
        }
        switch (input[0]) {
            case 'A': {
                string memory_amount, priority;
                cout << "Enter amount of memory to allocate for process: " << endl;
                cin >> memory_amount;
                if(!regex_match(memory_amount, r_int)) {
                    cout << "Not a valid input\n" << endl;
                    break;
                }
                cout << "Enter priority level for process: " << endl;
                cin >> priority;
                if(!regex_match(priority, r_int)) {
                    cout << "Not a valid input\n" << endl;
                    break;
                }
                process* p;
                if(process_table.empty())
                    p = new process();
                else {
                    p = process_table.front();
                    p->status = waiting;
                    process_table.pop();
                }
                
                p->pid = pid_counter+1;
                p->status = waiting;
                p->priority = stoi(priority);
                p->memory_base = memory_allocator.allocate_memory(p->pid, stoi(memory_amount));
                if(p->memory_base == -1) {
                    cout << "ERROR: Not enough memory for process. Cancelling....\n" << endl;
                    process_table.push(p);
                    break;
                }
                pid_counter++;
                cout << "Created process with pid: " << p->pid << endl;
                ready_queue.push(p);
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
                break;
            }
            case 't': {
                if(!running_process) {
                    cout << "ERROR :No process to terminated" << endl;
                    break;
                }
                process *terminated_process = running_process;
                terminated_process->status = terminated;
                //Dealloc memory
                memory_allocator.dellocate_memory(terminated_process->pid);
                cout << "Terminated process with pid: " << terminated_process->pid << endl;
                process_table.push(terminated_process);
                if(ready_queue.empty())
                    running_process = nullptr;
                else {
                    running_process = ready_queue.top();
                    ready_queue.pop();
                }
                break;
            }
            case 'P': {
                input = input.substr(1,input.size());
                unsigned opt = stoi(input)-1;
                if(opt+1 > printer_queue.size()) {
                    cout << opt << " " << printer_queue.size() << endl;
                    cout << "ERROR: Not valid printer" << endl;
                    break;
                }
                if(printer_queue[opt].empty()) {
                    cout << "ERROR: Printer queue is empty" << endl;
                    break;
                }
                process *p = printer_queue[opt].front();
                printer_queue[opt].pop();
                ready_queue.push(p);
                cout << "Process " << p->pid << " completed on Printer " << input << endl;
                break;
            }
            case 'p': {
                input = input.substr(1,input.size());
                unsigned int opt = stoi(input)-1;
                if(!running_process) {
                    cout << "ERROR: No running process" << endl;
                    break;
                }
                if(opt+1 > printer_queue.size()) {
                    cout << opt << " " << printer_queue.size() << endl;
                    cout << "ERROR: Not valid printer" << endl;
                    break;
                }
                process *p = running_process;
                running_process = ready_queue.top();
                ready_queue.pop();
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
                
                p->file_name = file_name;
                p->file_size = file_size;
                printer_queue[opt].push(p);
                cout << "Process " << p->pid << " queued for Printer " << input << endl;
                break;
            }
            case 'D': {
                input = input.substr(1,input.size());
                unsigned opt = stoi(input)-1;
                if(opt+1 > disk_queue.size()) {
                    cout << opt << " " << disk_queue.size() << endl;
                    cout << "ERROR: Not valid disk" << endl;
                    break;
                }
                if(disk_queue[opt].empty()) {
                    cout << "ERROR: Disk queue is empty" << endl;
                    break;
                }
                process *p = disk_queue[opt].front();
                disk_queue[opt].pop();
                ready_queue.push(p);
                cout << "Process " << p->pid << " completed on Disk " << input << endl;
                break;
            }
            case 'd': {
                input = input.substr(1,input.size());
                unsigned opt = stoi(input)-1;
                if(!running_process) {
                    cout << "ERROR: ERROR: No running process" << endl;
                    break;
                }
                if(opt+1 > disk_queue.size()) {
                    cout << opt << " " << disk_queue.size() << endl;
                    cout << "ERROR: Not valid disk" << endl;
                    break;
                }
                process *p = running_process;
                running_process = ready_queue.top();
                ready_queue.pop();
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
                p->file_name = file_name;
                p->file_size = file_size;
                disk_queue[opt].push(p);
                cout << "Process " << p->pid << " queued for Disk " << input << endl;
                break;
            }
            case 'S': {
                string snapshot_input;
                cin >> snapshot_input;
                if(!regex_match(snapshot_input, r_snapshot)) {
                    cout << "Not a valid snapshot command\n" << endl;
                    break;
                }
                switch(snapshot_input[0]) {
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
                    case 'i': {
                        cout << setw(15) << left << "Device"
                        << setw(5) << left << "pid"
                        << setw(10) << left << "Filename"
                        << setw(5) << "Filesize" << endl;
                        
                        for(int i = 0; i < printer_queue.size(); i++) {
                            queue<process*> device = printer_queue[i];
                            string d_id = "printer " + to_string(i+1);
                            while (!device.empty()) {
                                process *p = device.front();
                                device.pop();
                                cout << setw(15) << left << d_id
                                << setw(5) << left << p->pid
                                << setw(10) << left << p->file_name
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
                                << setw(10) << left << p->file_name
                                << setw(5) << p->file_size << endl;
                            }
                        }
                        break;
                    }
                    case 'm': {
                        cout << "Memory Snapshot:" << endl;
                        cout << setw(6) << left << "Start"
                        << setw(5) << left << "End"
                        << setw(5) << left << "pid" << endl;
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
    return 0;
}
