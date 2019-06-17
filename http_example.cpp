/* PROBLEMS:

 - right now, http-proxy and lightstep build separately which means that they
   have to build protobuf twice. Protobuf notices when it has been built and
   linked twice and complains. We need to find a way to build protobuf once
   for these two projects.
 -

*/

#include <evhttp.h>
#include <iostream>
#include "collector.pb.h" // TODO: WOW this is jank...
#include <sstream>
#include <iostream>


//
// #include <opentracing/tracer.h>
// #include <lightstep/tracer.h>

#define SATELLITE_IP "0.0.0.0"
#define SATELLITE_PORT 1000

#define BINARY_CONTENT_TYPE "application/octet-stream"

typedef google::protobuf::RepeatedPtrField< lightstep::collector::KeyValue > KeyValueDict;

void evbuffer_to_stringstream(evbuffer * ev_buffer, std::stringstream * stream)
{
  size_t ev_buffer_len;

  ev_buffer_len = evbuffer_get_length(ev_buffer);

  char * char_buffer = new char[ev_buffer_len];

  evbuffer_copyout(ev_buffer, char_buffer, ev_buffer_len);
  evbuffer_drain(ev_buffer, ev_buffer_len);

  stream->write(char_buffer, sizeof(char) * ev_buffer_len);
}

void print_keyvaluetags(KeyValueDict dict)
{
  for (int i = 0; i < dict.size(); i++)
  {
    if (i != 0) { std::cout << ", "; }
    std::cout << "{" << dict[i].key() << ": " << dict[i].string_value() << "}";
  }

  std::cout << std::endl;
}

void process_request(struct evhttp_request *req, void *arg){
    std::stringstream request_stream;

    struct evbuffer *request_buf = evhttp_request_get_input_buffer(req);
    struct evbuffer *response_buf = evbuffer_new();

    if (request_buf == NULL) {
      std::cerr << "Unable to get the request data buffer." << std::endl;
      return;
    }

    if (response_buf == NULL) {
      std::cerr << "Unable to get the response data buffer." << std::endl;
    }

    // get the headers, and print them !
    const char content_type_tag[] = "Content-Type";
    struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
    const char* content_type = evhttp_find_header(headers, content_type_tag);

    // if Content-Type is not application/octet-stream, we won't be able to
    // process the data
    if (content_type == NULL) {
      std::cerr << "No Content-Type header.";
      return;
    } else {
      std::string s(content_type);
      if (s != BINARY_CONTENT_TYPE) {
        std::cerr << "Application type " << s << " was not " << BINARY_CONTENT_TYPE << "." << std::endl;
      }
    }

    std::cout << "Received " << evbuffer_get_length(request_buf) << " bytes" << std::endl;

    // move from evbuffer --> stringstream
    evbuffer_to_stringstream(request_buf, &request_stream);

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    lightstep::collector::ReportRequest report_request;

    if (!report_request.ParseFromIstream(&request_stream)) {
      std::cerr << "There was a problem parsing the protobuf encoded binary." << std::endl;
      return;
    }

    if (report_request.spans().size() > 0) {
      std::string access_token = report_request.auth().access_token();
      lightstep::collector::Reporter reporter = report_request.reporter();

      // TODO: somehow operation_name is becoming confused with access_token
      std::cout << "number of spans: " << report_request.spans().size() << std::endl;
      print_keyvaluetags(reporter.tags());


    }


    evbuffer_add_printf(response_buf, "Requested: %s\n", evhttp_request_uri(req));
    evhttp_send_reply(req, HTTP_OK, "OK", response_buf);
}

int main () {
    struct event_base *base = NULL;
    struct evhttp *httpd = NULL;
    base = event_init();
    if (base == NULL) return -1;
    httpd = evhttp_new(base);
    if (httpd == NULL) return -1;
    if (evhttp_bind_socket(httpd, "0.0.0.0", 12345) != 0) return -1;
    evhttp_set_gencb(httpd, process_request, NULL);
    event_base_dispatch(base);
    return 0;
}
