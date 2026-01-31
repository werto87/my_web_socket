#include "my_web_socket/mockServer.hxx"
#include "mockServer.hxx"
#include "my_web_socket/coSpawnTraced.hxx"
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <syncstream>

namespace my_web_socket
{

template <class T> MockServer<T>::MockServer (boost::asio::ip::tcp::endpoint endpoint, MockServerOption const &mockServerOption_, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_) : mockServerOption{ mockServerOption_ }
{
  if (std::same_as<T, SSLWebSocket>)
    {
      if (mockServerOption.createSSLContext)
        {
          sslContext = mockServerOption.createSSLContext ();
        }
      else
        {
          throw std::logic_error{ "if you want to use SSLWebsocket you have to set mock server option ssl support" };
        }
    }
  coSpawnTraced (ioContext, listener (endpoint, loggingName_, loggingTextStyleForName_, id_), "MockServer listener");
  thread = std::thread{ [this] () { ioContext.run (); } };
  auto lk = std::unique_lock<std::mutex>{ waitForServerStarted };
  waitForServerStartedCond.wait (lk, [this] { return serverStarted; }); // checks if serverStarted is true and if not waits for waitForServerStartedCond notify and serverStarted == true
}
template <class T> MockServer<T>::~MockServer ()
{
  thread.join ();
  for (auto &onDestruct : mockServerOption.callAtTheEndOFDestruct)
    {
      if (onDestruct) onDestruct ();
    }
}
template <class T>
boost::asio::awaitable<void>
MockServer<T>::serverShutDownTime ()
{
  auto timer = CoroTimer{ co_await boost::asio::this_coro::executor };
  timer.expires_after (mockServerOption.mockServerRunTime.value ());
  try
    {
      co_await timer.async_wait ();
      co_await asyncShutDown ();
    }
  catch (boost::system::system_error &e)
    {
      if (boost::asio::error::misc_errors::eof == e.code ())
        {
          // swallow eof
        }
      else if (boost::asio::error::operation_aborted == e.code ())
        {
          co_return;
        }
      else
        {
          throw;
        }
    }
}
template <class T>
boost::asio::awaitable<void>
MockServer<T>::listener (boost::asio::ip::tcp::endpoint endpoint, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_)
{
  using namespace boost::beast;
  using namespace boost::asio;
  using boost::asio::ip::tcp;
  using tcp_acceptor = use_awaitable_t<>::as_default_on_t<tcp::acceptor>;
  auto executor = co_await this_coro::executor;
  acceptor = std::make_unique<tcp_acceptor> (executor, endpoint);
  while (running.load (std::memory_order_acquire))
    {
      try
        {
          using namespace boost::asio::experimental::awaitable_operators;
          if (not serverStarted)
            {
              {
                std::lock_guard<std::mutex> lk{ waitForServerStarted };
                serverStarted = true; // Set 'serverStarted' to true before notifying.
              }
              waitForServerStartedCond.notify_all ();
            }
          auto socket = co_await acceptor->async_accept ();
          if constexpr (std::same_as<T, WebSocket>)
            {
              auto webSocket = T{ std::move (socket) };
              webSocket.set_option (websocket::stream_base::timeout::suggested (role_type::server));
              webSocket.set_option (websocket::stream_base::decorator ([] (websocket::response_type &res) { res.set (http::field::server, std::string (BOOST_BEAST_VERSION_STRING) + " webSocket-server-async"); }));
              co_await webSocket.async_accept ();
              webSockets.emplace_back (std::make_shared<MyWebSocket<WebSocket> > (std::move (webSocket), loggingName_, loggingTextStyleForName_, id_));
            }
          else if constexpr (std::same_as<T, SSLWebSocket>)
            {
              auto webSocket = T{ std::move (socket), *sslContext };
              webSocket.set_option (websocket::stream_base::timeout::suggested (role_type::server));
              webSocket.set_option (websocket::stream_base::decorator ([] (websocket::response_type &res) { res.set (http::field::server, std::string (BOOST_BEAST_VERSION_STRING) + " websocket-server-async"); }));
              co_await webSocket.next_layer ().async_handshake (ssl::stream_base::server, use_awaitable);
              co_await webSocket.async_accept (use_awaitable);
              webSockets.emplace_back (std::make_shared<MyWebSocket<SSLWebSocket> > (std::move (webSocket), loggingName_, loggingTextStyleForName_, id_));
            }
          auto webSocketItr = std::prev (webSockets.end ());
          coSpawnTraced (executor, (*webSocketItr)->readLoop ([this, &_webSockets = webSockets, webSocketItr, &_mockServerOption = mockServerOption, &_ioContext = ioContext] (std::string msg) mutable {
            for (auto const &[startsWith, callback] : _mockServerOption.callOnMessageStartsWith)
              {
                if (boost::starts_with (msg, startsWith))
                  {
                    callback ();
                    break;
                  }
              }
            if (_mockServerOption.shutDownServerOnMessage && _mockServerOption.shutDownServerOnMessage.value () == msg)
              {
                coSpawnTraced (ioContext, asyncShutDown (), "MockServer shutDownServerOnMessage asyncShutDown");
              }
            else if (_mockServerOption.closeConnectionOnMessage && _mockServerOption.closeConnectionOnMessage.value () == msg)
              {
                coSpawnTraced (ioContext, (*webSocketItr)->asyncClose (), "MockServer closeConnectionOnMessage asyncClose");
              }
            else if (_mockServerOption.requestResponse.count (msg))
              (*webSocketItr)->queueMessage (_mockServerOption.requestResponse.at (msg));
            else if (not _mockServerOption.requestStartsWithResponse.empty ())
              {
                auto msgFound = false;
                for (auto const &[startsWith, response] : _mockServerOption.requestStartsWithResponse)
                  {
                    if (boost::starts_with (msg, startsWith))
                      {
                        msgFound = true;
                        (*webSocketItr)->queueMessage (response);
                        break;
                      }
                  }
                if (not msgFound)
                  {
                    std::osyncstream (std::cout) << "unhandled message: " << msg << std::endl;
                  }
              }
          }) && (*webSocketItr)->writeLoop (),
                         "MockServer read and write", [&_webSockets = webSockets, webSocketItr] (auto eptr) { _webSockets.erase (webSocketItr); });
          if (mockServerOption.mockServerRunTime)
            {
              coSpawnTraced (ioContext, serverShutDownTime (), "serverShutDownTime");
            }
        }
      catch (std::exception const &e)
        {
          throw e;
        }
    }
}
template <class T>
boost::asio::awaitable<void>
MockServer<T>::asyncShutDown ()
{
  if (not running.load (std::memory_order_acquire)) co_return;
  running.store (false, std::memory_order_release);
  boost::system::error_code ec;
  acceptor->cancel (ec);
  acceptor->close (ec);
  for (auto &webSocket : webSockets)
    {
      co_await webSocket->asyncClose ();
    }
}
template <class T>
bool
MockServer<T>::isRunning ()
{
  return running.load (std::memory_order_acquire);
}

template <class T>
void
MockServer<T>::shutDownUsingMockServerIoContext ()
{
  coSpawnTraced (ioContext, asyncShutDown (), "MockServer shutDownUsingMockServerIoContext asyncShutDown");
}

template class MockServer<WebSocket>;
template class MockServer<SSLWebSocket>;
}
