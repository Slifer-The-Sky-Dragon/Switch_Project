#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
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

using namespace std;

typedef enum {EXEC_NAME , SYSTEM_ID} Argv_Indexes;

#define SFD "$"

#define EMPTY ""
#define MAIN_TYPE "00"
#define MESSAGE_TYPE "01"
#define CONFIG_TYPE "11"

#define FILE_START_TYPE "FS"
#define FILE_LINE_TYPE "FL"
#define FILE_END_TYPE "FE"

#define NOT_DEFINED -1
#define DA_IND 1
#define SA_IND 7
#define DA_SA_LEN 6

#define MAIN_MESSAGE 0
#define SYSTEM_MESSAGE 1

const int MESSAGE_SIZE = 1500;
const string FILE_PREFIX = "file_";

int system_id;

string system_info() {
    return "SYSTEM(ID = " + to_string(system_id) + ")";
}

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

void main_command_handler(string message_data , int& system_write_pipe_fd , int& system_read_pipe_fd){
    stringstream ss(message_data);
    string command;
    ss >> command;

    if(command == "C"){
        string pipe_name1 , pipe_name2;
        ss >> pipe_name1 >> pipe_name2;
        system_read_pipe_fd = open(pipe_name2.c_str(), O_RDWR);
        system_write_pipe_fd = open(pipe_name1.c_str(), O_RDWR);
        
        if(system_write_pipe_fd < 0 || system_read_pipe_fd < 0){
            perror("open");
            abort();
        }

        string res = "";
        res += system_info();
        res += ": Connected to switch ";
        res += "(read_fd = " + to_string(system_read_pipe_fd);
        res += ", write_fd = " + to_string(system_write_pipe_fd);
        res += ")";
        cout << res << endl;
    }

    if(command == "S"){
        int da;
        string message_body = EMPTY;
        ss >> da;

        string line;
        bool check = false;
        while(ss >> line){
            if(check == true)
                message_body += " ";
            message_body += line;
            check = true;
        }
        
        string frame = convert_to_ehternet_frame(da , system_id , MESSAGE_TYPE , message_body);
        write(system_write_pipe_fd , frame.c_str() , frame.size());
        
        string res = "";
        res += system_info();
        res += ": Sent message ";
        res += "(write_fd = " + to_string(system_write_pipe_fd);
        res += ", dest = " + to_string(da);
        res += ", message = \"" + message_body + "\")";
    
        cout << res << endl;
    }

    if(command == "F"){
        int da;
        string file_name;
        ss >> da >> file_name;

        ifstream file(file_name);
        string file_line;
        vector<string> lines;
        while(getline(file, file_line))
            lines.push_back(file_line);

        string file_start_data = file_name;
        string file_start_frame = convert_to_ehternet_frame(da , system_id ,
                                    FILE_START_TYPE , file_start_data);
        // file_start_frame += '\n';
        write(system_write_pipe_fd, file_start_frame.c_str(), file_start_frame.size());
        
        string res = "";
        res += system_info();
        res += ": Sent file start ";
        res += "(write_fd = " + to_string(system_write_pipe_fd);
        res += ", dest = " + to_string(da);
        res += ", message = \"" + file_start_data + "\")";

        cout << res << endl;
        usleep(1000);
        for(size_t i = 0; i < lines.size(); i++){
            // "$<header><da><sa><file_type><filename> <line>";
            string file_line_data = file_name + " " + lines[i];
            string file_line_frame = convert_to_ehternet_frame(da , system_id ,
                                    FILE_LINE_TYPE , file_line_data);
            // file_line_frame += '\n';
            write(system_write_pipe_fd, file_line_frame.c_str(), file_line_frame.size());
            string res = "";
            res += system_info();
            res += ": Sent file line ";
            res += "(write_fd = " + to_string(system_write_pipe_fd);
            res += ", dest = " + to_string(da);
            res += ", message = \"" + file_line_data + "\")";

            cout << res << endl;
            usleep(1000);
        }

        string file_end_data = file_name;
        string file_end_frame = convert_to_ehternet_frame(da , system_id ,
                                    FILE_END_TYPE , file_end_data);
        
        write(system_write_pipe_fd, file_end_frame.c_str(), file_end_frame.size());        
        
        res = "";
        res += system_info();
        res += ": Sent file end ";
        res += "(write_fd = " + to_string(system_write_pipe_fd);
        res += ", dest = " + to_string(da);
        res += ", message = \"" + file_end_data + "\")";

        cout << res << endl;
        usleep(1000);
        file.close();
    }
}

string clear_new_line(string in){
    string res = "";
    for (size_t i = 0 ; i < in.size() ; i++)
        if(in[i] != '\n')
            res += in[i];

    return res;
}

