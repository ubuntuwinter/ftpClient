#ifndef COMMON_H
#define COMMON_H

enum WRITETYPE {
    DEFAULT,
    INFO,
    SEND,
    RECEIVE,
    DATA,
    WARNING,
    ERROR
};

enum DEALMODE {
    LIST,
    RETR,
    STOR
};

#endif // COMMON_H
