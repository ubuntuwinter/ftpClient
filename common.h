#ifndef COMMON_H
#define COMMON_H

enum WRITETYPE {
    DEFAULT = 0,
    INFO,
    SEND,
    RECEIVE,
    DATA,
    WARNING,
    ERROR
};

enum DEALMODE {
    LIST = 0,
    RETR,
    STOR
};

enum LIST {
    PERMISSION = 0,
    N_LINK,
    USER_ID,
    GROUP_ID,
    SIZE,
    MONTH,
    DAY,
    TIME,
    NAME
};

#endif // COMMON_H
