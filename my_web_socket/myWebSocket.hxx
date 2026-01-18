#pragma once

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

template <class T> class MyWebSocket
{
public:
  MyWebSocket () = default;
  explicit MyWebSocket (T &&webSocket_) : webSocket{ std::make_shared<T> (std::move (webSocket_)) } {}
  MyWebSocket (T &&webSocket_, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_) : webSocket{ std::make_shared<T> (std::move (webSocket_)) }, loggingName{ std::move (loggingName_) }, loggingTextStyleForName{ std::move (loggingTextStyleForName_) }, id{ std::move (id_) } {}

  boost::asio::awaitable<void> readLoop (std::function<void (std::string readResult)> onRead);


  /**
   * @brief call this to send one message. If you do not have a read loop running and you call this function consider to call asyncReadOneMessage or asyncClose for correct shutdown before my_web_socket gets destroyed
   * 
   * @param message 
   * @return boost::asio::awaitable<void> 
   */
  boost::asio::awaitable<void> asyncWriteOneMessage (std::string message);

  boost::asio::awaitable<void> writeLoop ();

  void queueMessage (std::string message);

  boost::asio::awaitable<void> sendPingToEndpoint ();

  boost::asio::awaitable<void> asyncClose ();

  boost::asio::awaitable<std::string> asyncReadOneMessage ();

  std::shared_ptr<T> webSocket{};

private:
  std::string rndNumberAsString ();

  std::string loggingName{};
  fmt::text_style loggingTextStyleForName{};
  std::string id{ rndNumberAsString () };
  std::deque<std::string> msgQueue{};
  std::shared_ptr<CoroTimer> msgQueueTimer{};
  std::shared_ptr<CoroTimer> pingTimer{};
  std::atomic_bool running{ true };
};

}
