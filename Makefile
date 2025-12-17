CC = gcc
CFLAGS = -Wall -Wextra -O2

SRCS = crc16.c manchester.c error.c
OBJS = $(SRCS:.c=.o)

all: sender receiver

sender: sender.c $(SRCS)
	$(CC) $(CFLAGS) -o sender sender.c $(SRCS)

receiver: receiver.c $(SRCS)
	$(CC) $(CFLAGS) -o receiver receiver.c $(SRCS)

.PHONY: server client clean

# Run receiver in foreground (use in terminal A)
server: receiver
	@echo "Starting receiver in foreground (press Ctrl+C to stop)..."
	./receiver

# Run sender in foreground (use in terminal B)
client: sender
	@echo "Starting sender in foreground (press Ctrl+C to stop)..."
	./sender

clean:
	-rm -f sender receiver *.o receiver.log receiver.pid