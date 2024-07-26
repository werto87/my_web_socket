#include "my_web_socket/myWebSocket.hxx"
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
namespace my_web_socket
{

#ifdef MY_WEB_SOCKET_LOG_CO_SPAWN_PRINT_EXCEPTIONS
void
printExceptionHelper (std::exception_ptr eptr)
{
  try
    {
      if (eptr)
        {
          std::rethrow_exception (eptr);
        }
    }
  catch (std::exception const &e)
    {
      std::cout << "co_spawn exception: '" << e.what () << "'" << std::endl;
    }
}
#else
void
printExceptionHelper (std::exception_ptr)
{
}
#endif

void
printTagWithPadding (std::string const &tag, fmt::text_style const &style, size_t maxLength)
{
  if (maxLength < 3) throw std::logic_error{ "maxLength should be min 3" };
  if (tag.length () > maxLength)
    {
      fmt::print (style, fmt::runtime ("[{:<" + std::to_string (maxLength) + "}]"), std::string{ tag.begin (), tag.begin () + boost::numeric_cast<int> (maxLength) - 3 } + "...");
    }
  else
    {
      fmt::print (style, "[{}]{}", tag, std::string (maxLength - tag.size (), '-'));
    }
}

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
  fmt::print ("[r] {}", msg);
  std::cout << std::endl;
#endif
  co_return msg;
}

template <class T>
inline boost::asio::awaitable<void>
MyWebSocket<T>::readLoop (std::function<void (std::string const &readResult)> onRead)
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
      webSocket.reset ();
      if (timer) timer->cancel ();
#ifdef MY_WEB_SOCKET_LOG_READ_END
      printTagWithPadding (loggingName + (loggingName.empty () ? "" : " ") + id, loggingTextStyleForName, 30);
      fmt::print ("[c]");
      std::cout << std::endl;
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
  fmt::print ("[w] {}", message);
  std::cout << std::endl;
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
          timer = std::make_shared<CoroTimer> (CoroTimer{ co_await boost::asio::this_coro::executor });
          timer->expires_after (std::chrono::system_clock::time_point::max () - std::chrono::system_clock::now ());
          try
            {
              co_await timer->async_wait ();
            }
          catch (boost::system::system_error &e)
            {
              if (boost::system::errc::operation_canceled == e.code ())
                {
                  //  swallow cancel
                }
              else
                {
                  std::cout << "error in timer boost::system::errc: " << e.code () << std::endl;
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
      if (timer) timer->cancel ();
      throw;
    }
}
template <class T>
inline void
MyWebSocket<T>::sendMessage (std::string message)
{
  msgQueue.push_back (std::move (message));
  if (timer) timer->cancel ();
}
template <class T>
inline void
MyWebSocket<T>::close ()
{
  try
    {
      if (webSocket) webSocket->close ("close connection");
    }
  catch (boost::system::system_error &e)
    {
      if (boost::asio::error::misc_errors::eof == e.code ())
        {
          // swallow eof
        }
      else
        {
          std::cout << "MyWebSocket::close () Exception : " << e.what () << std::endl;
          abort ();
        }
    }
}

template <class T>
boost::asio::awaitable<void>
MyWebSocket<T>::sendPingToEndpoint ()
{
  auto connection = std::weak_ptr<T>{ webSocket };
  auto pingTimer = CoroTimer{ co_await boost::asio::this_coro::executor };
  try
    {
      while (not connection.expired ())
        {
          pingTimer.expires_after (std::chrono::seconds{ 10 });
          co_await pingTimer.async_wait ();
          if (not connection.expired ())
            {
              co_await webSocket->async_ping ({}, boost::asio::use_awaitable);
            }
        }
    }
  catch (boost::system::system_error &e)
    {
      using namespace boost::system::errc;
      std::cout << "error in timer boost::system::errc: " << e.code () << std::endl;
    }
  co_return;
}
typedef boost::beast::websocket::stream<boost::asio::use_awaitable_t<>::as_default_on_t<boost::beast::tcp_stream> > Websocket;
template class MyWebSocket<Websocket>;
typedef boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream> > SSLWebsocket;
template class MyWebSocket<SSLWebsocket>;
}