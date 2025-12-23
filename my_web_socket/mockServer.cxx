#include "my_web_socket/mockServer.hxx"
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
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
  co_spawn (ioContext, listener (endpoint, loggingName_, loggingTextStyleForName_, id_), printException);
  thread = std::thread{ [this] () { ioContext.run (); } };
  auto lk = std::unique_lock<std::mutex>{ waitForServerStarted };
  waitForServerStartedCond.wait (lk, [this] { return serverStarted; }); // checks if serverStarted is true and if not waits for waitForServerStartedCond notify and serverStarted == true
}
template <class T> MockServer<T>::~MockServer ()
{
  ioContext.stop ();
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
  co_await timer.async_wait ();
  for (auto &myWebSocket : webSockets)
    {
      myWebSocket.close ();
    }
  ioContext.stop ();
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
  tcp_acceptor acceptor (executor, endpoint);
  while (not ioContext.stopped ())
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
          auto socket = co_await acceptor.async_accept ();
          if constexpr (std::same_as<T, WebSocket>)
            {
              auto webSocket = T{ std::move (socket) };
              webSocket.set_option (websocket::stream_base::timeout::suggested (role_type::server));
              webSocket.set_option (websocket::stream_base::decorator ([] (websocket::response_type &res) { res.set (http::field::server, std::string (BOOST_BEAST_VERSION_STRING) + " webSocket-server-async"); }));
              co_await webSocket.async_accept ();
              webSockets.emplace_back (MyWebSocket<T>{ std::move (webSocket), loggingName_, loggingTextStyleForName_, id_ });
            }
          else if constexpr (std::same_as<T, SSLWebSocket>)
            {
              auto webSocket = T{ std::move (socket), *sslContext };
              webSocket.set_option (websocket::stream_base::timeout::suggested (role_type::server));
              webSocket.set_option (websocket::stream_base::decorator ([] (websocket::response_type &res) { res.set (http::field::server, std::string (BOOST_BEAST_VERSION_STRING) + " websocket-server-async"); }));
              co_await webSocket.next_layer ().async_handshake (ssl::stream_base::server, use_awaitable);
              co_await webSocket.async_accept (use_awaitable);
              webSockets.emplace_back (MyWebSocket<T>{ std::move (webSocket), loggingName_, loggingTextStyleForName_, id_ });
            }
          auto webSocketItr = std::prev (webSockets.end ());
          boost::asio::co_spawn (executor, webSocketItr->readLoop ([&_webSockets = webSockets, webSocketItr, &_mockServerOption = mockServerOption, &_ioContext = ioContext] (std::string msg) mutable {
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
                for (auto &webSocket_ : _webSockets)
                  {
                    webSocket_.close ();
                  }
                _ioContext.stop ();
              }
            else if (_mockServerOption.closeConnectionOnMessage && _mockServerOption.closeConnectionOnMessage.value () == msg)
              {
                webSocketItr->close ();
              }
            else if (_mockServerOption.requestResponse.count (msg))
              webSocketItr->queueMessage (_mockServerOption.requestResponse.at (msg));
            else if (not _mockServerOption.requestStartsWithResponse.empty ())
              {
                auto msgFound = false;
                for (auto const &[startsWith, response] : _mockServerOption.requestStartsWithResponse)
                  {
                    if (boost::starts_with (msg, startsWith))
                      {
                        msgFound = true;
                        webSocketItr->queueMessage (response);
                        break;
                      }
                  }
                if (not msgFound)
                  {
                    std::cout << "unhandled message: " << msg << std::endl;
                  }
              }
          }) && webSocketItr->writeLoop (),
                                 [&_webSockets = webSockets, webSocketItr] (auto eptr) {
                                   printException (eptr);
                                   _webSockets.erase (webSocketItr);
                                 });
          if (mockServerOption.mockServerRunTime)
            {
              co_spawn (ioContext, serverShutDownTime (), printException);
            }
        }
      catch (std::exception const &e)
        {
          std::cout << "MockServer::listener ()  Exception : " << e.what () << std::endl;
          throw e;
        }
    }
}
template <class T>
bool
MockServer<T>::isRunning ()
{
  return not ioContext.stopped ();
}

template class MockServer<WebSocket>;
template class MockServer<SSLWebSocket>;
}
