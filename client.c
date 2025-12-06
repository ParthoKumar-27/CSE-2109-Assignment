#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include<time.h>
#include "crc16.h"
#include "manchester.h"
#include "error.h"

int main()
{
    srand(time(NULL));

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    char msg[256];
    printf("Enter message: ");
    scanf("%s", msg);

    size_t len = strlen(msg);

    // CRC attach
    uint16_t crc = crc16((uint8_t*)msg, len);

    uint8_t *packet = malloc(len + 2);
    memcpy(packet, msg, len);
    packet[len]   = crc >> 8;
    packet[len+1] = crc & 0xFF;

    // Manchester encoding
    size_t enc_len;
    uint8_t *encoded = man_encode(packet, len+2, &enc_len);

    printf("\nError Options:\n");
    printf("0 = No error\n");
    printf("1 = Single bit\n");
    printf("2 = Two isolated bits\n");
    printf("3 = Odd (3 bits)\n");
    printf("4 = Burst 8\n");
    printf("5 = Burst 17\n");
    printf("6 = Burst 22\n> ");

    int c;
    scanf("%d", &c);

    size_t bits = enc_len;

    switch(c)
    {
        case 1: inject_single(encoded, bits); break;
        case 2: inject_two(encoded, bits); break;
        case 3: inject_odd(encoded, bits, 3); break;
        case 4: inject_burst(encoded, bits, 8); break;
        case 5: inject_burst(encoded, bits, 17); break;
        case 6: inject_burst(encoded, bits, 22); break;
    }

    send(sock, encoded, enc_len, 0);

    printf("\nSent encoded data (%ld bits).\n", enc_len);

    close(sock);
    return 0;
}
