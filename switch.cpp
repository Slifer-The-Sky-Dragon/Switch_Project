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

using namespace std;

#define NOT_DEFINED -1
#define EMPTY ""

const int MESSAGE_SIZE = 1500;
const int MAX_SWITCH = 100;
const int MAX_PORT_CNT = 100;

typedef enum {EXEC_NAME , PORT_CNT, SWITCH_ID} Argv_Indexes;

int main(int argc , char* argv[]) {
    cout << "New Switch has been created..." << endl;

    int lookup_table[MAX_SWITCH] = { NOT_DEFINED };
    string ports_info[MAX_PORT_CNT] = { EMPTY };
    int switch_id = atoi(argv[SWITCH_ID]);
    int port_cnt = atoi(argv[PORT_CNT]);

    string switch_name_pipe = "fifo_ms" + to_string(switch_id);
    int switch_fd = open(switch_name_pipe.c_str() , O_RDONLY | O_NONBLOCK);

    if(switch_fd < 0){
        cout << "RIDY" << endl;
    }

    cout << "I'm here" << endl;

    fd_set read_fds;
    while(true){
        FD_ZERO(&read_fds);
        FD_SET(switch_fd , &read_fds);
        int res = select(switch_fd + 1 , &read_fds , NULL , NULL , NULL);
        cout << "salam" << endl;
        if(res < 0)
            cout << "Error occured..." << endl;
        
        if(FD_ISSET(switch_fd , &read_fds)){
            char message[MESSAGE_SIZE];
            bzero(message , MESSAGE_SIZE);
            read(switch_fd , message , MESSAGE_SIZE);
            cout << string(message) << endl;
        }
    }
}