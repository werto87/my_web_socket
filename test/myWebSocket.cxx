#include "util.hxx"
#include <catch2/catch.hpp>

template <typename T, typename U>
void
supperTest (my_web_socket::MockServerOption const &defaultMockServerOption, U const &createWebsocket)
{

  SECTION ("myWebSocket")
  {
    auto mockServerOption = defaultMockServerOption;
    auto ioContext = boost::asio::io_context{};
    SECTION ("send message to mockServer")
    {
      auto success = bool{};
      mockServerOption.callOnMessageStartsWith["my message"] = [&success, &ioContext] () {
        success = true;
        ioContext.stop ();
      };
      auto mockServer = my_web_socket::MockServer<T>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (
          ioContext,
          [createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
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
      auto mockServer = my_web_socket::MockServer<T>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (
          ioContext,
          [&success, &ioContext, createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
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
      auto mockServer = my_web_socket::MockServer<T>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
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
      ioContext.run_for (std::chrono::seconds{ 2 });
      REQUIRE (success);
    }
    SECTION ("send message to mockServer using writeLoop with queueMessage and read response")
    {
      auto success = bool{};
      mockServerOption.requestResponse["my message"] = "response";
      auto mockServer = my_web_socket::MockServer<T>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (
          ioContext,
          [&success, &ioContext, createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
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
      auto mockServer = my_web_socket::MockServer<T>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (
          ioContext,
          [&success, &ioContext, createWebsocket] () -> boost::asio::awaitable<void> {
            auto myWebSocket = co_await createWebsocket ();
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
    // TODO use test Cert for SERVER do not forget to use verify_none of this test
    //  sslContext.set_verify_mode (boost::asio::ssl::context::verify_peer);
    //  sslContext.set_default_verify_paths ();
    //  sslContext.use_certificate_chain_file (sslSupport.pathToChainFile);
    //  sslContext.use_private_key_file (sslSupport.pathToPrivateFile, boost::asio::ssl::context::pem);
    //  sslContext.use_tmp_dh_file (sslSupport.pathToTmpDhFile);
    //  boost::certify::enable_native_https_server_verification (sslContext);
    // sslContext.set_options (SSL_SESS_CACHE_OFF | SSL_OP_NO_TICKET); //  disable ssl cache. It has a bad support in boost asio/beast and I do not know if it helps in performance in our usecase
    return sslContext;
  };
  // TODO use test Cert for CLIENT
  auto sslContext = boost::beast::net::ssl::context{ boost::beast::net::ssl::context::tlsv12_client };
  supperTest<my_web_socket::SSLWebSocket> (mockServerOption, [&sslContext] () -> boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > { return createMySSLWebSocketClient (sslContext); });
}

TEST_CASE ("my_web_socket::SSLWebSocket")
{
  auto mockServerOption = my_web_socket::MockServerOption{};
  mockServerOption.createSSLContext = [] () {
    auto sslContext = boost::beast::net::ssl::context{ boost::asio::ssl::context_base::method::tls_server };
    sslContext.set_verify_mode (boost::asio::ssl::context::verify_none);
    // TODO use test Cert for SERVER
    //  sslContext.set_verify_mode (boost::asio::ssl::context::verify_peer);
    //  sslContext.set_default_verify_paths ();
    //  sslContext.use_certificate_chain_file (sslSupport.pathToChainFile);
    //  sslContext.use_private_key_file (sslSupport.pathToPrivateFile, boost::asio::ssl::context::pem);
    //  sslContext.use_tmp_dh_file (sslSupport.pathToTmpDhFile);
    //  boost::certify::enable_native_https_server_verification (sslContext);
    // sslContext.set_options (SSL_SESS_CACHE_OFF | SSL_OP_NO_TICKET); //  disable ssl cache. It has a bad support in boost asio/beast and I do not know if it helps in performance in our usecase
    return sslContext;
  };
  // TODO use test Cert for CLIENT do not forget to use verify_none of this test
  auto sslContext = boost::beast::net::ssl::context{ boost::beast::net::ssl::context::tlsv12_client };
  supperTest<my_web_socket::SSLWebSocket> (mockServerOption, [&sslContext] () -> boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > > { return createMySSLWebSocketClient (sslContext); });
}