#include "my_web_socket/myWebSocket.hxx"
#include "myWebSocket.hxx"
#include "my_web_socket/coSpawnPrintException.hxx"
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
MyWebSocket<T>::async_read_one_message ()
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
          auto oneMsg = co_await async_read_one_message ();
          onRead (std::move (oneMsg));
        }
    }
  catch (...)
    {
      if (webSocket) webSocket.reset ();
      if (msgQueueTimer) msgQueueTimer->cancel ();
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
MyWebSocket<T>::async_write_one_message (std::string message)
{
#ifdef MY_WEB_SOCKET_LOG_WRITE
  printTagWithPadding (loggingName + (loggingName.empty () ? "" : " ") + id, loggingTextStyleForName, 30);
  fmt::print ("[w] {} \n", message);
  std::fflush (stdout);
#endif
  co_await webSocket->async_write (boost::asio::buffer (std::move (message)), boost::asio::use_awaitable);
}
template <class T>
inline boost::asio::awaitable<void>
MyWebSocket<T>::writeLoop ()
{
  auto connection = std::weak_ptr<T>{ webSocket };
  try
    {
      while (not connection.expired ())
        {
          msgQueueTimer = std::make_shared<CoroTimer> (CoroTimer{ co_await boost::asio::this_coro::executor });
          msgQueueTimer->expires_after (std::chrono::system_clock::time_point::max () - std::chrono::system_clock::now ());
          try
            {
              co_await msgQueueTimer->async_wait ();
            }
          catch (boost::system::system_error &e)
            {
              if (boost::system::errc::operation_canceled == e.code ())
                {
                  //  swallow cancel
                }
              else
                {
                  std::cout << "error in msgQueueTimer boost::system::errc: " << e.code () << std::endl;
                  abort ();
                }
            }
          while (not connection.expired () && not msgQueue.empty ())
            {
              auto tmpMsg = std::move (msgQueue.front ());
              msgQueue.pop_front ();
              co_await async_write_one_message (std::move (tmpMsg));
            }
        }
    }
  catch (std::exception const &e)
    {
      webSocket.reset ();
      if (msgQueueTimer) msgQueueTimer->cancel ();
      throw;
    }
}
template <class T>
inline void
MyWebSocket<T>::queueMessage (std::string message)
{
  msgQueue.push_back (std::move (message));
  if (msgQueueTimer) msgQueueTimer->cancel ();
}
template <class T>
inline void
MyWebSocket<T>::close ()
{
  try
    {
      if (webSocket)
        {
          boost::beast::get_lowest_layer (*webSocket).cancel ();
          webSocket->close ("close connection");
        }
    }
  catch (boost::system::system_error &e)
    {
      if (boost::asio::error::misc_errors::eof == e.code ())
        {
          // swallow eof
        }
      else if (boost::asio::error::operation_aborted == e.code ())
        {
          // swallow operation_aborted
        }
      else
        {
          throw;
        }
    }
}

template <class T>
boost::asio::awaitable<void>
MyWebSocket<T>::asyncClose ()
{
  if (!webSocket) co_return;
  auto ws = webSocket;
  try
    {
      if (ws)
        {
          co_await ws->async_close (boost::beast::websocket::close_code::normal);
        }
    }
  catch (boost::system::system_error &e)
    {
      if (boost::asio::error::misc_errors::eof == e.code ())
        {
          // swallow eof
        }
      else if (boost::asio::error::operation_aborted == e.code ())
        {
          // swallow operation_aborted
        }
      else
        {
          throw;
        }
    }
}

template <class T>
boost::asio::awaitable<void>
MyWebSocket<T>::sendPingToEndpoint ()
{
  auto weak = std::weak_ptr<T>{ webSocket };
  auto pingTimer = CoroTimer{ co_await boost::asio::this_coro::executor };
  try
    {
      while (true)
        {
          pingTimer.expires_after (std::chrono::seconds{ 10 });
          co_await pingTimer.async_wait ();
          auto conn = weak.lock ();
          if (!conn) co_return;
          co_await conn->async_ping ({}, boost::asio::use_awaitable);
        }
    }
  catch (boost::system::system_error &e)
    {
      using namespace boost::system::errc;
      std::cout << "error in msgQueueTimer boost::system::errc: " << e.code () << std::endl;
    }
  co_return;
}

template class MyWebSocket<WebSocket>;
template class MyWebSocket<SSLWebSocket>;
}