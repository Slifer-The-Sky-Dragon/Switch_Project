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

typedef enum {EXEC_NAME , SYSTEM_ID} Argv_Indexes;

int main(int argc , char* argv[]) {
    cout << "System process (id = " << argv[SYSTEM_ID] << ")" << endl;
}