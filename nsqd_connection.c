#include "nsq.h"

#ifdef DEBUG
#define _DEBUG(...) fprintf(stdout, __VA_ARGS__)
#else
#define _DEBUG(...) do {;} while (0)
#endif

static void nsqd_connection_read_size(struct BufferedSocket *buffsock, void *arg);
static void nsqd_connection_read_data(struct BufferedSocket *buffsock, void *arg);
static void nsqd_connection_error_cb(struct BufferedSocket *buffsock, void *arg);
static void nsqd_connection_connect_cb(struct BufferedSocket *buffsock, void *arg)
{
    struct NSQDConnection *conn = (struct NSQDConnection *)arg;

    _DEBUG("%s: %p\n", __FUNCTION__, arg);

    // send magic
    buffered_socket_write(conn->bs, "  V2", 4);

    if (conn->connect_callback) {
        conn->connect_callback(conn, conn->arg);
    }

    buffered_socket_read_bytes(buffsock, 4, nsqd_connection_read_size, conn);
}

static void nsqd_connection_read_size(struct BufferedSocket *buffsock, void *arg)
{
    struct NSQDConnection *conn = (struct NSQDConnection *)arg;
    uint32_t *msg_size_be;

    _DEBUG("%s: %p\n", __FUNCTION__, arg);

    msg_size_be = (uint32_t *)buffsock->read_buf->data;
    buffer_drain(buffsock->read_buf, 4);

    // convert message length header from big-endian
    conn->current_msg_size = ntohl(*msg_size_be);

    _DEBUG("%s: msg_size = %d bytes %p\n", __FUNCTION__, conn->current_msg_size, buffsock->read_buf->data);

    buffered_socket_read_bytes(buffsock, conn->current_msg_size, nsqd_connection_read_data, conn);
}

static void nsqd_connection_read_data(struct BufferedSocket *buffsock, void *arg)
{
    struct NSQDConnection *conn = (struct NSQDConnection *)arg;
    struct NSQMessage *msg;

    conn->current_frame_type = ntohl(*((uint32_t *)buffsock->read_buf->data));
    buffer_drain(buffsock->read_buf, 4);
    conn->current_msg_size -= 4;

    printf("%s: frame type %d, data: %.*s\n", __FUNCTION__, conn->current_frame_type,
        conn->current_msg_size, buffsock->read_buf->data);

    conn->current_data = buffsock->read_buf->data;
    switch (conn->current_frame_type) {
        case NSQ_FRAME_TYPE_RESPONSE:
            if (strncmp(conn->current_data, "_heartbeat_", 11) == 0) {
                printf("get the heartbeat signal\n");
                buffer_reset(conn->command_buf);
                nsq_nop(conn->command_buf);
                buffered_socket_write_buffer(conn->bs, conn->command_buf);
                buffer_drain(buffsock->read_buf, 11);

                buffered_socket_read_bytes(buffsock, 4, nsqd_connection_read_size, conn);
                return;
            }
            break;
        case NSQ_FRAME_TYPE_MESSAGE:
            msg = nsq_decode_message(conn->current_data, conn->current_msg_size);
            if (conn->msg_callback) {
                conn->msg_callback(conn, msg, conn->arg);
            }
            break;
    }

    buffer_drain(buffsock->read_buf, conn->current_msg_size);

    buffered_socket_read_bytes(buffsock, 4, nsqd_connection_read_size, conn);
}



static void nsqd_connection_close_cb(struct BufferedSocket *buffsock, void *arg)
{
    struct NSQDConnection *conn = (struct NSQDConnection *)arg;

    _DEBUG("%s: %p\n", __FUNCTION__, arg);

    if (conn->close_callback) {
        conn->close_callback(conn, conn->arg);
    }
}

static void nsqd_reconnect(struct NSQDConnection *conn){
    conn->bs = new_buffered_socket(conn->loop, conn->address, conn->port,
        nsqd_connection_connect_cb, nsqd_connection_close_cb,
        NULL, NULL, nsqd_connection_error_cb,
        conn);
}

/*
Using this function to tell the main process that the socket is disconnected
*/
static void nsqd_connection_error_cb(struct BufferedSocket *buffsock, void *arg)
{
    struct NSQDConnection *conn = (struct NSQDConnection *)arg;
    //printf("reconnecting\n");
    //nsqd_reconnect(conn);
    conn->disconnect_callback(conn);
    _DEBUG("%s: conn %p\n", __FUNCTION__, conn);
}

struct NSQDConnection *new_nsqd_connection(struct ev_loop *loop, const char *address, int port,
    void (*connect_callback)(struct NSQDConnection *conn, void *arg),
    void (*close_callback)(struct NSQDConnection *conn, void *arg),
    void (*msg_callback)(struct NSQDConnection *conn, struct NSQMessage *msg, void *arg),
    void (*disconnect_callback)(struct NSQDConnection *conn),
    void *arg)
{
    struct NSQDConnection *conn;

    conn = malloc(sizeof(struct NSQDConnection));
    conn->command_buf = new_buffer(4096, 4096);
    conn->address=address;
    conn->port=port;
    conn->current_msg_size = 0;
    conn->connect_callback = connect_callback;
    conn->close_callback = close_callback;
    conn->msg_callback = msg_callback;
    conn->arg = arg;//the reader pointer
    conn->loop = loop;
    conn->disconnect_callback=disconnect_callback;
    conn->bs = new_buffered_socket(loop, address, port,
        nsqd_connection_connect_cb, nsqd_connection_close_cb,
        NULL, NULL, nsqd_connection_error_cb,
        conn);

    return conn;
}


void free_nsqd_connection(struct NSQDConnection *conn)
{
    if (conn) {
        free_buffer(conn->command_buf);
        free_buffered_socket(conn->bs);
        free(conn);
    }
}

int nsqd_connection_connect(struct NSQDConnection *conn)
{
    return buffered_socket_connect(conn->bs);
}

void nsqd_connection_disconnect(struct NSQDConnection *conn)
{
    buffered_socket_close(conn->bs);
    conn->disconnect_callback(conn);
}
