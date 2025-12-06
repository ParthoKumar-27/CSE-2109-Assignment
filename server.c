#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "crc16.h"
#include "manchester.h"

int main()
{
    int serv = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serv, (struct sockaddr*)&addr, sizeof(addr));
    listen(serv, 1);

    printf("Server waiting...\n");

    int cli = accept(serv, NULL, NULL);

    uint8_t buf[4096];
    int n = recv(cli, buf, sizeof(buf), 0);

    size_t dec_len;
    uint8_t *dec = man_decode(buf, n, &dec_len);

    uint16_t recv_crc = (dec[dec_len-2] << 8) | dec[dec_len-1];
    uint16_t calc_crc = crc16(dec, dec_len - 2);

    printf("\nReceived message: ");
    for (int i = 0; i < dec_len - 2; i++)
        printf("%c", dec[i]);

    printf("\nCRC Received: 0x%04X", recv_crc);
    printf("\nCRC Calculated: 0x%04X\n", calc_crc);

    if (recv_crc == calc_crc)
        printf("\n✔ NO ERROR DETECTED\n");
    else
        printf("\n✘ ERROR DETECTED!\n");

    close(cli);
    close(serv);
}
