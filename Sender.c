// sender.c  (client)
// Sends: [4-byte big-endian length][encoded frame bytes]
// Uses packed Manchester (manchester.c), crc16.c and error.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdint.h>

#include "crc16.h"
#include "manchester.h"
#include "error.h"

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080

static void trim_input(char *s)
{
    size_t l = strlen(s);
    while (l > 0 && (s[l - 1] == '\n' || s[l - 1] == '\r'))
        s[--l] = '\0';
}

static ssize_t send_all(int sock, const void *buf, size_t len)
{
    size_t sent = 0;
    const uint8_t *p = buf;
    while (sent < len)
    {
        ssize_t s = send(sock, p + sent, len - sent, 0);
        if (s < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        sent += (size_t)s;
    }
    return (ssize_t)sent;
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    const char *server_ip = DEFAULT_SERVER_IP;
    int server_port = DEFAULT_SERVER_PORT;
    if (argc >= 2)
        server_ip = argv[1];
    if (argc >= 3)
        server_port = atoi(argv[2]);

    int sock = -1;
    char line[1024];

    for (;;)
    {
        if (sock < 0)
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror("socket");
                return 1;
            }

            struct sockaddr_in server;
            memset(&server, 0, sizeof(server));
            server.sin_family = AF_INET;
            server.sin_port = htons(server_port);
            if (inet_pton(AF_INET, server_ip, &server.sin_addr) <= 0)
            {
                fprintf(stderr, "Invalid server IP: %s\n", server_ip);
                close(sock);
                return 1;
            }

            if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
            {
                close(sock);
                sock = -1;
                printf("No server available: %s\n", strerror(errno));
                return 0;
            }

            printf("✔ Connected to %s:%d\n", server_ip, server_port);
        }

        /* Message prompt */
        for (;;)
        {
            printf("\nEnter message (type 'exit' to quit): ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin))
            {
                close(sock);
                sock = -1;
                printf("Exiting client.\n");
                return 0;
            }
            trim_input(line);
            if (line[0] == '\0')
            {
                printf("Empty message — please enter a non-empty message or type 'exit'.\n");
                continue;
            }
            if (strcmp(line, "exit") == 0)
            {
                close(sock);
                sock = -1;
                printf("Exiting client.\n");
                return 0;
            }
            break;
        }

        size_t len = strlen(line);
        uint16_t crc = crc16((const uint8_t *)line, len);

        uint8_t *packet = malloc(len + 2);
        if (!packet)
        {
            fprintf(stderr, "malloc failed\n");
            close(sock);
            return 1;
        }
        memcpy(packet, line, len);
        packet[len] = (uint8_t)(crc >> 8);
        packet[len + 1] = (uint8_t)(crc & 0xFF);

        size_t enc_len;
        uint8_t *encoded = man_encode(packet, len + 2, &enc_len);
        free(packet);
        if (!encoded)
        {
            fprintf(stderr, "man_encode failed\n");
            close(sock);
            return 1;
        }

        int c = -1;
        for (;;)
        {
            printf("\nError Options:\n");
            printf("0 = No error\n1 = Single bit\n2 = Two isolated bits\n3 = Odd (3 bits)\n4 = Burst 8\n5 = Burst 17\n6 = Burst 22\n> ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin))
            {
                c = 0;
                break;
            }
            trim_input(line);
            char *endptr = NULL;
            long tmp = strtol(line, &endptr, 10);
            if (endptr == line || *endptr != '\0')
            {
                printf("Invalid input: enter number 0..6\n");
                continue;
            }
            c = (int)tmp;
            if (c < 0 || c > 6)
            {
                printf("Invalid input: enter number 0..6\n");
                continue;
            }
            break;
        }

        size_t bits = enc_len * 8;
        size_t *positions = NULL;
        if (bits > 0)
            positions = malloc(bits * sizeof(size_t));

        size_t flipped_count = 0;
        switch (c)
        {
        case 0:
            flipped_count = 0;
            break;
        case 1:
            flipped_count = inject_single(encoded, bits, positions, bits);
            break;
        case 2:
            flipped_count = inject_two(encoded, bits, positions, bits);
            break;
        case 3:
            flipped_count = inject_odd(encoded, bits, 3, positions, bits);
            break;
        case 4:
            flipped_count = inject_burst(encoded, bits, 8, positions, bits);
            break;
        case 5:
            flipped_count = inject_burst(encoded, bits, 17, positions, bits);
            break;
        case 6:
            flipped_count = inject_burst(encoded, bits, 22, positions, bits);
            break;
        default:
            flipped_count = 0;
            break;
        }

        if (flipped_count == 0)
        {
            printf("No bits flipped (no error injection).\n");
        }
        else
        {
            if(flipped_count==8 || flipped_count==17 || flipped_count==22)
            {
                 printf("Flipped %zu bit(s)", flipped_count);
            }else if (positions && flipped_count <= 64 && flipped_count!=8 && flipped_count !=17 && flipped_count !=22)
            {
                printf("Flipped %zu bit(s)", flipped_count);
                printf(" at position(s):");
                for (size_t i = 0; i < flipped_count; i++)
                {
                    printf(" %zu", positions[i]);
                    if (i + 1 < flipped_count)
                        printf(",");
                }
            }
            printf("\n");
        }

        uint32_t hdr_net = htonl((uint32_t)enc_len);
        if (send_all(sock, &hdr_net, sizeof(hdr_net)) < 0)
        {
            perror("send header");
            free(encoded);
            free(positions);
            close(sock);
            sock = -1;
            continue;
        }
        if (send_all(sock, encoded, enc_len) < 0)
        {
            perror("send body");
            free(encoded);
            free(positions);
            close(sock);
            sock = -1;
            continue;
        }

        free(encoded);
        free(positions);
        printf("\nSent encoded data (%zu bytes).\n", enc_len);
    }
}