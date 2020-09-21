//
// Created by Xiang Shi on 2020/9/18.
//

#include "AttachListener.h"
#include <sys/socket.h>
#include <zconf.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <thread>
#include <iostream>

using namespace std;

#define TEMP_PATH "/var/folders/qp/qtln82zj37l_rwmlwqrn9pzm0000gp/T"
#define CURRENT_PROCESS_ID ""


class ArgumentIterator {
private:
    char *_pos;
    char *_end;
public:
    ArgumentIterator(char *arg_buffer, size_t arg_size) {
        _pos = arg_buffer;
        _end = _pos + arg_size - 1;
    }

    char *next() {
        if (*_pos == '\0') {
            return NULL;
        }
        char *res = _pos;
        char *next_pos = strchr(_pos, '\0');
        if (next_pos < _end) {
            next_pos++;
        }
        _pos = next_pos;
        return res;
    }
};

typedef int (*AttachOperationFunction)(AttachOperation* op);

struct AttachOperationFunctionInfo {
    const char* name;
    AttachOperationFunction func;
};

static int load_agent_library(AttachOperation* op) {
    cout << "enter load_agent_library" << endl;
    return 0;
}

static AttachOperationFunctionInfo funcs[] = {
        { "load", load_agent_library },
        { NULL,               NULL }
};

static void attach_listener_thread_entry() {

    if (AttachListener::pd_init() != 0) {
        return;
    }

    for (;;) {
        AttachOperation* op = AttachListener::dequeue();
        if (op == NULL) {
            return;   // dequeue failed or shutdown
        }

        // find the function to dispatch too
        AttachOperationFunctionInfo* info = NULL;
        for (int i=0; funcs[i].name != NULL; i++) {
            const char* name = funcs[i].name;
            if (strcmp(op->name(), name) == 0) {
                info = &(funcs[i]);
                break;
            }
        }

        if (info != NULL) {
            // dispatch to the function that implements this operation
            (info->func)(op);
        }
    }
}

void AttachListener::init() {
    std::thread t(&attach_listener_thread_entry);
}

//static int _listener;
//static bool _has_path;
//static char _path[UNIX_PATH_MAX];
int AttachListener::_listener = -1;
bool AttachListener::_has_path;
char AttachListener::_path[UNIX_PATH_MAX];


extern "C" {
static void listener_cleanup() {
    static int cleanup_done;
    if (!cleanup_done) {
        cleanup_done = 1;
        int s = AttachListener::listener();
        if (s != -1) {
            ::close(s);
        }
        if (AttachListener::has_path()) {
            ::unlink(AttachListener::path());
        }
    }
}
}

/**
 * 初始化domain socket监听
 * JVM通过监听指定线程文件文件
 * 获取client想JVM发送的指令并进行相关操作
 * @return
 */
int AttachListener::pd_init() {
    char path[UNIX_PATH_MAX];          // socket file
    char initial_path[UNIX_PATH_MAX];  // socket file during setup
    int listener;                      // listener socket (file descriptor)

    // register function to cleanup
    ::atexit(listener_cleanup);

    int n = snprintf(path, UNIX_PATH_MAX, "%s/.java_pid%d",
                     TEMP_PATH, CURRENT_PROCESS_ID);
    if (n < (int) UNIX_PATH_MAX) {
        n = snprintf(initial_path, UNIX_PATH_MAX, "%s.tmp", path);
    }
    if (n >= (int) UNIX_PATH_MAX) {
        return -1;
    }

    // create the listener socket
    listener = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if (listener == -1) {
        return -1;
    }

    // bind socket
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, initial_path);
    ::unlink(initial_path);
    int res = ::bind(listener, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1) {
        ::close(listener);
        return -1;
    }

    // put in listen mode, set permissions, and rename into place
    res = ::listen(listener, 5);
    if (res == 0) {
        RESTARTABLE(::chmod(initial_path, S_IREAD | S_IWRITE), res);
        if (res == 0) {
            // make sure the file is owned by the effective user and effective group
            // (this is the default on linux, but not on mac os)
            RESTARTABLE(::chown(initial_path, geteuid(), getegid()), res);
            if (res == 0) {
                res = ::rename(initial_path, path);
            }
        }
    }
    if (res == -1) {
        ::close(listener);
        ::unlink(initial_path);
        return -1;
    }
    set_path(path);
    set_listener(listener);

    return 0;
}


AttachOperation *AttachListener::dequeue() {
    for (;;) {
        int s;

        // wait for client to connect
        struct sockaddr addr;
        socklen_t len = sizeof(addr);
        RESTARTABLE(::accept(listener(), &addr, &len), s);
        if (s == -1) {
            return NULL;      // log a warning?
        }

        // get the credentials of the peer and check the effective uid/guid
        // - check with jeff on this.
        uid_t puid;
        gid_t pgid;
        if (::getpeereid(s, &puid, &pgid) != 0) {
            ::close(s);
            continue;
        }
        uid_t euid = geteuid();
        gid_t egid = getegid();

        if (puid != euid || pgid != egid) {
            ::close(s);
            continue;
        }

        // peer credential look okay so we read the request
        AttachOperation *op = read_request(s);
        if (op == NULL) {
            ::close(s);
            continue;
        } else {
            return op;
        }
    }
}

AttachOperation *AttachListener::read_request(int s) {
    char ver_str[8];
    int expected_str_count = 2 + AttachOperation::arg_count_max;
    const int max_len = (sizeof(ver_str) + 1) + (AttachOperation::name_length_max + 1) +
                        AttachOperation::arg_count_max * (AttachOperation::arg_length_max + 1);

    char buf[max_len];
    int str_count = 0;
    int off = 0;
    int left = max_len;

    do {
        int n;
        RESTARTABLE(read(s, buf + off, left), n);
        if (n == -1) {
            return NULL;      // reset by peer or other error
        }
        if (n == 0) {
            break;
        }
        for (int i = 0; i < n; i++) {
            if (buf[off + i] == 0) {
                // EOS found
                str_count++;

                // The first string is <ver> so check it now to
                // check for protocol mis-match
                if (str_count == 1) {
                    if ((strlen(buf) != strlen(ver_str)) ||
                        (atoi(buf) != ATTACH_PROTOCOL_VER)) {
                        char msg[32];
                        sprintf(msg, "%d\n", ATTACH_ERROR_BADVERSION);
                        write_fully(s, msg, strlen(msg));
                        return NULL;
                    }
                }
            }
        }
        off += n;
        left -= n;
    } while (left > 0 && str_count < expected_str_count);

    if (str_count != expected_str_count) {
        return NULL;        // incomplete request
    }

    // parse request
    ArgumentIterator args(buf, (max_len) - left);

    // version already checked
    char *v = args.next();

    char *name = args.next();
    if (name == NULL || strlen(name) > AttachOperation::name_length_max) {
        return NULL;
    }

    AttachOperation *op = new AttachOperation(name);

    for (int i = 0; i < AttachOperation::arg_count_max; i++) {
        char *arg = args.next();
        if (arg == NULL) {
            op->set_arg(i, NULL);
        } else {
            if (strlen(arg) > AttachOperation::arg_length_max) {
                delete op;
                return NULL;
            }
            op->set_arg(i, arg);
        }
    }

    op->set_socket(s);
    return op;
}

int AttachListener::write_fully(int s, char *buf, int len) {
    do {
        int n = ::write(s, buf, len);
        if (n == -1) {
            if (errno != EINTR) return -1;
        } else {
            buf += n;
            len -= n;
        }
    } while (len > 0);
    return 0;
}
