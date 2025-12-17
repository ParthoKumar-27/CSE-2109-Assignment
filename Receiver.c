// receiver.c  (server)
// Reads [4-byte big-endian length][encoded frame bytes]
// Uses packed manchester and robust recv_all

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "crc16.h"
#include "manchester.h"

#define LISTEN_PORT 8080
#define BACKLOG 4
#define MAX_FRAME (1 << 20) // 1 MiB limit

static void print_addr(struct sockaddr_in *a, char *out, size_t outlen)
{
    snprintf(out, outlen, "%s:%d", inet_ntoa(a->sin_addr), ntohs(a->sin_port));
}

static ssize_t recv_all(int sock, void *buf, size_t n)
{
    size_t recvd = 0;
    uint8_t *p = buf;
    while (recvd < n)
    {
        ssize_t r = recv(sock, p + recvd, n - recvd, 0);
        if (r == 0)
            return 0; // peer closed
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        recvd += (size_t)r;
    }
    return (ssize_t)recvd;
}

int main(void)
{
    int serv = socket(AF_INET, SOCK_STREAM, 0);
    if (serv < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(serv);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serv, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(serv);
        return 1;
    }
    if (listen(serv, BACKLOG) < 0)
    {
        perror("listen");
        close(serv);
        return 1;
    }

    printf("Server listening on port %d\n", LISTEN_PORT);

    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int cli = accept(serv, (struct sockaddr *)&client_addr, &client_len);
        if (cli < 0)
        {
            perror("accept");
            continue;
        }

        char addrstr[64];
        print_addr(&client_addr, addrstr, sizeof(addrstr));
        printf("\nClient connected: %s\n", addrstr);

        while (1)
        {
            uint32_t hdr_net;
            ssize_t r = recv_all(cli, &hdr_net, sizeof(hdr_net));
            if (r == 0)
            {
                printf("Client %s disconnected.\n", addrstr);
                break;
            }
            if (r < 0)
            {
                printf("recv header failed: %s\n", strerror(errno));
                break;
            }

            uint32_t enc_len = ntohl(hdr_net);
            if (enc_len == 0)
            {
                printf("Received zero-length frame: ignoring\n");
                continue;
            }
            if (enc_len > MAX_FRAME)
            {
                printf("Frame too large (%u) — closing connection\n", enc_len);
                break;
            }

            uint8_t *encbuf = malloc(enc_len);
            if (!encbuf)
            {
                printf("OOM allocating %u bytes\n", enc_len);
                break;
            }

            r = recv_all(cli, encbuf, enc_len);
            if (r == 0)
            {
                printf("Client %s disconnected mid-frame.\n", addrstr);
                free(encbuf);
                break;
            }
            if (r < 0)
            {
                printf("recv body failed: %s\n", strerror(errno));
                free(encbuf);
                break;
            }

            printf("\n--- Received %u bytes from %s ---\n", enc_len, addrstr);
            printf("----------------------------------------------\n\n");

            size_t dec_len = 0;
            uint8_t *dec = man_decode(encbuf, enc_len, &dec_len);
            free(encbuf);
            if (!dec)
            {
                fprintf(stderr, "man_decode returned NULL\n");
                continue;
            }
            if (dec_len < 2)
            {
                fprintf(stderr, "decoded length too small (%zu) — ignoring\n", dec_len);
                free(dec);
                continue;
            }

            uint16_t recv_crc = ((uint16_t)dec[dec_len - 2] << 8) | dec[dec_len - 1];
            uint16_t calc_crc = crc16(dec, dec_len - 2);

            printf("Decoded message (%zu bytes): ", dec_len - 2);
            for (size_t i = 0; i < dec_len - 2; i++)
                putchar(dec[i]);
            putchar('\n');

            printf("CRC Received:    0x%04X\n", recv_crc);
            printf("CRC Calculated:  0x%04X\n", calc_crc);

            if (recv_crc == calc_crc)
                printf("✔ NO ERROR DETECTED\n");
            else
                printf("✘ ERROR DETECTED!\n");

            free(dec);
            fflush(stdout);
        }

        //close(cli);
        close(serv);
        return 0;
    }

    close(serv);
    return 0;
}