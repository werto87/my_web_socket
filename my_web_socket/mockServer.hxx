#pragma once

#include "my_web_socket/myWebSocket.hxx"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <thread>
#include <variant>

namespace my_web_socket
{
struct MockServerOption
{
  std::optional<std::string> shutDownServerOnMessage{};
  std::optional<std::string> closeConnectionOnMessage{};
  std::map<std::string, std::string> requestResponse{};
  std::map<std::string, std::string> requestStartsWithResponse{};
  std::map<std::string, std::function<void ()> > callOnMessageStartsWith{};
};

struct MockServer
{
  MockServer (boost::asio::ip::tcp::endpoint endpoint, MockServerOption const &mockserverOption_, std::string loggingName_ = {}, fmt::text_style loggingTextStyleForName_ = {}, std::string id_ = {}) : mockserverOption{ mockserverOption_ }
  {
    co_spawn (ioContext, listener (endpoint, loggingName_, loggingTextStyleForName_, id_), printException);
    thread = std::thread{ [this] () { ioContext.run (); } };
    std::unique_lock<std::mutex> lk{ waitForServerStarted };
    // block main thread until server is started
    // blocking code based on https://stackoverflow.com/questions/43675995/is-my-wait-notify-mechanism-using-stdmutex-correct
    while (!serverStarted)
      { // Wait inside loop to handle spurious wakeups etc.

        waitForServerStartedCond.wait (lk);
      }
  }

  ~MockServer ()
  {
    ioContext.stop ();
    thread.join ();
  }

  boost::asio::awaitable<void>
  listener (boost::asio::ip::tcp::endpoint endpoint, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_)
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
                std::lock_guard<std::mutex> lk{ waitForServerStarted };
                serverStarted = true; // Set 'serverStarted' to true before notifying.
                waitForServerStarted.unlock ();
                waitForServerStartedCond.notify_all ();
              }
            auto socket = co_await (acceptor.async_accept ());
            auto webSocket = WebSocket{ std::move (socket) };
            webSocket.set_option (websocket::stream_base::timeout::suggested (role_type::server));
            webSocket.set_option (websocket::stream_base::decorator ([] (websocket::response_type &res) { res.set (http::field::server, std::string (BOOST_BEAST_VERSION_STRING) + " webSocket-server-async"); }));
            co_await webSocket.async_accept ();
            webSockets.emplace_back (MyWebSocket<WebSocket>{ std::move (webSocket), loggingName_, loggingTextStyleForName_, id_ });
            std::list<MyWebSocket<WebSocket> >::iterator webSocketItr = std::prev (webSockets.end ());
            boost::asio::co_spawn (executor, webSocketItr->readLoop ([&_webSockets = webSockets, webSocketItr, &_mockserverOption = mockserverOption, &_ioContext = ioContext] (const std::string &msg) mutable {
              for (auto const &[startsWith, callback] : _mockserverOption.callOnMessageStartsWith)
                {
                  if (boost::starts_with (msg, startsWith))
                    {
                      callback ();
                      break;
                    }
                }
              if (_mockserverOption.shutDownServerOnMessage && _mockserverOption.shutDownServerOnMessage.value () == msg)
                {
                  for (auto &webSocket : _webSockets)
                    {
                      webSocket.close ();
                    }
                  _ioContext.stop ();
                }
              else if (_mockserverOption.closeConnectionOnMessage && _mockserverOption.closeConnectionOnMessage.value () == msg)
                {
                  webSocketItr->close ();
                }
              else if (_mockserverOption.requestResponse.count (msg))
                webSocketItr->sendMessage (_mockserverOption.requestResponse.at (msg));
              else
                {
                  auto msgFound = false;
                  for (auto const &[startsWith, response] : _mockserverOption.requestStartsWithResponse)
                    {
                      if (boost::starts_with (msg, startsWith))
                        {
                          msgFound = true;
                          webSocketItr->sendMessage (response);
                          break;
                        }
                    }
                  if (not msgFound)
                    {
                      std::cout << "unhandled message: " << msg << std::endl;
                    }
                }
            }) && webSocketItr->writeLoop (),
                                   [&_websockets = webSockets, webSocketItr] (auto eptr) {
                                     printException (eptr);
                                     _websockets.erase (webSocketItr);
                                   });
          }
        catch (std::exception const &e)
          {
            std::cout << "MockServer::listener ()  Exception : " << e.what () << std::endl;
            throw e;
          }
      }
  }
  MockServerOption mockserverOption{};
  bool shouldRun = true;
  boost::asio::io_context ioContext;
  std::thread thread{};
  std::list<MyWebSocket<WebSocket> > webSockets{ {} };
  std::mutex waitForServerStarted{};
  std::condition_variable waitForServerStartedCond;
  bool serverStarted = false;
};
}
