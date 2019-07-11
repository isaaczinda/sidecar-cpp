# What is This?

This is a very simple sidecar which will listen for report_requests on port 1998 and forward them to a satellite running on localhost:8360.
Active work is happening on the boost_asio branch. This is a prototype, and so there is lots of unnecessary copying and the retry / error handling logic is very bad.

Compile with `bazel build //:boost_example`.

# Requirements
 - use bazel 0.27.0
 - compile on mac (or another POSIX thing)

# Upcoming Features (when the project gets approved)
 - split report_requests into smaller chunks so that we lose less if a satellite drops a report_request
 - try to get this to zero-copy
 - send 200 OK to tracer even if there are parse errors because we don't want the tracer's buffer filling up. Maybe send 200 OK before we even read the whole request?
 - have client retry connecting to different satellites if a POST to one satellite fails.
