#include "eth.h"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
  int sockfd;
  struct ifreq if_idx, if_mac,ifr;
  int i, iLen, iLenHeader, iLenTotal;
  byte sendbuf[BUF_SIZ], Mac[6];
  pid_t pid;
  int Salir;
  char UserMAC[100];
  int saddr_size;
  struct sockaddr saddr;
  ssize_t numbytes;
  /*La cabecera Ethernet (eh) y sendbuf apuntan a lo mismo*/
  struct ether_header *eh = (struct ether_header *)sendbuf;
  struct sockaddr_ll socket_address;
  if (argc != 3)
  {
    printf("Error en argumentos.\n\n");
    printf("labarp INTERFACE-SALIDA NOMBRE (Formato XXXXXXXXXXXX).\n");
    printf("El nombre es cualquier cadena y la proporciona el usuario.\n");
    printf("Ejemplo: labarp eth0 pcX\n\n");

    exit(1);
  }
  /*Apartir de este este punto, argv[1] = Nombre de la interfaz, */
  /*y argv[2] posee el nombre, de capa 3, que se le asigna a la  */
  /*maquina virtual.                                             */

  /*Vamos a crear dos procesos: padre e hijo. Cuando se ejecuta  */
  /*la siguiente linea se crean dos procesos, el proceso hijo es */
  /*tal que pid = 0, en cambio el proceso padre pid != 0.        */
  /*El proceso padre sera el encargado de preguntar por una MAC, */
  /*en cambio, el proceso hijo debera responder.                 */

  pid = fork();
  if (pid != 0)
  { /*Soy el proceso padre*/
    printf("Soy el proceso padre y voy a preguntar por la MAC dado un nombre.\n");

    /*Abre el socket. Para que sirven los parametros empleados?*/
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
      perror("socket");

    /* Mediante el nombre de la interface, se obtiene su indice */
    memset(&if_idx, 0, sizeof(struct ifreq)); /*Llena de ceros la estructura if_idx*/

    for (i = 0; argv[1][i]; i++)
      if_idx.ifr_name[i] = argv[1][i];

    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
      perror("SIOCGIFINDEX"); /*Toma el control del driver*/

    /*Ahora obtenemos la MAC de la interface por donde saldran los datos */
    memset(&if_mac, 0, sizeof(struct ifreq));

    for (i = 0; argv[1][i]; i++)
      if_mac.ifr_name[i] = argv[1][i];

    if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
      perror("SIOCGIFHWADDR");

    /*Recibimos en la variable el nombre de la pc destino*/
    char *nombre_pc_destino[5];
    nombre_pc_destino[0] = argv[2];
    printf("Busco a %s\n", nombre_pc_destino[0]);
    //Encontramosm en que red estamos
    strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);
    printf("Estoy en la red %s\n",ifr.ifr_name);

    // Obtener nombre de host de origen
    FILE *fp = popen("hostname", "r");
    char nombre_pc_origen[256];
    if (fp == NULL || fgets(nombre_pc_origen, sizeof(nombre_pc_origen), fp) == NULL)
    {
      perror("hostname retrieval failed");
      exit(1);
    }
    nombre_pc_origen[strcspn(nombre_pc_origen, "\n")] = '\0';
    pclose(fp);
    printf("El host origen es: %s\n",nombre_pc_origen);
    /*Se imprime la MAC del host*/
    printf("Iterface de salida: %u, con MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (byte)(if_idx.ifr_ifindex),
           (byte)(if_mac.ifr_hwaddr.sa_data[0]), (byte)(if_mac.ifr_hwaddr.sa_data[1]),
           (byte)(if_mac.ifr_hwaddr.sa_data[2]), (byte)(if_mac.ifr_hwaddr.sa_data[3]),
           (byte)(if_mac.ifr_hwaddr.sa_data[4]), (byte)(if_mac.ifr_hwaddr.sa_data[5]));

    Salir = 0;
    while (!Salir)
    {
      printf("Direccion MAC (XXXXXXXXXXXX): ");
      fgets(UserMAC, 100, stdin);
      iLen = strlen(UserMAC);
      for (i = 0; i < iLen; i++)
        if (UserMAC[i] == 10)
          UserMAC[i] = 0;
      if (!strcmp(UserMAC, "exit"))
      {
        close(sockfd);
        kill(pid, 9);
        return (0);
      }
      /*Ahora se construye la trama Ethernet empezando por su encabezado. El   */
      /*formato, para la trama IEEE 802.3 version 2, es:                       */
      /*6         bytes de MAC Origen                                          */
      /*6         bytes de MAC Destino                                         */
      /*2         bytes para longitud de la trama o Ether_Type                 */
      /*46 a 1500 bytes de payload                                             */
      /*4         bytes Frame Check Sequence                                   */
      /*Total sin contar bytes de sincronizacion, va de 64 a 1518 bytes.       */
      memset(sendbuf, 0, BUF_SIZ); /*Llenamos con 0 el buffer de datos (payload)*/
      /*Direccion MAC Origen*/
      eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
      eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
      eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
      eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
      eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
      eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
      /*Direccion MAC destino*/
      ConvierteMAC(Mac, UserMAC);
      eh->ether_dhost[0] = Mac[0];
      eh->ether_dhost[1] = Mac[1];
      eh->ether_dhost[2] = Mac[2];
      eh->ether_dhost[3] = Mac[3];
      eh->ether_dhost[4] = Mac[4];
      eh->ether_dhost[5] = Mac[5];
      /*Tipo de protocolo o la longitud del paquete*/
      eh->ether_type = htons(ETHER_TYPE);

      /*La carga util ustedes la definen*/
      iLenHeader = sizeof(struct ether_header);
      for (i = 0; i < ETHER_TYPE; i++)
        sendbuf[iLenHeader + i] = 65 + i;

      /*Finalmente FCS*/
      for (i = 0; i < 4; i++)
        sendbuf[iLenHeader + i] = 0;

      iLenTotal = iLenHeader + ETHER_TYPE + 4;

      socket_address.sll_ifindex = if_idx.ifr_ifindex;
      socket_address.sll_halen = ETH_ALEN;
      socket_address.sll_addr[0] = Mac[0];
      socket_address.sll_addr[1] = Mac[1];
      socket_address.sll_addr[2] = Mac[2];
      socket_address.sll_addr[3] = Mac[3];
      socket_address.sll_addr[4] = Mac[4];
      socket_address.sll_addr[5] = Mac[5];

      /*Envio del paquete*/
      iLen = sendto(sockfd, sendbuf, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
      if (iLen < 0)
        printf("Send failed\n");
      printf("Se ha enviado un paquete de %d bytes de payload.\n", iLenTotal);
    }
  }

  else
  { /*Proceso hijo*/
    printf("Soy el proceso hijo y voy a responder mi MAC dado mi nombre.\n");
    if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
    {
      perror("Listener: socket");
      return -1;
    }

    /*Ahora obtenemos la MAC de la interface del host*/
    memset(&if_mac, 0, sizeof(struct ifreq));
    for (i = 0; argv[1][i]; i++)
      if_mac.ifr_name[i] = argv[1][i];
    if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
      perror("SIOCGIFHWADDR");

    /*Se imprime la MAC del host*/
    printf("Direccion MAC de la interfaz de entrada: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (byte)(if_mac.ifr_hwaddr.sa_data[0]), (byte)(if_mac.ifr_hwaddr.sa_data[1]),
           (byte)(if_mac.ifr_hwaddr.sa_data[2]), (byte)(if_mac.ifr_hwaddr.sa_data[3]),
           (byte)(if_mac.ifr_hwaddr.sa_data[4]), (byte)(if_mac.ifr_hwaddr.sa_data[5]));
    do
    { /*Capturando todos los paquetes*/
      saddr_size = sizeof saddr;
      numbytes = recvfrom(sockfd, sendbuf, 65536, 0, &saddr, (socklen_t *)(&saddr_size));
      /*Recibe todo.*/
      printf("**********************************************\n");
      printf("Llego paquete de %d bytes: \n", numbytes);
      printf("Host Destino: %02x:%02x:%02x:%02x:%02x:%02x\n",
             eh->ether_dhost[0], eh->ether_dhost[1], eh->ether_dhost[2],
             eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5]);
      printf("Host Fuente: %02x:%02x:%02x:%02x:%02x:%02x\n",
             eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2],
             eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5]);
      printf("Ether Type: %04X\n", htons(eh->ether_type));
      printf("Contenido completo de la trama: \n");
      for (i = 0; i < numbytes; i++)
        printf("%02x ", sendbuf[i]);
      printf("\n");
    } while (1);
    close(sockfd);
  }
  return 0;
}
