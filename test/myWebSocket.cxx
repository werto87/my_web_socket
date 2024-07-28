#include "util.hxx"
#include <catch2/catch.hpp>

TEST_CASE ("myWebSocket")
{
  auto mockServerOption = my_web_socket::MockServerOption{};
  auto ioContext = boost::asio::io_context{};
  SECTION ("send message to mockServer")
  {
    auto success = bool{};
    mockServerOption.callOnMessageStartsWith["my message"] = [&success, &ioContext] () {
      success = true;
      ioContext.stop ();
    };
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (
        ioContext,
        [] () -> boost::asio::awaitable<void> {
          auto myWebSocket = co_await createMyWebSocket ();
          co_await myWebSocket->async_write_one_message ("my message");
          auto doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly = my_web_socket::CoroTimer{ co_await boost::asio::this_coro::executor };
          doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly.expires_after (std::chrono::system_clock::time_point::max () - std::chrono::system_clock::now ());
          co_await doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly.async_wait ();
        },
        my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 2 });
    REQUIRE (success);
  }
  SECTION ("send message to mockServer and read response")
  {
    auto success = bool{};
    mockServerOption.requestResponse["my message"] = "response";
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (
        ioContext,
        [&success, &ioContext] () -> boost::asio::awaitable<void> {
          auto myWebSocket = co_await createMyWebSocket ();
          boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->readLoop ([&success, &ioContext, myWebSocket] (std::string message) {
            if (message == "response")
              {
                success = true;
                ioContext.stop ();
              }
          }),
                                 my_web_socket::printException);
          co_await myWebSocket->async_write_one_message ("my message");
        },
        my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 2 });
    REQUIRE (success);
  }
  SECTION ("send message to mockServer using writeLoop with queueMessage")
  {
    auto success = bool{};
    mockServerOption.callOnMessageStartsWith["my message"] = [&success, &ioContext] () {
      success = true;
      ioContext.stop ();
    };
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (
        ioContext,
        [] () -> boost::asio::awaitable<void> {
          auto myWebSocket = co_await createMyWebSocket ();
          boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->writeLoop (), my_web_socket::printException);
          myWebSocket->queueMessage ("my message");
          auto doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly = my_web_socket::CoroTimer{ co_await boost::asio::this_coro::executor };
          doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly.expires_after (std::chrono::system_clock::time_point::max () - std::chrono::system_clock::now ());
          co_await doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly.async_wait ();
        },
        my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 2 });
    REQUIRE (success);
  }
  SECTION ("send message to mockServer using writeLoop with queueMessage and read response")
  {
    auto success = bool{};
    mockServerOption.requestResponse["my message"] = "response";
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (
        ioContext,
        [&success, &ioContext] () -> boost::asio::awaitable<void> {
          auto myWebSocket = co_await createMyWebSocket ();
          using namespace boost::asio::experimental::awaitable_operators;
          boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->writeLoop () || myWebSocket->readLoop ([&success, &ioContext, myWebSocket] (std::string message) {
            if (message == "response")
              {
                success = true;
                ioContext.stop ();
              }
          }),
                                 my_web_socket::printException);
          myWebSocket->queueMessage ("my message");
        },
        my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 2 });
    REQUIRE (success);
  }
  SECTION ("mock server disconnects")
  {
    auto success = bool{};
    mockServerOption.closeConnectionOnMessage = "please close connection";
    auto mockServer = my_web_socket::MockServer{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (
        ioContext,
        [&success, &ioContext] () -> boost::asio::awaitable<void> {
          auto myWebSocket = co_await createMyWebSocket ();
          using namespace boost::asio::experimental::awaitable_operators;
          boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->writeLoop () || myWebSocket->readLoop ([] (std::string) {}), [myWebSocket, &success, &ioContext] (std::exception_ptr, auto) {
            success = true;
            ioContext.stop ();
          });
          myWebSocket->queueMessage ("please close connection");
        },
        my_web_socket::printException);
    ioContext.run_for (std::chrono::seconds{ 2 });
    REQUIRE (success);
  }
}