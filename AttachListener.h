//
// Created by Xiang Shi on 2020/9/18.
//

#ifndef JVMATTACH_ATTACHLISTENER_H
#define JVMATTACH_ATTACHLISTENER_H


#include "AttachOperation.h"

class AttachListener {
public:
    static AttachOperation* dequeue();
    static int init();
};


#endif //JVMATTACH_ATTACHLISTENER_H
