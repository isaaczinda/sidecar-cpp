/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <evhttp.h>

// from libevent
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

// standard libs
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>

#define MAX_LINE 16384

struct WriteProgress {
  std::string message;
  bool written;
  bool ready;
};

WriteProgress context;

/* just writes prints everything it receives
*/
void readcb(struct bufferevent *bev, void *ctx)
{
  WriteProgress *context = (WriteProgress*) ctx;

  int number_read;
  char buffer[1024];

  std::cout << "***read callback***\n";

  struct evbuffer *input = bufferevent_get_input(bev);

  size_t total_input_length = evbuffer_get_length(input);

  std::cout << "just received " << total_input_length << "\n";

  // keep reading until everything has been printed!
  while ((number_read = evbuffer_remove(input, buffer, sizeof(buffer))) > 0) {
      std::cout << buffer;
  }

  context->ready=true;
    //
    // char response_text[] = "this is a response";
    // struct evbuffer *output = bufferevent_get_output(bev);
    //
    // evbuffer_add(output, response_text, sizeof(response_text)); // TODO: does sizeof work here?
}

void write_callback(struct bufferevent *bev, void *ctx)
{
  WriteProgress *context = (WriteProgress*) ctx;

  if (!context->written && context->ready) {
    std::cout << "***writing response***\n";
    std::cout << context->message.c_str() << "\n";

    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_add(output, context->message.c_str(), context->message.length()); // TODO: does sizeof work here?
    bufferevent_write_buffer(bev, output); // write all this to the wire!

    context->written = true;
  }
}

void
errorcb(struct bufferevent *bev, short error, void *ctx)
{
  if (error & BEV_EVENT_CONNECTED) {
       std::cout << "connected\n";
  } else {
    if (error & BEV_EVENT_EOF) {
      std::cout << "connection closed\n";
        /* connection has been closed, do any clean up here */
        /* ... */
    } else if (error & BEV_EVENT_ERROR) {
      std::cout << "error\n";

        /* check errno to see what error occurred */
        /* ... */
    } else if (error & BEV_EVENT_TIMEOUT) {
      std::cout << "timeout\n";
        /* must be a timeout event handle, handle it */
        /* ... */
    }

    // we're done with this bufferevent becuase the connection closed
    bufferevent_free(bev);
  }
}

void
do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = (event_base*) arg;
    struct sockaddr_storage ss;



    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);

    std::cout << "*** do_accept got assigned" << fd << "***\n";

    if (fd < 0) {
        throw "Error accepting connection.";
    } else if (fd > FD_SETSIZE) {
        close(fd);
    } else {
        context.ready = false;
        context.written = false;

        // if everything worked as expected:
        struct bufferevent *bev;
        // set socket to nonblocking mode
        evutil_make_socket_nonblocking(fd);
        // create a new bufferevent
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        // read, write, signal, data
        bufferevent_setcb(bev, readcb, write_callback, errorcb, &context);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    }
}

void
run(void)
{
    evutil_socket_t listener; // socket itself
    struct sockaddr_in sin; // socket address
    struct event_base *base;
    struct event *listener_event;

    // create an event_base
    base = event_base_new();
    // if (!base)
    //     return; /*XXXerr*/

    // setup a listener
    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

#ifndef WIN32
    {
        int one = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }
#endif

    // setup socket in and bind listener
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(40713);
    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        throw "bind";

    // opens the socket for listening
    // 0 means success here
    if (listen(listener, 16) < 0) // TODO: why 16 ?
        throw "listen";

    // what does EV_PERSIST do ??
    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    event_add(listener_event, NULL); // NULL so that we add the event immediately

    // keeps running event loop until there are no more registered events
    event_base_dispatch(base);
}

int
main(int c, char **v)
{
  context.message = "HTTP/1.1 200 OK\r\n<html><h1>hello</h1></html>\r\n\r\n";

    setvbuf(stdout, NULL, _IONBF, 0); // TODO: figure out what this does

    run();
    return 0;
}
