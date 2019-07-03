#include "span_converter.h"

/*
google::protobuf::Timestamp ToTimestamp(
    const std::chrono::system_clock::time_point& t) {
  using namespace std::chrono;
  auto nanos = duration_cast<nanoseconds>(t.time_since_epoch()).count();
  google::protobuf::Timestamp ts;
  const uint64_t nanosPerSec = 1000000000;
  ts.set_seconds(nanos / nanosPerSec);
  ts.set_nanos(nanos % nanosPerSec);
  return ts;
}

timeval ToTimeval(std::chrono::microseconds microseconds) {
  timeval result;
  auto num_microseconds = microseconds.count();
  const size_t microseconds_in_second = 1000000;
  result.tv_sec =
      static_cast<time_t>(num_microseconds / microseconds_in_second);
  result.tv_usec =
      static_cast<suseconds_t>(num_microseconds % microseconds_in_second);
  return result;
}


std::time_t epoch_time = std::chrono::system_clock::to_time_t(p0);
std::cout << "epoch: " << std::ctime(&epoch_time);


time_point<[...], duration<long long, ratio<[...],
      1000000>>>
*/


class SpanContext : public opentracing::SpanContext {
 public:
  virtual bool sampled() const noexcept = 0;

  virtual uint64_t trace_id() const noexcept = 0;

  virtual uint64_t span_id() const noexcept = 0;

  virtual opentracing::expected<void> Inject(
      const PropagationOptions& propagation_options,
      std::ostream& writer) const = 0;

  virtual opentracing::expected<void> Inject(
      const PropagationOptions& propagation_options,
      const opentracing::TextMapWriter& writer) const = 0;

  virtual opentracing::expected<void> Inject(
      const PropagationOptions& propagation_options,
      const opentracing::HTTPHeadersWriter& writer) const = 0;
};

void PrintTimePoint(TimePointOpenTracing & tp) {
  long duration = tp.time_since_epoch().count();

  std::cout << "microseconds: " << duration << std::endl;
}


TimePointOpenTracing FromTimestamp(google::protobuf::Timestamp timestamp)
{
  // fetch the nanos and seconds fields from the protobuf object
  NanoDuration duration_nanos(timestamp.nanos());
  SecondDuration duration_seconds(timestamp.seconds());

  // createa a time_point object that starts at the beginning of the epoch
  // and add the seconds and nanos to it
  TimePointOpenTracing timepoint_micros;
  timepoint_micros += std::chrono::duration_cast < MicroDuration > (duration_seconds);
  timepoint_micros += std::chrono::duration_cast < MicroDuration > (duration_nanos);

  return timepoint_micros;
}

std::pair < opentracing::string_view, opentracing::Value > FromKeyValue (lightstep::collector::KeyValue keyvalue)
{
  opentracing::Value opentracing_value;

  switch (keyvalue.value_case()) {
    case lightstep::collector::KeyValue::kStringValue:
      opentracing_value = opentracing::Value (keyvalue.string_value());
      break;

    case lightstep::collector::KeyValue::kIntValue:
      opentracing_value = opentracing::Value (keyvalue.int_value());
      break;

    case lightstep::collector::KeyValue::kDoubleValue:
      opentracing_value = opentracing::Value (keyvalue.double_value());
      break;

    case lightstep::collector::KeyValue::kBoolValue:
      opentracing_value = opentracing::Value (keyvalue.bool_value());
      break;

    case lightstep::collector::KeyValue::kJsonValue:
      //TODO: lets put this in a dictionary eventually...
      opentracing_value = opentracing::Value (keyvalue.json_value());
      break;

    case lightstep::collector::KeyValue::VALUE_NOT_SET:
      opentracing_value = opentracing::Value();
      break;

    default:
      throw "not a valid case";
  }

  auto key = opentracing::string_view(keyvalue.key());

  // TODO: check pointer logic on this ?
  return std::pair < opentracing::string_view, opentracing::Value > (key, opentracing_value);
}


opentracing::LogRecord FromLog(lightstep::collector::Log log)
{
  std::vector < std::pair < std::string, opentracing::Value > > ot_fields;
  auto ot_timestamp = FromTimestamp(log.timestamp());

  auto fields = log.fields();

  for (auto iter = fields.begin(); iter != fields.end(); iter++) {
    auto pair = FromKeyValue(*iter);
    ot_fields.push_back(pair);
  }

  return opentracing::LogRecord{ot_timestamp, ot_fields};
}


std::pair < opentracing::SpanReferenceType, const opentracing::SpanContext* >
  FromReference(lightstep::collector::Reference reference)
{

}


opentracing::SpanReferenceType FromRelationship(lightstep::collector::Reference_Relationship relationship)
{
  if (relationship == lightstep::collector::Reference::CHILD_OF) { return opentracing::SpanReferenceType::ChildOfRef; }
  if (relationship == lightstep::collector::Reference::FOLLOWS_FROM) { return opentracing::SpanReferenceType::FollowsFromRef; }

  //TODO: make this a better exception
  throw "lightstep::collector::Relationship had an illegal value.";
}

opentracing::expected<std::unique_ptr<opentracing::SpanContext>> FromSpanContext(lightstep::collector::SpanContext span_context)
{
  std::unordered_map<std::string, std::string> ls_baggage;
  auto baggage = span_context.baggage();

  for (auto iter = baggage.begin(); iter != baggage.end(); iter++) {
    std::cout << "{" << iter->first << ", " << iter->second << "}, ";
    ls_baggage.insert(*iter); // copy the pair into the new map
  }
  std::cout << std::endl;

  auto ls_span_context = std::unique_ptr<opentracing::SpanContext>{
    new lightstep::LightStepImmutableSpanContext{ span_context.trace_id(), span_context.span_id(), true, ls_baggage }};

  return opentracing::expected<std::unique_ptr<opentracing::SpanContext>>(ls_span_context);

}
