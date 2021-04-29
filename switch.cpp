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

using namespace std;

typedef enum {EXEC_NAME , PORT_CNT, SWITCH_ID} Argv_Indexes;

int main(int argc , char* argv[]) {
    cout << "Switch process (id = " << argv[SWITCH_ID] << ", port count = "
         << argv[PORT_CNT] << ")" << endl;
}