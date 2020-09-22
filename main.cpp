#include <iostream>
#include <zconf.h>
#include "os.h"

using namespace std;



int main() {
    int pid = (int) getpid();
    cout << "main id=" << pid << endl;

    os::signal_init();

    while (true) {}
    return 0;
}
