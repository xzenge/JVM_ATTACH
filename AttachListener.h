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
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define RESTARTABLE(_cmd, _result) do { \
    _result = _cmd; \
  } while(((int)_result == -1) && (errno == 4))


#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX   sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

class AttachListener {
private:

public:
    static AttachOperation *dequeue();

    static void init();

    static int pd_init();

    static AttachOperation *read_request(int s);

    static int write_fully(int s, char *buf, int len);

    enum {
        ATTACH_PROTOCOL_VER = 1                     // protocol version
    };
    enum {
        ATTACH_ERROR_BADVERSION = 101           // error codes
    };
    static int _listener;
    static bool _has_path;
    static char _path[UNIX_PATH_MAX];

    static void set_path(char *path) {
        if (path == NULL) {
            _has_path = false;
        } else {
            strncpy(_path, path, UNIX_PATH_MAX);
            _path[UNIX_PATH_MAX - 1] = '\0';
            _has_path = true;
        }
    }

    static void set_listener(int s) {
        _listener = s;
    }

    static char* path() { return _path; }

    static bool has_path() { return _has_path; }

    static int listener() { return _listener; }
};


#endif //JVMATTACH_ATTACHLISTENER_H
