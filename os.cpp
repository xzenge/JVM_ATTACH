//
// Created by Xiang Shi on 2020/9/21.
//

#include <cstring>
#include <sys/param.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/sync_policy.h>
#include <mach/task.h>
#include <signal.h>
#include "os.h"
#include "AttachListener.h"
#include <iostream>
#include <mach/semaphore.h>
#include <thread>

using namespace std;

typedef semaphore_t os_semaphore_t;
typedef uintptr_t     address_word;
#define SIGBREAK SIGQUIT
#define CAST_TO_FN_PTR(func_type, value) (reinterpret_cast<func_type>(value))
#define CAST_FROM_FN_PTR(new_type, func_ptr) ((new_type)((address_word)(func_ptr)))
//信号标志位，当信号下标对应的值为1时，说明该信号被监听到了
static volatile int pending_signals[NSIG+1] = { 0 };
static os_semaphore_t sig_sem;

/**
 * 单独线程对pending_signals状态并进行相应处理
 */
static void signal_thread_entry() {
    cout << "enter signal_thread_entry" << endl;
    while (true){
        int sig;
        {
            sig = os::signal_wait();
//            cout << "signal_thread_entry sig:" << sig << endl;
        }
        switch (sig) {
            case SIGBREAK: {
                if (AttachListener::init()) {
                    continue;
                }
            }
        }

    }
}

/**
 * JVM启动时监听signal
 */
void os::signal_init() {
    //初始化signal监听pending_signals
    os::signal_init_pd();
    //创建线程监听pending_signals状态并进行相应处理
    pthread_t tids;
    int ret = pthread_create(&tids, NULL, reinterpret_cast<void *(*)(void *)>(signal_thread_entry), NULL);
    //注册signal处理器
    os::signal(SIGBREAK,os::user_handler());
}

/**
 * 初始化监听pending_signals
 */
void os::signal_init_pd() {
    // Initialize signal structures
    ::memset((void*)pending_signals, 0, sizeof(pending_signals));

    // Initialize signal semaphore
    //    ::SEM_INIT(sig_sem, 0);
    semaphore_create(mach_task_self(), &sig_sem, SYNC_POLICY_FIFO, 0);
}

extern "C" {
typedef void (*sa_handler_t)(int);
typedef void (*sa_sigaction_t)(int, siginfo_t *, void *);
}

/**
 * 注册监听信号&处理器
 * @param signal_number 信号编号
 * @param handler 信号处理器
 * @return
 */
void *os::signal(int signal_number, void *handler) {
    struct sigaction sigAct, oldSigAct;

    sigfillset(&(sigAct.sa_mask));
    sigAct.sa_flags   = SA_RESTART|SA_SIGINFO;
    sigAct.sa_handler = CAST_TO_FN_PTR(sa_handler_t, handler);

    if (sigaction(signal_number, &sigAct, &oldSigAct)) {
        // -1 means registration failed
        return (void *)-1;
    }

    return CAST_FROM_FN_PTR(void*, oldSigAct.sa_handler);
}

static volatile int sigint_count = 0;

/**
 * 信号处理器，OS监听到信号后转发至该处理器
 * @param sig
 * @param siginfo
 * @param context
 */
static void UserHandler(int sig, void *siginfo, void *context) {
    cout << "Interrupt signal (" << sig << ") received.\n" << endl;
    os::signal_notify(sig);
}

void *os::user_handler() {
    return CAST_FROM_FN_PTR(void*, UserHandler);
}

static int check_pending_signals(bool wait) {
    for(;;){
        for (int i = 0; i < NSIG + 1; i++) {
            int n = pending_signals[i];
            if (n > 0) {
                return i;
            }
        }
        if (!wait) {
            return -1;
        }
        sleep(5);
    }
}

int os::signal_wait() {
    return check_pending_signals(true);
}

/**
 * 通过pending_signals保留监听到的信息，交由专门的线程处理该信息
 * @param signal_number
 */
void os::signal_notify(int signal_number) {
    cout << "enter signal_notify sig:" << signal_number << endl;
    //将监听到的信号为竖为1
    //改操作应该为原子操作，确保不会被其他线程修改
    //JVM中混编平台相关汇编指令实现原子操作，演示代码中不研究相关实现
    pending_signals[signal_number] = 1;

//    for(int i = 0;i< NSIG + 1;i++){
//        cout << "i=" << i << " pending_signals= " << pending_signals[i] << endl;
//    }
//    ::SEM_POST(sig_sem);
    semaphore_signal(sig_sem);
}

