/* PROBLEMS:

boost: 1.70.0
*/

// external includes
#include <evhttp.h>
#include <opentracing/tracer.h>

// system includes
#include <iostream>
#include <sstream>
#include <iostream>
#include <map>

// protobuf includes
#include <collector.pb.h>


#define SATELLITE_ADDRESS "localhost"
#define SATELLITE_PORT 8360

#define BINARY_CONTENT_TYPE "application/octet-stream"

void evbuffer_to_stringstream(evbuffer * ev_buffer, std::stringstream * stream)
{
  size_t ev_buffer_len;

  ev_buffer_len = evbuffer_get_length(ev_buffer);

  char * char_buffer = new char[ev_buffer_len];

  evbuffer_copyout(ev_buffer, char_buffer, ev_buffer_len);
  evbuffer_drain(ev_buffer, ev_buffer_len);

  stream->write(char_buffer, sizeof(char) * ev_buffer_len);
}

// void print_keyvaluetags(KeyValueDict dict)
// {
//   for (int i = 0; i < dict.size(); i++)
//   {
//     if (i != 0) { std::cout << ", "; }
//     std::cout << "{" << dict[i].key() << ": " << dict[i].string_value() << "}";
//   }
//
//   std::cout << std::endl;
// }

void process_request(struct evhttp_request *req, void *arg) {
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

    auto spans = report_request.spans();

    if (spans.size() > 0) {
      // std::string access_token = report_request.auth().access_token();
      // lightstep::collector::Reporter reporter = report_request.reporter();
      //       std::string component_name = reporter.;
      // int reporter_id = reporter.reporter_id();

      // auto timestamp = FromTimestamp(spans[0].start_timestamp());

      std::cout << "found " << spans.size() << " spans." <<std::endl;

      // auto span_context = FromSpanContext(spans[0].span_context());

      // timepoint --> time_t
      // std::time_t timestamp_t = std::chrono::system_clock::to_time_t(timestamp);
      //
      // std::cout << "timestamp is " << std::ctime(&timestamp_t) << std::endl;

      // auto tracer = initTracer(component_name, reporter_id);
      //
      // for (auto iter = spans.begin(); iter != spans.end(); iter++) {
      //   std::string operation_name = iter->operation_name();
      //
      //   // quickly start and finish each span (I guess)
      //   auto span = tracer->StartSpan(operation_name);
      //   span->Finish();
      // }
      //
      // tracer->Flush();

      // // TODO: somehow operation_name is becoming confused with access_token
      // std::cout << "number of spans: " << report_request.spans().size() << std::endl;
      //
      // std::cout << reporter.reporter_id() << std::endl;
      // print_keyvaluetags(reporter.tags());
    }

    // to see if LightStep is working...
    // initGlobalTracer();

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
