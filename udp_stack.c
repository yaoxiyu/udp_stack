#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>

#define NETMAP_WITH_LIBS

#include <net/netmap_user.h>

#pragma pack(1)

#define PROTO_IP    0x0800
#define PROTO_UDP   17
#define ETH_LENGTH  6

// 以太网协议头
struct ethhdr
{
    unsigned char dest[ETH_LENGTH]; // 目标mac地址 6byte
    unsigned char src[ETH_LENGTH];  // 源mac地址 6byte
    unsigned short proto;           // 类型 2byte
};

// ip协议头
struct iphdr
{
    unsigned char version:4,        // 版本 4bit ipv4/ipv6
                  hdrlen:4;         // 长度 4bit
    unsigned char tos;              // 服务类型 8bit
    unsigned short tot_len;         // 总长度 16bit
    unsigned short id;              // 标识位 16bit ip分片用
    unsigned short flag:3,          // 标志 3bit
                   offset:13;       // 片偏移 13bit
    unsigned char ttl;              // 生存时间 8bit
    unsigned char proto;            // 协议 8bit
    unsigned short check;           // 首部检验和 16bit
    unsigned int sip;               // 源ip地址 32bit
    unsigned int dip;               // 目的ip地址 32bit
};

// udp协议头
struct udphdr
{
    unsigned short sport;           // 源端口 2byte
    unsigned short dport;           // 目标端口 2byte
    unsigned short length;          // 长度 2byte
    unsigned short check;           // 校验 2byte
};

// udp数据报
struct udppkt
{
    struct ethhdr eh;
    struct iphdr ih;
    struct udphdr uh;
    
    /*
     * 柔性数组 零长数组
     * 1. 对于数组长度不确定
     * 2. 提前分配好
     * 3. 通过计算可以得出数组长度
     */
    unsigned char body[0];          
};

int main()
{
    struct nm_desc *nmr = nm_open("netmap:eth0", NULL, 0, NULL);
    if (nmr == NULL)
    {
        return -1;
    }

    struct pollfd pfd = {0};
    pfd.fd = nmr->fd;
    pfd.events = POLLIN;
    while(1)
    {
        int ret = poll(&pfd, 1, -1);
        if (ret < 0)
        {
            continue;
        }
        if (pfd.events & POLLIN)
        {
            struct nm_pkthdr h;
            unsigned char *stream = nm_nextpkt(nmr, &h);
            struct ethhdr *eh = (struct ethhdr*)stream;
            
            // network --> host
            if (ntohs(eh->proto) == PROTO_IP)
            {
                struct udppkt *up = (struct udppkt*)stream;
                if (ntohs(up->ih.proto) == PROTO_UDP)
                {
                    int udp_length = ntohs(up->uh.length);
                    up->body[udp_length - 8] = '\0';
                    printf("udp --> %s", up->body);
                }
            }
        }
    }
    return 0;
}