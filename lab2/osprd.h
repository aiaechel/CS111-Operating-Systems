#ifndef OSPRD_H
#define OSPRD_H

// ioctl constants
#define OSPRDIOCACQUIRE		42
#define OSPRDIOCTRYACQUIRE	43
#define OSPRDIOCRELEASE		44
#define OSPRDSETPASS            45
#define OSPRDAUTHORIZE          46
#define OSPRDDEAUTHORIZE        47
#define OSPRDPASSEXISTS         48

#define MAX_PASS_LEN            17
#define KEY_LENGTH              64

#define AES_KEY_LENGTH          16
#endif
