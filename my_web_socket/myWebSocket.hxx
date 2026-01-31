#pragma once

#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <deque>
#include <fmt/color.h>

namespace my_web_socket
{

typedef boost::beast::websocket::stream<boost::asio::use_awaitable_t<>::as_default_on_t<boost::beast::tcp_stream> > WebSocket;
typedef boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream> > SSLWebSocket;
typedef boost::asio::use_awaitable_t<>::as_default_on_t<boost::asio::basic_waitable_timer<boost::asio::chrono::system_clock> > CoroTimer;

template <class T> class MyWebSocket : public std::enable_shared_from_this<MyWebSocket<T> >
{
public:
  explicit MyWebSocket (T &&webSocket_) : webSocket{ std::move (webSocket_) } {}
  MyWebSocket (T &&webSocket_, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_) : webSocket{ std::move (webSocket_) }, loggingName{ std::move (loggingName_) }, loggingTextStyleForName{ std::move (loggingTextStyleForName_) }, id{ std::move (id_) } {}

  void queueMessage (std::string message);
  boost::asio::awaitable<void> readLoop (std::function<void (std::string readResult)> onRead);
  boost::asio::awaitable<void> writeLoop ();
  boost::asio::awaitable<void> asyncWriteOneMessage (std::string message);
  boost::asio::awaitable<void> sendPingToEndpoint ();
  boost::asio::awaitable<void> asyncClose ();
  boost::asio::awaitable<std::string> asyncReadOneMessage ();

private:
  std::string rndNumberAsString ();

  T webSocket{};
  std::string loggingName{};
  fmt::text_style loggingTextStyleForName{};
  std::string id{ rndNumberAsString () };
  std::deque<std::string> msgQueue{};
  CoroTimer pingTimer{ webSocket.get_executor () };
  std::atomic_bool running{ true };
  boost::asio::experimental::channel<boost::asio::any_io_executor, void (boost::system::error_code)> writeSignal{ webSocket.get_executor (), 1 };
};

}
