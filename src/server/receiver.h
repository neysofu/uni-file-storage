#ifndef SOL_SERVER_RECEIVER
#define SOL_SERVER_RECEIVER

struct Receiver;

struct Receiver *
receiver_create(int socket_descriptor);

int
receiver_wait_read(struct Receiver *receiver);

void
receiver_free(struct Receiver *receiver);

int
receiver_add(struct Receiver *receiver, int fd);

#endif
