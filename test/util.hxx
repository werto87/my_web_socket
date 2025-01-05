#pragma once
#include "my_web_socket/mockServer.hxx"
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > > createMyWebSocket (boost::asio::ip::tcp::endpoint endpoint = { boost::asio::ip::tcp::v4 (), 11111 });
boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > createMySSLWebSocketClient (boost::beast::net::ssl::context &ctx, boost::asio::ip::tcp::endpoint endpoint = { boost::asio::ip::tcp::v4 (), 11111 });

template <typename T>
boost::asio::awaitable<void>
sendMessageToWebSocketStartReadingHandleResponse (T messages, std::function<void (std::string, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >)> handleResponse = {})
{
  auto myWebSocket = co_await createMyWebSocket ();
  boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->readLoop ([myWebSocket, handleResponse] (std::string msg) {
    if (handleResponse) handleResponse (msg, myWebSocket);
  }),
                         my_web_socket::printException);
  using messageTyp = std::decay_t<T>;
  if constexpr (std::is_convertible_v<messageTyp, std::string>)
    {
      co_await myWebSocket->async_write_one_message (messages);
    }
  else
    {
      for (auto const &message : messages)
        {
          co_await myWebSocket->async_write_one_message (message);
        }
    }
}