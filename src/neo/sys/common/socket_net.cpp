#include "idlib/precompiled.h"

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <sys/select.h>
#include <net/if.h>

static int Net_GetLastError()
{
    return errno;
}

#define D3_NET_EWOULDBLOCK   EWOULDBLOCK
#define D3_NET_ECONNRESET    ECONNRESET
#define D3_NET_EADDRNOTAVAIL EADDRNOTAVAIL

typedef int SOCKET;
#define closesocket(x) close(x)
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

idCvar net_socksServer("net_socksServer", "", CVAR_ARCHIVE, "");
idCvar net_socksPort( "net_socksPort", "1080", CVAR_ARCHIVE | CVAR_INTEGER, "" );
idCvar net_socksUsername( "net_socksUsername", "", CVAR_ARCHIVE, "" );
idCvar net_socksPassword( "net_socksPassword", "", CVAR_ARCHIVE, "" );

idCvar net_ip( "net_ip", "localhost", CVAR_NOCHEAT, "local IP address" );

static struct sockaddr_in socksRelayAddr;

static bool usingSocks = false;
static SOCKET socks_socket = 0;
static char socksBuf[4096];

typedef struct
{
    unsigned int ip;
    unsigned int mask;
    char addr[16];
} net_interface;

#define MAX_INTERFACES 32
int num_interfaces = 0;
net_interface netint[MAX_INTERFACES];

static void ip_to_addr( const char ip[4], char* addr )
{
	snprintf( addr, 16, "%d.%d.%d.%d", ( unsigned char )ip[0], ( unsigned char )ip[1],
					 ( unsigned char )ip[2], ( unsigned char )ip[3] );
}

void Sys_InitNetworking()
{
    bool foundloopback = false;

    int s;
    char buf[MAX_INTERFACES * sizeof(ifreq)];
    ifconf ifc;
    ifreq* ifr;
    int ifindex;
    unsigned int ip, mask;

    num_interfaces = 0;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    ifc.ifc_len = MAX_INTERFACES*sizeof(ifreq);
    ifc.ifc_buf = buf;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
    {
        common->Error("InitNetworking: SIOCGIFCONF error - %s\n", strerror(errno));
        return;
    }
    ifindex = 0;
    while (ifindex < ifc.ifc_len)
    {
        printf("Found interface %s - ", ifc.ifc_buf + ifindex);
        ifr = (ifreq*)(ifc.ifc_buf + ifindex);
        if( ioctl( s, SIOCGIFADDR, ifr ) < 0 )
		{
			printf( "SIOCGIFADDR failed: %s\n", strerror( errno ) );
		}
		else
        {
            if (ioctl(s, SIOCGIFADDR, ifr) < 0)
            {
                printf("not AF_INET\n");
            }
            else
            {
                ip = ntohl(*(unsigned int*)&ifr->ifr_addr.sa_data[2]);
                if (ip == INADDR_LOOPBACK)
                {
                    foundloopback = true;
                    printf("loopback\n");
                }
                else
                {
                    printf( "%d.%d.%d.%d",
                                        ( unsigned char )ifr->ifr_addr.sa_data[2],
                                        ( unsigned char )ifr->ifr_addr.sa_data[3],
                                        ( unsigned char )ifr->ifr_addr.sa_data[4],
                                        ( unsigned char )ifr->ifr_addr.sa_data[5] );
                }

                ip_to_addr(&ifr->ifr_addr.sa_data[2], netint[num_interfaces].addr);

                if (ioctl(s, SIOCGIFNETMASK, ifr) < 0)
                {
                    printf(" SIOCGIFNETMASK failed: %s\n", strerror(errno));
                }
                else
                {
                    mask = ntohl(*(unsigned int*)&ifr->ifr_addr.sa_data[2]);
                    if (ip != INADDR_LOOPBACK)
                    {
                        printf("/%d.%d.%d.%d\n",
                                            ( unsigned char )ifr->ifr_addr.sa_data[2],
                                            ( unsigned char )ifr->ifr_addr.sa_data[3],
                                            ( unsigned char )ifr->ifr_addr.sa_data[4],
                                            ( unsigned char )ifr->ifr_addr.sa_data[5] );
                    }
                    netint[num_interfaces].ip = ip;
                    netint[num_interfaces].mask = mask;
                    num_interfaces++;
                }
            }
        }
        ifindex += sizeof(ifreq);
    }
}