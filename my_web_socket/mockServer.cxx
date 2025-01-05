#include "my_web_socket/mockServer.hxx"
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>
#include <openssl/ssl3.h>
namespace my_web_socket
{

boost::asio::awaitable<void>
tryUntilNoException (std::function<void ()> const &fun, std::chrono::seconds const &timeToWaitBeforeCallingFunctionAgain)
{
  for (;;) // try until no exception
    {
      try
        {
          fun ();
          break;
        }
      catch (std::exception &e)
        {
          std::cout << "exception : " << e.what () << std::endl;
        }
      std::cout << "trying again in: " << timeToWaitBeforeCallingFunctionAgain.count () << " seconds" << std::endl;
      auto timer = CoroTimer{ co_await boost::asio::this_coro::executor };
      timer.expires_after (timeToWaitBeforeCallingFunctionAgain);
      co_await timer.async_wait ();
    }
}

boost::beast::net::ssl::context
createSSLContext (SSLSuport const &sslSupport, boost::asio::ssl::context_base::method const &method)
{
  auto sslContext = boost::beast::net::ssl::context{ method };
  if (sslSupport.sslContextVerifyNone)
    {
      sslContext.set_verify_mode (boost::asio::ssl::context::verify_none);
    }
  else
    {
      sslContext.set_verify_mode (boost::asio::ssl::context::verify_peer);
    }
  sslContext.set_default_verify_paths ();
  sslContext.use_certificate_chain_file (sslSupport.pathToChainFile);
  sslContext.use_private_key_file (sslSupport.pathToPrivateFile, boost::asio::ssl::context::pem);
  sslContext.use_tmp_dh_file (sslSupport.pathToTmpDhFile);
  boost::certify::enable_native_https_server_verification (sslContext);
  sslContext.set_options (SSL_SESS_CACHE_OFF | SSL_OP_NO_TICKET); //  disable ssl cache. It has a bad support in boost asio/beast and I do not know if it helps in performance in our usecase
  return sslContext;
}

template <class T> MockServer<T>::MockServer (boost::asio::ip::tcp::endpoint endpoint, MockServerOption const &mockServerOption_, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_) : mockServerOption{ mockServerOption_ }
{
  if (std::same_as<T, SSLWebSocket>)
    {
      if (mockServerOption.sslSupport.has_value ())
        {
          sslContext = createSSLContext (mockServerOption.sslSupport.value (), boost::asio::ssl::context_base::method::tls_server);
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
          boost::asio::co_spawn (executor, webSocketItr->readLoop ([&_webSockets = webSockets, webSocketItr, &_mockServerOption = mockServerOption, &_ioContext = ioContext] (const std::string &msg) mutable {
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
