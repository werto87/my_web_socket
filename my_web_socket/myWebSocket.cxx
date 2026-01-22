#include "my_web_socket/myWebSocket.hxx"
#include "myWebSocket.hxx"
#include "my_web_socket/coSpawnPrintException.hxx"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>

namespace my_web_socket
{

template <class T>
std::string
MyWebSocket<T>::rndNumberAsString ()
{
  static std::random_device rd;       // Get a random seed from the OS entropy device, or whatever
  static std::mt19937_64 eng (rd ()); // Use the 64-bit Mersenne Twister 19937 generator
  std::uniform_int_distribution<uint64_t> distr{};
  return std::to_string (distr (eng));
}

template <class T>
boost::asio::awaitable<std::string>
MyWebSocket<T>::asyncReadOneMessage ()
{
  boost::beast::flat_buffer buffer;
  co_await webSocket->async_read (buffer, boost::asio::use_awaitable);
  auto msg = boost::beast::buffers_to_string (buffer.data ());
#ifdef MY_WEB_SOCKET_LOG_WRITE
  printTagWithPadding (loggingName + (loggingName.empty () ? "" : " ") + id, loggingTextStyleForName, 30);
  fmt::print ("[r] {} \n", msg);
  std::fflush (stdout);
#endif
  co_return msg;
}

template <class T>
inline boost::asio::awaitable<void>
MyWebSocket<T>::readLoop (std::function<void (std::string readResult)> onRead)
{
  try
    {
      for (;;)
        {
          auto oneMsg = co_await asyncReadOneMessage ();
          onRead (std::move (oneMsg));
        }
    }
  catch (...)
    {
#ifdef MY_WEB_SOCKET_LOG_READ
      printTagWithPadding (loggingName + (loggingName.empty () ? "" : " ") + id, loggingTextStyleForName, 30);
      fmt::print ("[c] \n");
      std::fflush (stdout);
#endif
      throw;
    }
}
template <class T>
inline boost::asio::awaitable<void>
MyWebSocket<T>::asyncWriteOneMessage (std::string message)
{
#ifdef MY_WEB_SOCKET_LOG_WRITE
  printTagWithPadding (loggingName + (loggingName.empty () ? "" : " ") + id, loggingTextStyleForName, 30);
  fmt::print ("[w] {} \n", message);
  std::fflush (stdout);
#endif
  co_await webSocket->async_write (boost::asio::buffer (std::move (message)), boost::asio::use_awaitable);
}

template <class T>
boost::asio::awaitable<void>
MyWebSocket<T>::writeLoop ()
{
  if (not writeSignal) writeSignal = std::make_unique<boost::asio::experimental::channel<boost::asio::any_io_executor, void (boost::system::error_code)> > (webSocket->get_executor (), 1);
  while (running.load (std::memory_order_acquire))
    {
      co_await writeSignal->async_receive (boost::asio::use_awaitable);
      while (running && !msgQueue.empty ())
        {
          auto msg = std::move (msgQueue.front ());
          msgQueue.pop_front ();
          co_await asyncWriteOneMessage (std::move (msg));
        }
    }
}

template <class T>
inline void
MyWebSocket<T>::queueMessage (std::string message)
{
  if (not writeSignal) writeSignal = std::make_unique<boost::asio::experimental::channel<boost::asio::any_io_executor, void (boost::system::error_code)> > (webSocket->get_executor (), 1);
  msgQueue.push_back (std::move (message));
  writeSignal->try_send (boost::system::error_code{});
}

template <class T>
boost::asio::awaitable<void>
MyWebSocket<T>::asyncClose ()
{
  if (not running.load (std::memory_order_acquire)) co_return;
  running.store (false, std::memory_order_release);
  webSocket->set_option (boost::beast::websocket::stream_base::timeout{ .handshake_timeout = std::chrono::milliseconds{ 1 } }); // do not wait longer than 1 millisecond for handshake close
  auto ec = boost::system::error_code{};
  co_await webSocket->async_close (boost::beast::websocket::close_code::normal, boost::asio::redirect_error (boost::asio::use_awaitable, ec));
  if (pingTimer) pingTimer->cancel ();
  if (writeSignal) writeSignal->close ();
}

template <class T>
boost::asio::awaitable<void>
MyWebSocket<T>::sendPingToEndpoint ()
{
  pingTimer = std::make_shared<CoroTimer> (CoroTimer{ co_await boost::asio::this_coro::executor });
  while (running.load (std::memory_order_acquire))
    {
      pingTimer->expires_after (std::chrono::seconds{ 10 });
      co_await pingTimer->async_wait ();
      co_await webSocket->async_ping ({}, boost::asio::use_awaitable);
    }
  co_return;
}

template class MyWebSocket<WebSocket>;
template class MyWebSocket<SSLWebSocket>;
}