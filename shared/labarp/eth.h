#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

/*Longitud del payload*/
#define ETHER_TYPE 60
/*Con 2000 bytes son suficientes para la trama, ya que va de 64 a 1518*/
#define BUF_SIZ 2000

/*Tipo de dato sin signo*/
typedef unsigned char byte;

void ConvierteMAC(char *Mac, char *Org)
{ /*En lugar de caracteres, se requiere el numero*/
  int i, j, Aux, Acu;
  for (i = 0, j = 0, Acu = 0; i < 12; i++)
  {
    if ((Org[i] > 47) && (Org[i] < 58))
      Aux = Org[i] - 48;
    if ((Org[i] > 64) && (Org[i] < 97))
      Aux = Org[i] - 55;

    if (Org[i] > 96)
      Aux = Org[i] - 87;
    if ((i % 2) == 0)
      Acu = Aux * 16;
    else
      Mac[j] = Acu + Aux;
    j++;
  }
}
