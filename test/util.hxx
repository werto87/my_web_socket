#pragma once
#include "my_web_socket/mockServer.hxx"
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > > createMyWebSocket (boost::asio::ip::tcp::endpoint endpoint = { boost::asio::ip::make_address ("127.0.0.1"), 11111 });
boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > createMySSLWebSocketClient (boost::beast::net::ssl::context &ctx, boost::asio::ip::tcp::endpoint endpoint = { boost::asio::ip::make_address ("127.0.0.1"), 11111 });

template <typename T>
boost::asio::awaitable<void>
sendMessageToWebSocketStartReadingHandleResponse (T messages, std::function<void (std::string, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >)> handleResponse = {})
{
  auto myWebSocket = co_await createMyWebSocket ();
  using namespace boost::asio::experimental::awaitable_operators;
  boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->sendPingToEndpoint (), my_web_socket::printException);
  boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->readLoop ([myWebSocket, handleResponse] (std::string msg) {
    if (handleResponse) handleResponse (msg, myWebSocket);
  }),
                         my_web_socket::printException);
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
sendPingAndMessageToWebSocketStartReadingHandleResponse (T messages, std::function<void (std::string, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >)> handleResponse = {})
{
  auto myWebSocket = co_await createMyWebSocket ();
  using namespace boost::asio::experimental::awaitable_operators;
  boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->sendPingToEndpoint () && myWebSocket->readLoop ([myWebSocket, handleResponse] (std::string msg) {
    if (handleResponse) handleResponse (msg, myWebSocket);
  }),
                         [myWebSocket] (auto) {}); // myWebSocket has to life until here. if we remove it from here it will die after readLoop ends and we get an read after free
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