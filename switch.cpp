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

using namespace std;

#define NOT_DEFINED -1
#define EMPTY ""

#define DA_IND 1
#define SA_IND 7
#define DA_SA_LEN 6

const int MESSAGE_SIZE = 1500;
const int MAX_SWITCH = 100;
const int MAX_PORT_CNT = 100;

typedef enum {EXEC_NAME , PORT_CNT, SWITCH_ID} Argv_Indexes;

int find_int_value(string a , int st , int length){
    string temp = EMPTY;
    for(int i = st ; i < st + length ; i++){
        temp += a[i];
    }
    return atoi(temp.c_str());
}

string find_message_data(string message){
    string result = EMPTY;
    for(int i = SA_IND + DA_SA_LEN + 2 ; i < message.size() ; i++){
        result += message[i];
    }
    return result;
}

void ethernet_frame_decoder(string message){
    int da = find_int_value(message , DA_IND , DA_SA_LEN);
    int sa = find_int_value(message , SA_IND , DA_SA_LEN);

    string message_type = EMPTY;
    message_type += message[SA_IND + DA_SA_LEN];
    message_type += message[SA_IND + DA_SA_LEN + 1];

    string message_data = find_message_data(message);

    cout << "Dest Addrr = " << da << endl;
    cout << "Source Addrr = " << sa << endl;
    cout << "message type = " << message_type << endl;
    cout << "message data = " << message_data << endl;
}

int main(int argc , char* argv[]) {
    cout << "New Switch has been created..." << endl;

    int lookup_table[MAX_SWITCH] = { NOT_DEFINED };
    map < int , string > port_to_addrr;
    map < int , int > system_to_port;
    int switch_id = atoi(argv[SWITCH_ID]);
    int port_cnt = atoi(argv[PORT_CNT]);

    string switch_name_pipe = "fifo_ms" + to_string(switch_id);
    int switch_fd = open(switch_name_pipe.c_str() , O_RDONLY | O_NONBLOCK);

    if(switch_fd < 0){
        cout << "Error in opening name pipe..." << endl;
        exit(0);
    }

    fd_set read_fds;
    while(true){
        FD_ZERO(&read_fds);
        FD_SET(switch_fd , &read_fds);
        int res = select(switch_fd + 1 , &read_fds , NULL , NULL , NULL);
        if(res < 0)
            cout << "Error occured in select..." << endl;
        
        if(FD_ISSET(switch_fd , &read_fds)){
            char message[MESSAGE_SIZE];
            bzero(message , MESSAGE_SIZE);
            read(switch_fd , message , MESSAGE_SIZE);
            ethernet_frame_decoder(string(message));
        }
    }
}