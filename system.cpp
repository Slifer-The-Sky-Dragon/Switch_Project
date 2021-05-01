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

#define NOT_DEFINED -1
#define EMPTY ""

const int MESSAGE_SIZE = 1500;

using namespace std;

typedef enum {EXEC_NAME , SYSTEM_ID} Argv_Indexes;

int main(int argc , char* argv[]) {
    cout << "New System has been created..." << endl;

    int system_id = atoi(argv[SYSTEM_ID]);

    string system_name_pipe = "fifo_mss" + to_string(system_id);
    int system_fd = open(system_name_pipe.c_str() , O_RDONLY | O_NONBLOCK);

    if(system_fd < 0){
        cout << "RIDY" << endl;
    }

    cout << "I'm here" << endl;

    fd_set read_fds;
    while(true){
        FD_ZERO(&read_fds);
        FD_SET(system_fd , &read_fds);
        int res = select(system_fd + 1 , &read_fds , NULL , NULL , NULL);
        cout << "salam" << endl;
        if(res < 0)
            cout << "Error occured..." << endl;
        
        if(FD_ISSET(system_fd , &read_fds)){
            char message[MESSAGE_SIZE];
            bzero(message , MESSAGE_SIZE);
            read(system_fd , message , MESSAGE_SIZE);
            cout << string(message) << endl;
        }
    }
}