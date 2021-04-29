#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <cstdlib>
#include <sys/wait.h>
#include <fcntl.h>

#define NEW_SWITCH "MySwitch"
#define NEW_SYSTEM "MySystem"

char SWITCH_EXEC[] = { "./switch" };
char SYSTEM_EXEC[] = { "./system" };

using namespace std;

typedef string ID;

void new_switch_command_handler(stringstream& ss , vector<ID>& switch_indexes){
    string port_cnt , switch_id;
    ss >> port_cnt >> switch_id;

    string fifo_name = "s" + switch_id;
    mkfifo(fifo_name.c_str(), 0666);
    
    int pid = fork();

    if (pid == 0){
        char* args[] = { SWITCH_EXEC , (char*)port_cnt.c_str() ,
                        (char*)switch_id.c_str(), NULL };
        
        execvp(SWITCH_EXEC, args);
    }

    else{
        switch_indexes.push_back(switch_id);
    }
}

void new_system_command_handler(stringstream& ss , vector<ID>& system_indexes){
    string system_id;
    ss >> system_id;
    
    string fifo_name = "ss" + system_id;
    mkfifo(fifo_name.c_str(), 0666);

    int pid = fork();

    if (pid == 0){
        char* args[] = {SYSTEM_EXEC , (char*)system_id.c_str() , NULL};
        
        execvp(SYSTEM_EXEC, args);
    }

    else{
        system_indexes.push_back(system_id);
    }
}

void command_handler(string command , 
                     vector<ID>& switch_indexes ,
                     vector<ID>& system_indexes){
    stringstream ss(command);
    string command_type;
    ss >> command_type;

    if(command_type == NEW_SWITCH)
        new_switch_command_handler(ss, switch_indexes);

    if(command_type == NEW_SYSTEM)
        new_system_command_handler(ss, system_indexes);
}

int main(){
    // ID id = 0;
    vector<ID> switch_indexes , system_indexes;

    string command;
    
    while(getline(cin , command)){
        command_handler(command, switch_indexes, system_indexes);
    }
}