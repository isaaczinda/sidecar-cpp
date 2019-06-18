#include "collector.pb.h"
#include <opentracing/tracer.h>
#include <lightstep/tracer.h>
#include <chrono>
#include <ratio>

#define NANOS_PER_SECOND 1000000000
#define MICROS_PER_SECOND 1000000

typedef std::chrono::duration<long long, std::ratio < 1, MICROS_PER_SECOND > > MicroDuration;
typedef std::chrono::duration < long long, std::ratio < 1, NANOS_PER_SECOND > > NanoDuration;
typedef std::chrono::duration < long long > SecondDuration;

typedef std::chrono::time_point < std::chrono::system_clock, MicroDuration > TimePointOpenTracing;

// convert from protobuf timestamp to system timestamp
TimePointOpenTracing FromTimestamp(google::protobuf::Timestamp timestamp);

std::pair < opentracing::string_view, opentracing::Value > FromKeyValue(lightstep::collector::KeyValue keyvalue);

opentracing::LogRecord FromLog(lightstep::collector::Log log);

std::pair < opentracing::SpanReferenceType, const opentracing::SpanContext* >
  FromReference(lightstep::collector::Reference reference);

opentracing::SpanReferenceType FromRelationship(lightstep::collector::Reference_Relationship relationship);

opentracing::expected<std::unique_ptr<opentracing::SpanContext>>
  FromSpanContext(lightstep::collector::SpanContext spancontext);

/*

collector::KeyValue ToKeyValue(opentracing::string_view key,
                               const opentracing::Value& value) {
  collector::KeyValue key_value;
  key_value.set_key(key);
  ValueVisitor value_visitor{key_value, value};
  apply_visitor(value_visitor, value);
  return key_value;
}

void span_from_proto(LightStepTracerPtr tracer, lightstep::collector::Span span)
{
  span

  opentracing::StartSpanOptions start_options{};

  tracer->
}

using SystemClock = std::chrono::system_clock;
using SteadyClock = std::chrono::steady_clock;
using SystemTime = SystemClock::time_point;
using SteadyTime = SteadyClock::time_point;

*/
