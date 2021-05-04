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
#define CONFIG_TYPE "11"

#define FILE_START_TYPE "FS"
#define FILE_LINE_TYPE "FL"
#define FILE_END_TYPE "FE"

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
typedef map < int , int > Port2Fd;
typedef vector < int > FdVec;

int switch_id;

struct system {
    int id, read_fd, write_fd;
};

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


void main_command_handler(string message_data , Port2Fd& port_to_write_fd ,
                        Sys2Port& system_to_port , FdVec& all_read_fd , FdVec& all_write_fd , 
                        map < int , int >& read_fd_to_write_fd){

    stringstream ss(message_data);
    string command;
    ss >> command;

    if (command == "C"){
        int port_number;
        string pipe_name1 , pipe_name2;
        ss >> port_number >> pipe_name1 >> pipe_name2;

        int switch_read_fd = open(pipe_name2.c_str(), O_RDWR);
        int switch_write_fd = open(pipe_name1.c_str(), O_RDWR);

        if(switch_write_fd < 0 || switch_read_fd < 0){
            perror("open");
            abort();
        }

        port_to_write_fd[port_number] = switch_write_fd;
        all_write_fd.push_back(switch_write_fd);
        all_read_fd.push_back(switch_read_fd);

        read_fd_to_write_fd[switch_read_fd] = switch_write_fd;

        string res = "SWITCH";
        res += "(ID = " + to_string(switch_id) + ")";
        res += ": Added connection ";
        res += "(port = " + to_string(port_number);
        res += ", read_fd = " + to_string(switch_read_fd);
        res += ", write_fd = " + to_string(switch_write_fd);
        res += ")";

        cout << res << endl;
    }

}

string switch_info() {
    return "SWITCH(ID = " + to_string(switch_id) + ")";
}

string extend_string_length(string a , size_t final_length){
    string result;
    for(size_t i = 0 ; i < final_length - a.size() ; i++){
        result += "0";
    }
    result += a;
    return result;
}

string convert_to_ehternet_frame(int da , int sa , string message_type , string data){
    string result = EMPTY;

    result = SFD + extend_string_length(to_string(da) , 6) + extend_string_length(to_string(sa) , 6) +
                         message_type + data + "\n";
    return result;
}

void message_command_handler(string raw_message , int received_fd, int main_switch_fd, int da, int sa , string message_data , 
                            Port2Fd& port_to_write_fd , Sys2Port& system_to_write_port , FdVec& all_read_fd,
                            FdVec& all_write_fd , map < int , int >& read_fd_to_write_fd){

    system_to_write_port[sa] = read_fd_to_write_fd[received_fd];

    // string framed_message = convert_to_ehternet_frame(da , sa, MESSAGE_TYPE , raw_message);

    if(system_to_write_port.find(da) != system_to_write_port.end()){
        int dest_write_fd = system_to_write_port[da];
        write(dest_write_fd , raw_message.c_str() , raw_message.size());
        string res = EMPTY;
        res += switch_info();
        res += ": Sent packet ";
        res += "(source = " + to_string(sa);
        res += ", dest = " + to_string(da);
        res += ")";
        // sleep(1);
        cout << res << '\n';
    }
    else{
        for(size_t i = 0 ; i < all_write_fd.size() ; i++){
            if(all_write_fd[i] == read_fd_to_write_fd[received_fd])
                continue;
            write(all_write_fd[i] , raw_message.c_str() , raw_message.size());
            string res = EMPTY;
            res += switch_info();
            res += ": Broadcasted packet ";
            res += "(source = " + to_string(sa);
            res += ", dest = " + to_string(da);
            res += ", fd = " + to_string(all_write_fd[i]);
            res += ")";
            // sleep(1);
            cout << res << '\n';
        }
    }
}

void switch_command_handler(int received_fd, int main_switch_fd, string raw_message, int da, int sa , string message_type ,
                            string message_data , Port2Fd& port_to_write_fd ,
                            Sys2Port& system_to_port , FdVec& all_read_fd , FdVec& all_write_fd, 
                            map < int , int >& read_fd_to_write_fd){
    if(message_type == MAIN_TYPE)
        main_command_handler(message_data, port_to_write_fd, system_to_port, all_read_fd , all_write_fd , read_fd_to_write_fd);
    
    else if(message_type == CONFIG_TYPE) {}
    else
        message_command_handler(raw_message , received_fd , main_switch_fd , da , sa , message_data , port_to_write_fd ,
                                 system_to_port , all_read_fd , all_write_fd , read_fd_to_write_fd);
}

void ethernet_frame_decoder(string message, int received_fd, int main_switch_fd, Port2Fd& port_to_write_fd ,
                            Sys2Port& system_to_port , FdVec& all_read_fd , FdVec& all_write_fd , 
                            map < int , int >& read_fd_to_write_fd){
    int da = find_int_value(message , DA_IND , DA_SA_LEN);
    int sa = find_int_value(message , SA_IND , DA_SA_LEN);

    string message_type = EMPTY;
    message_type += message[SA_IND + DA_SA_LEN];
    message_type += message[SA_IND + DA_SA_LEN + 1];

    string message_data = find_message_data(message);
    switch_command_handler(received_fd , main_switch_fd , message , da , sa , message_type , message_data ,
                            port_to_write_fd , system_to_port , all_read_fd , all_write_fd , read_fd_to_write_fd);
}

int main(int argc , char* argv[]) {
    switch_id = atoi(argv[SWITCH_ID]);
    // int port_cnt = atoi(argv[PORT_CNT]);
    cout << "New Switch has been created..." << endl;

    Port2Fd port_to_write_fd;
    Sys2Port system_to_port;
    FdVec all_read_fd;
    FdVec all_write_fd;
    map < int , int > read_fd_to_write_fd;

    string switch_name_pipe = "fifo_ms" + to_string(switch_id);
    int main_switch_fd = open(switch_name_pipe.c_str() , O_RDONLY | O_NONBLOCK);

    if(main_switch_fd < 0){
        cout << "Error in opening name pipe..." << endl;
        exit(0);
    }
    all_read_fd.push_back(main_switch_fd);

    fd_set read_fds;
    while(true){
        FD_ZERO(&read_fds);
        int max_fd = -1;
        for(size_t i = 0 ; i < all_read_fd.size() ; i++){
            FD_SET(all_read_fd[i] , &read_fds);
            max_fd = max(all_read_fd[i] , max_fd);
        }
        // cout << "hi" << endl;
        int res = select(max_fd + 1 , &read_fds , NULL , NULL , NULL);
        if(res < 0)
            cout << "Error occured in select..." << endl;
        
        for(size_t i = 0 ; i < all_read_fd.size() ; i++){
            if(FD_ISSET(all_read_fd[i] , &read_fds)){
                char message[MESSAGE_SIZE];
                bzero(message , MESSAGE_SIZE);
                int res = read(all_read_fd[i] , message , MESSAGE_SIZE);
                if (res == 0) {
                    cout << switch_info() << ": PANIC! Read EOF on descriptor!" << endl;
                    abort();
                }
                // cout << string(message) << endl;
                ethernet_frame_decoder(string(message), all_read_fd[i] , main_switch_fd , port_to_write_fd , 
                                        system_to_port , all_read_fd , all_write_fd , read_fd_to_write_fd);
            }
        }
    }
}