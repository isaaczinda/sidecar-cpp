/*
taken from: http://think-async.com/Asio/boost_asio_1_13_0/doc/html/boost_asio/tutorial/tutdaytime3/src.html

todo: match both \r\n\r\n\ and \n\n
*/

// #include <collector.pb.h>

#include <ctime>
#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>


#define PORT 1998
#define READ_BUFFER_LENGTH 128

using boost::asio::ip::tcp;

/*
we inherit from boost::enable_shared_from_this and support a shared_ptr
so that we can continue reusing a connection so long as
*/
class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection>
{
private:
  std::string message_;
  tcp::socket socket_;
  boost::asio::streambuf request_data_;

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

    message_ =
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 12\r\n"
      "Content-Type: text/plain\r\n\r\n"
      "Hello world.";

    auto write_handler = boost::bind(
      &tcp_connection::handle_write,
      // passes 'this' in a way that the shared pointer class notices
      shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred);

    // setup the read buffer, underlying memory allocated on the heap
    // std::shared_ptr<std::vector<char>> char_buf = new std::vector<char>(READ_BUFFER_LENGTH + 1); // TODO: remove 1
    // std::shared_ptr<char> char_buf(new char[READ_BUFFER_LENGTH + 1], std::default_delete<char[]>());
    // auto read_buffer = boost::asio::buffer(char_buf.get(), READ_BUFFER_LENGTH + 1);

    // read handler will write data into the read buffer
    auto read_handler = boost::bind(
      &tcp_connection::handle_read_finish,
      shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred);

    boost::asio::async_write(socket_, boost::asio::buffer(message_), write_handler);
    boost::asio::async_read_until(socket_, request_data_, "\r\n\r\n", read_handler);
  }

private:
  tcp_connection(boost::asio::io_context &io_context)
    : socket_(io_context)
  {
  }

  void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/)
  {
    std::cout << "tcp_connection::handle_write()" << std::endl;
  }

  void handle_read_finish(const boost::system::error_code& error, size_t bytes_transferred)
  {
    if (error != boost::asio::error::misc_errors::eof) {
      std::cerr << "read finished with error code: " << error << std::endl;
    }



    std::cout << "tcp_connection::handle_read_finish() transferred "
      << bytes_transferred << " bytes" << std::endl;

    std::cout << "size: " << request_data_.size() << std::endl;
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

/*
io cocntext:  boost-:asio::io_context io_context;
io object:    boost::asio::ip::tcp::socket socket(io_context);

errors:
 - boost::system::error_code is passed from io context to io object
   - may be tested as bool (false means no error)
 - void your_completion_handler(const boost::system::error_code& ec);


io_context::run() -- starts the loop that checks OS queues for finished work


*/
