/* PROBLEMS:

 - right now, we are relying on a sketchy relative include path in our Makefile
   to find the lightstep proto headers.
 - create some data structure to manage the different tracers, because we don't
   want to have to create a new one every time. we also want some sort of
   garbage collector to clean up ones that haven't been used for too long
    * maybe LRU replacement ?
 - fix /usr/local/include/google/protobuf/parse_context.h warnings.

 - standard way of setting tracer baggage
    * how to change tracer `component_name`
    * might be a useful read: https://github.com/opentracing/specification/blob/master/semantic_conventions.md

 - how to convert span proto span object --> span and preserve the time it was
   originally recorded
*/

#include <evhttp.h>
#include <iostream>
#include "collector.pb.h" // TODO: WOW this is jank...
#include <sstream>
#include <iostream>

#include <opentracing/tracer.h>
#include <lightstep/tracer.h>
#include <map>
#include "span_converter.h"

#define SATELLITE_ADDRESS "localhost"
#define SATELLITE_PORT 8360

#define BINARY_CONTENT_TYPE "application/octet-stream"

// make the custom-defined protobuf key value dict a bit more paletable
typedef google::protobuf::RepeatedPtrField< lightstep::collector::KeyValue > KeyValueDict;
typedef std::shared_ptr < lightstep::LightStepTracer > LightStepTracerPtr;


// keep track of all the tracers we spin up !
std::map< int, std::shared_ptr< lightstep::LightStepTracer > > Tracers;


std::shared_ptr < lightstep::LightStepTracer > initTracer(std::string component_name, int tracer_id) {
  // check if we need to make a new tracer for this tracer_id
  auto found_tracer = Tracers.find(tracer_id);

  // if we were able to find this tracer_id in the nam
  if (found_tracer != Tracers.end()) {
    std::cout << "we found tracer " << tracer_id << " in the map." << std::endl;
    return found_tracer->second;
  } else
  {
    std::cout << "we are creating new tracer with id " << tracer_id <<  std::endl;
  }

  lightstep::LightStepTracerOptions options;

  options.component_name = component_name;
  options.access_token = "default token";

  // use the streaming tracer settings
  options.collector_plaintext = true;
  options.satellite_endpoints = {{SATELLITE_ADDRESS, SATELLITE_PORT}};
  options.use_stream_recorder = true;
  options.verbose = true;

  auto tracer = lightstep::MakeLightStepTracer(std::move(options));

  Tracers.insert(std::pair< int, std::shared_ptr< lightstep::LightStepTracer > >(tracer_id, tracer));
  return tracer;
}

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

      auto timestamp = FromTimestamp(spans[0].start_timestamp());

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
