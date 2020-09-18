//
// Created by Xiang Shi on 2020/9/18.
//

#include "AttachListener.h"
#include <sys/socket.h>
#include <zconf.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <cstdio>

extern int * __error(void);
#define errno (*__error())
#define RESTARTABLE(_cmd, _result) do { \
    _result = _cmd; \
  } while(((int)_result == -1) && (errno == 4))


#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX   sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

#define TEMP_PATH "/var/folders/qp/qtln82zj37l_rwmlwqrn9pzm0000gp/T"
#define CURRENT_PROCESS_ID ""

/**
 * 初始化domain socket监听
 * JVM通过监听指定线程文件文件
 * 获取client想JVM发送的指令并进行相关操作
 * @return
 */
int AttachListener::init() {
    char path[UNIX_PATH_MAX];          // socket file
    char initial_path[UNIX_PATH_MAX];  // socket file during setup
    int listener;                      // listener socket (file descriptor)

    int n = snprintf(path, UNIX_PATH_MAX, "%s/.java_pid%d",
            TEMP_PATH, CURRENT_PROCESS_ID);
    if (n < (int)UNIX_PATH_MAX) {
        n = snprintf(initial_path, UNIX_PATH_MAX, "%s.tmp", path);
    }
    if (n >= (int)UNIX_PATH_MAX) {
        return -1;
    }












}


AttachOperation* AttachListener::dequeue() {
    for (;;) {
        int s;

        // wait for client to connect
        struct sockaddr addr;
        socklen_t len = sizeof(addr);
        RESTARTABLE(::accept(-1, &addr, &len), s);
        if (s == -1) {
            return NULL;      // log a warning?
        }

        // get the credentials of the peer and check the effective uid/guid
        // - check with jeff on this.
        struct ucred cred_info;
        socklen_t optlen = sizeof(cred_info);
        if (::getsockopt(s, SOL_SOCKET, SO_PEERCRED, (void*)&cred_info, &optlen) == -1) {
            ::close(s);
            continue;
        }
        uid_t euid = geteuid();
        gid_t egid = getegid();

        if (cred_info.uid != euid || cred_info.gid != egid) {
            ::close(s);
            continue;
        }

        // peer credential look okay so we read the request
        LinuxAttachOperation* op = read_request(s);
        if (op == NULL) {
            ::close(s);
            continue;
        } else {
            return op;
        }
    }
}