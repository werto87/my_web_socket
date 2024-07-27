#include "my_web_socket/mockServer.hxx"
#include <boost/asio/co_spawn.hpp>
#include <catch2/catch.hpp>
#include <iostream>
boost::asio::awaitable<void>
sendMessageToWebSocketStartReadingHandleResponse (std::string message, std::function<void (std::string, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >)> handleResponse = {})
{
  auto webSocket = my_web_socket::WebSocket{ co_await boost::asio::this_coro::executor };
  auto endpoint = boost::asio::ip::tcp::endpoint{ boost::asio::ip::tcp::v4 (), 11111 };
  co_await boost::beast::get_lowest_layer (webSocket).async_connect (endpoint);
  co_await webSocket.async_handshake (endpoint.address ().to_string () + std::to_string (endpoint.port ()), "/");
  auto myWebSocket = std::make_shared<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > (my_web_socket::MyWebSocket{ std::move (webSocket) });
  boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->readLoop ([myWebSocket, handleResponse] (std::string msg) {
    if (handleResponse) handleResponse (msg, myWebSocket);
  }),
                         my_web_socket::printException);
  co_await myWebSocket->async_write_one_message (message);
}

TEST_CASE ("mockServerOption.callOnDestruct")
{
  auto mockServerOption = my_web_socket::MockServerOption{};
  auto success = bool{};
  mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
  {
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  }
  REQUIRE (success);
}

TEST_CASE ("mockServerOption.callOnMessageStartsWith")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  auto success = bool{};
  mockServerOption.callOnMessageStartsWith["shut down"] = [&success, &ioContext] () {
    success = true;
    ioContext.stop ();
  };
  auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), my_web_socket::printException);
  ioContext.run_for (std::chrono::seconds{ 1 });
  REQUIRE (success);
}

TEST_CASE ("mockServerOption.shutDownServerOnMessage")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  auto success = bool{};
  mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
  mockServerOption.shutDownServerOnMessage = "shut down";
  {
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 1 });
  }
  REQUIRE (success);
}

TEST_CASE ("mockServerOption.closeConnectionOnMessage")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  auto success = bool{};
  mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
  mockServerOption.closeConnectionOnMessage = "shut down";
  {
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 1 });
  }
  REQUIRE (success);
}

TEST_CASE ("mockServerOption.requestResponse")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.requestResponse["request"] = "response";
  auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  boost::asio::co_spawn (ioContext,
                         sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                           [] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                             REQUIRE (msg == "response");
                                                                             myWebSocket->close ();
                                                                           }),
                         my_web_socket::printException);
  ioContext.run ();
}

TEST_CASE ("mockServerOption.mockServerRunTime")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.mockServerRunTime = std::chrono::microseconds{ 100 };
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("request"), my_web_socket::printException);
  auto t1 = high_resolution_clock::now ();
  ioContext.run_for (std::chrono::seconds{ 2 });
  auto t2 = high_resolution_clock::now ();
  REQUIRE ((t2 - t1) < std::chrono::milliseconds{ 100 });
}

TEST_CASE ("mockServerOption.requestStartsWithResponse")
{
  auto ioContext = boost::asio::io_context{};
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.requestStartsWithResponse["req"] = "response";
  auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
  boost::asio::co_spawn (ioContext,
                         sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                           [] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                             REQUIRE (msg == "response");
                                                                             myWebSocket->close ();
                                                                           }),
                         my_web_socket::printException);
  ioContext.run ();
}
