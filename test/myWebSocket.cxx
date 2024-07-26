#include "my_web_socket/mockServer.hxx"
#include <boost/asio/co_spawn.hpp>
#include <catch2/catch.hpp>
#include <iostream>
boost::asio::awaitable<void>
sendMessageToWebSocketAndStartReading (std::string message)
{
  auto webSocket = my_web_socket::WebSocket{ co_await boost::asio::this_coro::executor };
  auto endpoint = boost::asio::ip::tcp::endpoint{ boost::asio::ip::tcp::v4 (), 11111 };
  co_await boost::beast::get_lowest_layer (webSocket).async_connect (endpoint);
  co_await webSocket.async_handshake (endpoint.address ().to_string () + std::to_string (endpoint.port ()), "/");
  auto myWebSocket = my_web_socket::MyWebSocket{ std::move (webSocket) };
  boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket.readLoop ([] (std::string msg) { std::cout << msg << std::endl; }), my_web_socket::printException);
  co_await myWebSocket.async_write_one_message (message);
}
TEST_CASE ("mockServerOption.shutDownServerOnMessage")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.shutDownServerOnMessage = "shut down";
  auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  boost::asio::co_spawn (ioContext, sendMessageToWebSocketAndStartReading ("shut down"), my_web_socket::printException);
  ioContext.run ();
}

TEST_CASE ("mockServerOption.closeConnectionOnMessage")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.closeConnectionOnMessage = "shut down";
  auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  boost::asio::co_spawn (ioContext, sendMessageToWebSocketAndStartReading ("shut down"), my_web_socket::printException);
  ioContext.run ();
}