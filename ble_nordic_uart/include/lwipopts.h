#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Minimal lwIP configuration for Pico W
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define MEM_LIBC_MALLOC             1
#define MEMP_MEM_MALLOC             1
#define LWIP_NETCONN                0
#define LWIP_DHCP                   1
#define LWIP_UDP                    1
#define LWIP_TCP                    1
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_PCB            4
#define PBUF_POOL_SIZE              8

#endif /* _LWIPOPTS_H */