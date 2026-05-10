#pragma once
#include "my_web_socket/coSpawnTraced.hxx"
#include "my_web_socket/mockServer.hxx"
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > > createMyWebSocket (boost::asio::ip::tcp::endpoint endpoint);
boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > createMySSLWebSocketClient (boost::beast::net::ssl::context &ctx, boost::asio::ip::tcp::endpoint endpoint);

template <typename T>
boost::asio::awaitable<void>
sendMessageToWebSocketStartReadingHandleResponse (boost::asio::ip::tcp::endpoint endpoint, T messages, std::function<void (std::string, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >)> handleResponse = {})
{
  auto myWebSocket = co_await createMyWebSocket (endpoint);
  using namespace boost::asio::experimental::awaitable_operators;
  my_web_socket::coSpawnTraced (co_await boost::asio::this_coro::executor, myWebSocket->sendPingToEndpoint (), "test");
  my_web_socket::coSpawnTraced (co_await boost::asio::this_coro::executor,
                                myWebSocket->readLoop (
                                    [myWebSocket, handleResponse] (std::string msg)
                                      {
                                        if (handleResponse) handleResponse (msg, myWebSocket);
                                      }),
                                "test");
  using messageTyp = std::decay_t<T>;
  if constexpr (std::is_convertible_v<messageTyp, std::string>)
    {
      co_await myWebSocket->asyncWriteOneMessage (messages);
    }
  else
    {
      for (auto const &message : messages)
        {
          co_await myWebSocket->asyncWriteOneMessage (message);
        }
    }
}

template <typename T>
boost::asio::awaitable<void>
sendPingAndMessageToWebSocketStartReadingHandleResponse (boost::asio::ip::tcp::endpoint endpoint, T messages, std::function<void (std::string, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >)> handleResponse = {})
{
  auto myWebSocket = co_await createMyWebSocket (endpoint);
  using namespace boost::asio::experimental::awaitable_operators;
  my_web_socket::coSpawnTraced (co_await boost::asio::this_coro::executor,
                                myWebSocket->sendPingToEndpoint ()
                                    && myWebSocket->readLoop (
                                        [myWebSocket, handleResponse] (std::string msg)
                                          {
                                            if (handleResponse) handleResponse (msg, myWebSocket);
                                          }),
                                "test", [myWebSocket] (auto) {}); // myWebSocket has to life until here. if we remove it from here it will die after readLoop ends and we get an read after free
  using messageTyp = std::decay_t<T>;
  if constexpr (std::is_convertible_v<messageTyp, std::string>)
    {
      co_await myWebSocket->asyncWriteOneMessage (messages);
    }
  else
    {
      for (auto const &message : messages)
        {
          co_await myWebSocket->asyncWriteOneMessage (message);
        }
    }
}