#include <iostream>
#include <zconf.h>
#include "os.h"

using namespace std;



int main() {
    int pid = (int) getpid();
    cout << "main id=" << pid << endl;

//    signal(SIGBREAK, user_handler());
//
//    pause();

    os::signal_init();

    return 0;
}
