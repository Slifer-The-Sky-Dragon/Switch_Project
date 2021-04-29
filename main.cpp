#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <sys/types.h>
#include <unistd.h> 
#include <cstdlib>
#include <sys/wait.h>

#define NEW_SWITCH "MySwitch"

using namespace std;

void new_switch_command_handler(stringstream& ss){
    int port_cnt , switch_id;
    ss >> port_cnt >> switch_id;

    cout << port_cnt << ' ' << switch_id << endl;
}

void command_handler(string command){
    stringstream ss(command);
    string command_type;
    ss >> command_type;

    if(command_type == NEW_SWITCH)
        new_switch_command_handler(ss);
}

int main(){
    while(true){
        string command;
        getline(cin , command);
        command_handler(command);
    }
}