//
// Created by Xiang Shi on 2020/9/18.
//

#ifndef JVMATTACH_ATTACHOPERATION_H
#define JVMATTACH_ATTACHOPERATION_H


#include <cstring>

class AttachOperation {
public:
    enum {
        name_length_max = 16,       // maximum length of  name
        arg_length_max = 1024,      // maximum length of argument
        arg_count_max = 3           // maximum number of arguments
    };

private:
    char _name[name_length_max+1];
    char _arg[arg_count_max][arg_length_max+1];
    int _socket;


public:
    const char* name() const                      { return _name; }

    // set the operation name
    void set_name(char* name) {
        strcpy(_name, name);
    }

    // get an argument value
    const char* arg(int i) const {
        return _arg[i];
    }

    // set an argument value
    void set_arg(int i, char* arg) {
        if (arg == NULL) {
            _arg[i][0] = '\0';
        } else {
            strcpy(_arg[i], arg);
        }
    }

    void set_socket(int s)                                { _socket = s; }
    int socket() const                                    { return _socket; }

    // create an operation of a given name
    AttachOperation(char* name) {
        set_name(name);
        for (int i=0; i<arg_count_max; i++) {
            set_arg(i, NULL);
        }
    }
};


#endif //JVMATTACH_ATTACHOPERATION_H
