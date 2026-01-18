#include "my_web_socket/test_cert/testCertClient.hxx"
#include "my_web_socket/test_cert/testCertServer.hxx"
#include "util.hxx"
#include <catch2/catch_test_macros.hpp>
template <typename T, typename U>
void
supperTest (my_web_socket::MockServerOption const &defaultMockServerOption, U const &createWebsocket)
{

  SECTION ("myWebSocket")
  {
    auto mockServerOption = defaultMockServerOption;
    auto ioContext = boost::asio::io_context{};
    std::unique_ptr<my_web_socket::MockServer<my_web_socket::WebSocket> > mockServer;
    SECTION ("send message to mockServer")
    {
      auto success = bool{};
      mockServerOption.mockServerRunTime = std::chrono::seconds{ 1 };
      mockServerOption.callOnMessageStartsWith["my message"] = [&success, &mockServer] () {
        success = true;
        mockServer->shutDownUsingMockServerIoContext ();
      };
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      boost::asio::co_spawn (
          ioContext,
          [createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
            // co_await myWebSocket->asyncWriteOneMessage ("my message");
            // co_await myWebSocket->asyncClose (); // wait for close
          },
          my_web_socket::printException);
      ioContext.run ();
      // REQUIRE (success);
    }
    SECTION ("send message to mockServer and read response")
    {
      auto success = bool{};
      mockServerOption.requestResponse["my message"] = "response";
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      boost::asio::co_spawn (
          ioContext,
          [&success, &mockServer, createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
            boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->readLoop ([&success, &mockServer, myWebSocket] (std::string message) {
              if (message == "response")
                {
                  success = true;
                  mockServer->shutDownUsingMockServerIoContext ();
                }
            }),
                                   my_web_socket::printException);
            co_await myWebSocket->asyncWriteOneMessage ("my message");
          },
          my_web_socket::printException);
      ioContext.run ();
      REQUIRE (success);
    }
    SECTION ("send message to mockServer using writeLoop with queueMessage")
    {
      auto success = bool{};
      mockServerOption.callOnMessageStartsWith["my message"] = [&success, &mockServer] () {
        success = true;
        mockServer->shutDownUsingMockServerIoContext ();
      };
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      boost::asio::co_spawn (
          ioContext,
          [createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
            boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->writeLoop (), my_web_socket::printException);
            myWebSocket->queueMessage ("my message");
            auto doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly = my_web_socket::CoroTimer{ co_await boost::asio::this_coro::executor };
            doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly.expires_after (std::chrono::system_clock::time_point::max () - std::chrono::system_clock::now ());
            co_await doSomethingSoMyWebSocketDoesNotGetDestroyedTooEarly.async_wait ();
          },
          my_web_socket::printException);
      ioContext.run ();
      mockServer.reset ();
      REQUIRE (success);
    }
    SECTION ("send message to mockServer using writeLoop with queueMessage and read response")
    {
      auto success = bool{};
      mockServerOption.requestResponse["my message"] = "response";
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      boost::asio::co_spawn (
          ioContext,
          [&success, &mockServer, createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
            using namespace boost::asio::experimental::awaitable_operators;
            boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->writeLoop () || myWebSocket->readLoop ([&success, &mockServer, myWebSocket] (std::string message) {
              if (message == "response")
                {
                  success = true;
                  mockServer->shutDownUsingMockServerIoContext ();
                }
            }),
                                   my_web_socket::printException);
            myWebSocket->queueMessage ("my message");
          },
          my_web_socket::printException);
      ioContext.run ();
      REQUIRE (success);
    }
    SECTION ("mock server disconnects")
    {
      auto success = bool{};
      mockServerOption.closeConnectionOnMessage = "please close connection";
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      boost::asio::co_spawn (
          ioContext,
          [&success, &mockServer, createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
            using namespace boost::asio::experimental::awaitable_operators;
            boost::asio::co_spawn (co_await boost::asio::this_coro::executor, myWebSocket->writeLoop () || myWebSocket->readLoop ([] (std::string) {}), [myWebSocket, &success, &mockServer] (std::exception_ptr, auto) {
              success = true;
              mockServer->shutDownUsingMockServerIoContext ();
            });
            myWebSocket->queueMessage ("please close connection");
          },
          my_web_socket::printException);
      ioContext.run ();
      REQUIRE (success);
    }
  }
}

TEST_CASE ("my_web_socket::WebSocket")
{
  supperTest<my_web_socket::WebSocket> ({}, [] () -> boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > > { return createMyWebSocket (); });
}
TEST_CASE ("my_web_socket::SSLWebSocket without ssl check")
{
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.createSSLContext = [] () {
    auto sslContext = boost::beast::net::ssl::context{ boost::asio::ssl::context_base::method::tls_server };
    sslContext.set_verify_mode (boost::asio::ssl::context::verify_none);
    my_web_socket::test_load_server_certificate (sslContext);
    return sslContext;
  };
  auto sslContext = boost::beast::net::ssl::context{ boost::beast::net::ssl::context::tlsv12_client };
  sslContext.set_verify_mode (boost::asio::ssl::context::verify_none);
  my_web_socket::test_load_client_certificate (sslContext);
  supperTest<my_web_socket::SSLWebSocket> (mockServerOption, [&sslContext] () -> boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > { return createMySSLWebSocketClient (sslContext); });
}

TEST_CASE ("my_web_socket::SSLWebSocket")
{
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.createSSLContext = [] () {
    auto sslContext = boost::beast::net::ssl::context{ boost::asio::ssl::context_base::method::tls_server };
    my_web_socket::test_load_server_certificate (sslContext);
    return sslContext;
  };
  auto sslContext = boost::beast::net::ssl::context{ boost::beast::net::ssl::context::tlsv12_client };
  my_web_socket::test_load_client_certificate (sslContext);
  supperTest<my_web_socket::SSLWebSocket> (mockServerOption, [&sslContext] () -> boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > { return createMySSLWebSocketClient (sslContext); });
}