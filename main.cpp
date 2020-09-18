#include <iostream>
#include <zconf.h>
#include <csignal>
#include <semaphore.h>
#include <cstring>
#include <iostream>
#include "AttachListener.h"

using namespace std;

//kill -3 信号
#define SIGBREAK SIGTERM
//#define SIGBREAK SIGINT

typedef unsigned long           uintptr_t;
typedef uintptr_t     address_word;

#define CAST_TO_FN_PTR(func_type, value) (reinterpret_cast<func_type>(value))
#define CAST_FROM_FN_PTR(new_type, func_ptr) ((new_type)((address_word)(func_ptr)))

//信号标志位，当信号下标对应的值为1时，说明该信号被监听到了
static volatile int pending_signals[NSIG + 1] = {0};

static int sig_sem;

void signal_init_pd() {
    // Initialize signal structures
    ::memset((void*)pending_signals, 0, sizeof(pending_signals));

    // Initialize signal semaphore
    ::sem_init(&sig_sem, 0, 0);
}

static void UserHandler(int sig) {
    cout << "Interrupt signal (" << sig << ") received.\n" << endl;
    //将监听到的信号为竖为1
    //改操作应该为原子操作，确保不会被其他线程修改
    //JVM中混编平台相关汇编指令实现原子操作，演示代码中不研究相关实现
    pending_signals[sig] = 1;

    for(int i = 0;i< NSIG + 1;i++){
        cout << "i=" << i << " pending_signals= " << pending_signals[i] << endl;
    }

    ::sem_post(&sig_sem);
}

void* user_handler() {
    return CAST_FROM_FN_PTR(void*, UserHandler);
}

extern "C" {
typedef void (*sa_handler_t)(int);
typedef void (*sa_sigaction_t)(int, siginfo_t *, void *);
}

void* signal(int signal_number, void* handler) {
    struct sigaction sigAct, oldSigAct;

    sigfillset(&(sigAct.sa_mask));
    sigAct.sa_flags   = SA_SIGINFO;
    sigAct.sa_handler = CAST_TO_FN_PTR(sa_handler_t, handler);;

    int registration = sigaction(signal_number, &sigAct, &oldSigAct);
    if (registration) {
        // -1 means registration failed
        return (void *)-1;
    }

    return CAST_FROM_FN_PTR(void*, oldSigAct.sa_handler);
}

int main() {
    int pid = (int) getpid();
    cout << "main id=" << pid << endl;

    AttachListener::init();

    signal_init_pd();

    signal(SIGBREAK, user_handler());

    pause();

    return 0;
}
