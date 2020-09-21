//
// Created by Xiang Shi on 2020/9/21.
//

#ifndef JVMATTACH_OS_H
#define JVMATTACH_OS_H


class os {
public:
    /**
     * JVM启动时，初始化信号监听
     */
    static void  signal_init();
    static void  signal_init_pd();
    static void* signal(int signal_number, void* handler);
    static void* user_handler();
    static void  signal_notify(int signal_number);
    static int   signal_wait();
};


#endif //JVMATTACH_OS_H
