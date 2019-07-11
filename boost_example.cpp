/*
taken from: http://think-async.com/Asio/boost_asio_1_13_0/doc/html/boost_asio/tutorial/tutdaytime3/src.html
*/

#include <collector.pb.h>

#include <ctime>
#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/beast.hpp>

#include <sstream>

#define PORT 1998

using boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

/*
we inherit from boost::enable_shared_from_this and support a shared_ptr
so that we can continue reusing a connection so long as
*/
class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection>
{
private:
  tcp::socket socket_;
  boost::asio::streambuf request_data_;
  std::string message_;

  beast::flat_buffer overflow_buf_; // resizable buffer of chars
  http::request<http::string_body> request_;

public:
  // make a nice name for a pointer to this class
  typedef boost::shared_ptr<tcp_connection> pointer;

  // create tcp_connection with this static factory-style method instead of a
  // constructor
  static pointer create(boost::asio::io_context& io_context)
  {
    return pointer(new tcp_connection(io_context));
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    std::cout << "tcp_connection::start()" << std::endl;

    // read handler will write data into the read buffer
    auto read_handler = boost::bind(
      &tcp_connection::handle_read,
      shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred);


    request_ = {}; // clear the request before writing to it
    http::async_read(socket_, overflow_buf_, request_, read_handler);
  }

private:
  // private constructor because we are using enable_shared_from_this
  tcp_connection(boost::asio::io_context &io_context)
    : socket_(io_context)
  {
  }

  void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/)
  {
  }

  void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
  {
    if(error == http::error::end_of_stream) {
      std::cout << "end of stream." << std::endl;
      return;
    }

    if(error) {
      std::cerr << "error reading : " << error << std::endl;
      return;
    }

    if (request_.base().method_string() != "POST") {
      std::cerr << "sidecar only accepts POST requests from tracers, not '"
        << request_.base().method_string() << "' requests." << std::endl;
      return;
    }

    // convert the request body from string --> stringstream
    std::stringstream body_stream(request_.body());

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    lightstep::collector::ReportRequest report_request;

    if (!report_request.ParseFromIstream(&body_stream)) {
      std::cerr << "there was an error parsing the report request" << std::endl;
      return;
    }

    std::cout << "received spans: " << report_request.spans().size() << std::endl;

    write_okay_response();
  }

  void write_okay_response()
  {
    message_ = "HTTP/1.1 200 OK\r\n\r\n";

    auto write_handler = boost::bind(
      &tcp_connection::handle_write,
      // passes 'this' in a way that the shared pointer class notices
      shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred);

    boost::asio::async_write(socket_, boost::asio::buffer(message_), write_handler);
  }
};


class tcp_server
{
private:
  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;

public:
  tcp_server(boost::asio::io_context& io_context)
    : io_context_(io_context),
      // connect on every adddress on this machine
      acceptor_(io_context, tcp::endpoint(tcp::v4(), PORT))
  {
    std::cout << "tcp_server::tcp_server()" << std::endl;

    start_accept();
  }

private:
  void start_accept()
  {
    std::cout << "tcp_server::start_accept()" << std::endl;

    // create a new socket that we can connect on
    tcp_connection::pointer new_connection =  tcp_connection::create(io_context_);

    // bind an accept handler to the new socket
    acceptor_.async_accept(
      new_connection->socket(),
      boost::bind(&tcp_server::handle_accept, this, new_connection, boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error)
  {
    std::cout << "tcp_server::handle_accept()" << std::endl;

    if (!error) {
      new_connection->start();
    }

    start_accept();
  }
};

int main() {
  try
  {
    boost::asio::io_context io_context;
    tcp_server server(io_context);
    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
