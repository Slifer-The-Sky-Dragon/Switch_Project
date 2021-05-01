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
#include <map>

#define EMPTY ""
#define SFD "$"

#define MAIN_MESSAGE 0

#define NEW_SWITCH "MySwitch"
#define NEW_SYSTEM "MySystem"
#define CONNECT_SWITCH_SYSTEM "ConnectSwitchSystem"
#define CONNECT_SWITCH_SWITCH "ConnectSwitchSwitch"

char SWITCH_EXEC[] = { "./switch" };
char SYSTEM_EXEC[] = { "./system" };

using namespace std;


const string FIFO_PREFIX = "fifo_";
typedef string ID;

void new_switch_command_handler(stringstream& ss , vector<ID>& switch_indexes , map < string , int >& fifo_to_fd){
    string port_cnt , switch_id;
    ss >> port_cnt >> switch_id;

    string fifo_name = FIFO_PREFIX + "ms" + switch_id;
    mkfifo(fifo_name.c_str(), 0666);

    int pid = fork();

    if (pid == 0){
        char* args[] = { SWITCH_EXEC , (char*)port_cnt.c_str() ,
                        (char*)switch_id.c_str() , NULL };
        
        execvp(SWITCH_EXEC, args);
    }

    else{
        int new_fd = open(fifo_name.c_str() , O_WRONLY);
        fifo_to_fd[fifo_name] = new_fd;
        switch_indexes.push_back(switch_id);
    }
}

void new_system_command_handler(stringstream& ss , vector<ID>& system_indexes , map < string , int >& fifo_to_fd){
    string system_id;
    ss >> system_id;
    
    string fifo_name = FIFO_PREFIX + "mss" + system_id;
    mkfifo(fifo_name.c_str(), 0666);

    int pid = fork();

    if (pid == 0){
        char* args[] = {SYSTEM_EXEC , (char*)system_id.c_str() , NULL};
        
        execvp(SYSTEM_EXEC, args);
    }

    else{
        int new_fd = open(fifo_name.c_str() , O_WRONLY);
        fifo_to_fd[fifo_name] = new_fd;
        system_indexes.push_back(system_id);
    }
}

string convert_to_message_type(int type){
    if(type == MAIN_MESSAGE){
        return "00";
    }
    else
        return "01";
}

string extend_string_length(string a , int final_length){
    string result;
    for(int i = 0 ; i < final_length - a.size() ; i++){
        result += "0";
    }
    result += a;
    return result;
}

string convert_to_ehternet_frame(int da , int sa , int type , string data){
    string result = EMPTY;
    string message_type = convert_to_message_type(type);

    result = SFD + extend_string_length(to_string(da) , 6) + extend_string_length(to_string(sa) , 6) +
                         message_type + data + "\n";
    return result;
}

void connect_switch_system_command_handler(stringstream& ss , map < string , int >& fifo_to_fd){
    int system_id , switch_id , port_number;
    ss >> system_id >> switch_id >> port_number;

    string pipe_name = FIFO_PREFIX + "s" + to_string(switch_id) + "ss" + to_string(system_id);
    mkfifo(pipe_name.c_str() , 0666);

    string switch_data = "C " + to_string(port_number) + " " + pipe_name;
    string system_data = "C " + pipe_name;

    string switch_message = convert_to_ehternet_frame(0 , 0 , MAIN_MESSAGE , switch_data);
    string system_message = convert_to_ehternet_frame(0 , 0 , MAIN_MESSAGE , system_data);

    string switch_pipe_name = FIFO_PREFIX + "ms" + to_string(switch_id);
    string system_pipe_name = FIFO_PREFIX + "mss" + to_string(system_id);
    
    if(fifo_to_fd.find(switch_pipe_name) != fifo_to_fd.end() && fifo_to_fd.find(system_pipe_name) != fifo_to_fd.end()){
        write(fifo_to_fd[switch_pipe_name] , switch_message.c_str() , switch_message.size());
        write(fifo_to_fd[system_pipe_name] , system_message.c_str() , system_message.size());
    }    
}

void connect_switch_switch_command_handler(stringstream& ss , map < string , int >& fifo_to_fd){
    int switch_id1, switch_id2 , port_number1 , port_number2;
    ss >> switch_id1 >> port_number1 >> switch_id2 >> port_number2;

    string pipe_name = FIFO_PREFIX + "s" + to_string(switch_id1) + "s" + to_string(switch_id2);
    mkfifo(pipe_name.c_str() , 0666);

    string switch_data1 = "C " + to_string(port_number1) + " " + pipe_name;
    string switch_data2 = "C " + to_string(port_number2) + " " + pipe_name;

    string switch_message1 = convert_to_ehternet_frame(0 , 0 , MAIN_MESSAGE , switch_data1);
    string switch_message2 = convert_to_ehternet_frame(0 , 0 , MAIN_MESSAGE , switch_data2);

    string switch_pipe_name1 = FIFO_PREFIX + "ms" + to_string(switch_id1);
    string switch_pipe_name2 = FIFO_PREFIX + "ms" + to_string(switch_id2);
    
    if(fifo_to_fd.find(switch_pipe_name1) != fifo_to_fd.end() && fifo_to_fd.find(switch_pipe_name2) != fifo_to_fd.end()){
        write(fifo_to_fd[switch_pipe_name1] , switch_message1.c_str() , switch_message1.size());
        write(fifo_to_fd[switch_pipe_name2] , switch_message2.c_str() , switch_message2.size());
    }    
}

void command_handler(string command , 
                     vector<ID>& switch_indexes ,
                     vector<ID>& system_indexes , 
                     map < string , int >& fifo_to_fd){
    stringstream ss(command);
    string command_type;
    ss >> command_type;

    if(command_type == NEW_SWITCH)
        new_switch_command_handler(ss, switch_indexes , fifo_to_fd);
    else if(command_type == NEW_SYSTEM)
        new_system_command_handler(ss, system_indexes , fifo_to_fd);
    else if(command_type == CONNECT_SWITCH_SYSTEM)
        connect_switch_system_command_handler(ss , fifo_to_fd);
    else if(command_type == CONNECT_SWITCH_SWITCH)
        connect_switch_switch_command_handler(ss , fifo_to_fd);
}

int main(){
    vector<ID> switch_indexes , system_indexes;
    map < string , int > fifo_to_fd;
    string command;
    
    while(getline(cin , command)){
        command_handler(command, switch_indexes, system_indexes , fifo_to_fd);
    }
}