void message_command_handler(int da , int sa , string message_data){
    string res = "";
    res += system_info();
    res += ": Received message ";
    res += "(source = " + to_string(sa);
    res += ", dest = " + to_string(da);
    res += ", message = \"" + clear_new_line(message_data) + "\")";

    cout << res << endl;
}

void file_start_command_handler(int da , int sa , string message_data){
    string file_name = FILE_PREFIX + "ss" + to_string(system_id) + "_" + clear_new_line(message_data);
    ofstream file(file_name);
    file.close();

    string res = "";
    res += system_info();
    res += ": File created ";
    res += "(file_name = \"" + file_name + "\")";

    cout << res << endl;
}

void file_line_command_handler(int da , int sa , string message_data){
    stringstream ss(message_data);
    string file_name, line;
    ss >> file_name;

    string s;
    bool check = false;
    while(ss >> s){
        if(check == true)
            line += " ";
        line += s;
        check = true;
    }

    file_name = FILE_PREFIX + "ss" + to_string(system_id) + "_" + clear_new_line(file_name);
    ofstream file(file_name, ios_base::app);
    file << line << endl;
    file.close();

    string res = "";
    res += system_info();
    res += ": File updated ";
    res += "(file_name = \"" + file_name + "\"";
    res += ", line = \"" + line + "\"";
    res += ")";

    cout << res << endl;
}

void system_command_handler(int da , int sa , string message_type ,
                            string message_data , int& system_write_pipe_fd , int& system_read_pipe_fd){
    if(message_type == MAIN_TYPE)
        main_command_handler(message_data , system_write_pipe_fd , system_read_pipe_fd);

    else if(message_type == MESSAGE_TYPE)
        message_command_handler(da , sa , message_data);

    else if(message_type == FILE_START_TYPE)
        file_start_command_handler(da , sa , message_data);
    
    else if(message_type == FILE_LINE_TYPE)
        file_line_command_handler(da , sa , message_data);

    else if(message_type == FILE_END_TYPE) {}
}

void ethernet_frame_decoder(string message , int& system_write_pipe_fd , int& system_read_pipe_fd){
    int da = find_int_value(message , DA_IND , DA_SA_LEN);
    int sa = find_int_value(message , SA_IND , DA_SA_LEN);

    string message_type = EMPTY;
    message_type += message[SA_IND + DA_SA_LEN];
    message_type += message[SA_IND + DA_SA_LEN + 1];

    string message_data = find_message_data(message);
    system_command_handler(da , sa , message_type , message_data , system_write_pipe_fd , system_read_pipe_fd);
}

int main(int argc , char* argv[]) {
    cout << "New System has been created..." << endl;

    system_id = atoi(argv[SYSTEM_ID]);
    int system_write_pipe_fd = -1;
    int system_read_pipe_fd = -1;
    
    string system_name_pipe = "fifo_mss" + to_string(system_id);
    int system_fd = open(system_name_pipe.c_str() , O_RDONLY | O_NONBLOCK);

    if(system_fd < 0){
        cout << "Error in opening name pipe..." << endl;
        exit(0);
    }

    fd_set read_fds;
    while(true){
        FD_ZERO(&read_fds);
        FD_SET(system_fd , &read_fds);
        if(system_read_pipe_fd != -1)
            FD_SET(system_read_pipe_fd , &read_fds);
        int max_fd = max(system_fd, system_read_pipe_fd);
        int res = select(max_fd + 1, &read_fds , NULL , NULL , NULL);
        if(res < 0)
            cout << "Error occured in select..." << endl;
        
        // cout << system_info() << ": Event occured" << '\n';

        if(FD_ISSET(system_fd , &read_fds)){
            char message[MESSAGE_SIZE];
            bzero(message , MESSAGE_SIZE);
            int res = read(system_fd , message , MESSAGE_SIZE);
            if (res == 0) {
                cout << system_info() << ": PANIC! Read EOF on descriptor!" << endl;
                abort();
            }
            // cout << string(message) << endl;
            ethernet_frame_decoder(string(message), system_write_pipe_fd , system_read_pipe_fd);
        }
        if(FD_ISSET(system_read_pipe_fd , &read_fds)){
            char message[MESSAGE_SIZE];
            bzero(message , MESSAGE_SIZE);
            int res = read(system_read_pipe_fd , message , MESSAGE_SIZE);
            if (res == 0) {
                cout << system_info() << ": PANIC! Read EOF on descriptor!" << endl;
                abort();
            }
            // cout << string(message) << endl;
            ethernet_frame_decoder(string(message), system_write_pipe_fd , system_read_pipe_fd);
        }
    }
}