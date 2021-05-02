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
#include <string.h>
#include <map>
#include <algorithm>

using namespace std;

#define EMPTY ""
#define MAIN_TYPE "00"
#define MESSAGE_TYPE "01"
#define FILE_TYPE "10"
#define CONFIG_TYPE "11"

#define SFD "$"

#define NOT_DEFINED -1
#define DA_IND 1
#define SA_IND 7
#define DA_SA_LEN 6

const int MESSAGE_SIZE = 1500;
const int MAX_SWITCH = 100;
const int MAX_PORT_CNT = 100;

typedef enum {EXEC_NAME , PORT_CNT, SWITCH_ID} Argv_Indexes;

typedef map < int , int > Sys2Port;
typedef map < int , int > Port2Addr;
typedef vector < int > FdVec;

int switch_id;

int find_int_value(string a , int st , int length){
    string temp = EMPTY;
    for(int i = st ; i < st + length ; i++){
        temp += a[i];
    }
    return atoi(temp.c_str());
}

string find_message_data(string message){
    string result = EMPTY;
    for(size_t i = SA_IND + DA_SA_LEN + 2 ; i < message.size() ; i++){
        result += message[i];
    }
    return result;
}

void main_command_handler(string message_data , Port2Addr& port_to_addr ,
                        Sys2Port& system_to_port , FdVec& all_fd){
    // cout << "main_command_handler::raw_message: " << message_data << endl;
    
    stringstream ss(message_data);
    string command;
    ss >> command;

    if (command == "C"){
        int port_number;
        string pipe_name;
        ss >> port_number >> pipe_name;

        int pipe_fd = open(pipe_name.c_str(), O_RDWR);
        if(pipe_fd < 0){
            perror("open");
            abort();
        }

        port_to_addr[port_number] = pipe_fd;
        all_fd.push_back(pipe_fd);

        string res = "SWITCH";
        res += "(ID = " + to_string(switch_id) + ")";
        res += ": Added connection ";
        res += "(port = " + to_string(port_number);
        res += ", pipe_name = " + pipe_name;
        res += ")";

        cout << res << endl;
    }

}

void message_command_handler(string raw_message , int received_fd, int main_switch_fd, int da, int sa , string message_data , Port2Addr& port_to_addr ,
                            Sys2Port& system_to_port , FdVec& all_fd){

    system_to_port[sa] = received_fd;
    if(system_to_port.find(da) != system_to_port.end()){
        int dest_port = system_to_port[da];
        write(port_to_addr[dest_port] , raw_message.c_str() , raw_message.size());
    }
    else{
        for(int i = 0 ; i < all_fd.size() ; i++){
            if(all_fd[i] == main_switch_fd || all_fd[i] == received_fd)
                continue;
            write(all_fd[i] , raw_message.c_str() , raw_message.size());
        }
    }
}

void switch_command_handler(int received_fd, int main_switch_fd, string raw_message, int da, int sa , string message_type ,
                            string message_data , Port2Addr& port_to_addr ,
                            Sys2Port& system_to_port , FdVec& all_fd){
    if(message_type == MAIN_TYPE)
        main_command_handler(message_data, port_to_addr, system_to_port, all_fd);
    if(message_type == MESSAGE_TYPE)
        message_command_handler(raw_message , received_fd , main_switch_fd , da , sa , message_data , port_to_addr , system_to_port , all_fd);
}

void ethernet_frame_decoder(string message, int received_fd, int main_switch_fd, Port2Addr& port_to_addr ,
                            Sys2Port& system_to_port , FdVec& all_fd){
    int da = find_int_value(message , DA_IND , DA_SA_LEN);
    int sa = find_int_value(message , SA_IND , DA_SA_LEN);

    string message_type = EMPTY;
    message_type += message[SA_IND + DA_SA_LEN];
    message_type += message[SA_IND + DA_SA_LEN + 1];

    string message_data = find_message_data(message);
    switch_command_handler(received_fd , main_switch_fd , message , da , sa , message_type , message_data ,
                        port_to_addr , system_to_port , all_fd);
}

int main(int argc , char* argv[]) {
    switch_id = atoi(argv[SWITCH_ID]);
    
    cout << "New Switch has been created..." << endl;

    Port2Addr port_to_addr;
    Sys2Port system_to_port;
    FdVec all_fd;
    
    int switch_id = atoi(argv[SWITCH_ID]);
    int port_cnt = atoi(argv[PORT_CNT]);

    string switch_name_pipe = "fifo_ms" + to_string(switch_id);
    int main_switch_fd = open(switch_name_pipe.c_str() , O_RDONLY | O_NONBLOCK);

    if(main_switch_fd < 0){
        cout << "Error in opening name pipe..." << endl;
        exit(0);
    }
    all_fd.push_back(main_switch_fd);

    fd_set read_fds;
    while(true){
        FD_ZERO(&read_fds);
        int max_fd = -1;
        for(int i = 0 ; i < all_fd.size() ; i++){
            FD_SET(all_fd[i] , &read_fds);
            max_fd = max(all_fd[i] , max_fd);
        }
        int res = select(max_fd + 1 , &read_fds , NULL , NULL , NULL);
        if(res < 0)
            cout << "Error occured in select..." << endl;
        
        for(int i = 0 ; i < all_fd.size() ; i++){
            if(FD_ISSET(all_fd[i] , &read_fds)){
                char message[MESSAGE_SIZE];
                bzero(message , MESSAGE_SIZE);
                read(all_fd[i] , message , MESSAGE_SIZE);
                ethernet_frame_decoder(string(message), all_fd[i] , main_switch_fd , port_to_addr , system_to_port , all_fd);
            }
        }
    }
}