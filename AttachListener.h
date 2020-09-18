//
// Created by Xiang Shi on 2020/9/18.
//

#ifndef JVMATTACH_ATTACHLISTENER_H
#define JVMATTACH_ATTACHLISTENER_H


#include "AttachOperation.h"
#include <sys/socket.h>
#include <zconf.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <cstdio>

extern int *__error(void);

#define errno (*__error())
#define RESTARTABLE(_cmd, _result) do { \
    _result = _cmd; \
  } while(((int)_result == -1) && (errno == 4))


#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX   sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

class AttachListener {
private:
    static int _listener;
    static bool _has_path;
    static char _path[UNIX_PATH_MAX];
    static void set_path(char* path);
    static void set_listener(int s);

public:
    static AttachOperation* dequeue();
    static int init();
    static char* path();
    static bool has_path();
    static int listener();
    static AttachOperation* read_request(int s);
    static int write_fully(int s, char* buf, int len);
    enum {
        ATTACH_PROTOCOL_VER = 1                     // protocol version
    };
    enum {
        ATTACH_ERROR_BADVERSION     = 101           // error codes
    };
};


#endif //JVMATTACH_ATTACHLISTENER_H
