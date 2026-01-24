#include "my_web_socket/coSpawnTraced.hxx"
#include "util.hxx"
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("mockServerOption")
{
  auto mockServerOption = my_web_socket::MockServerOption{};
  SECTION ("callOnDestruct")
  {
    SECTION ("1 function gets called")
    {
      auto success = bool{};
      mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
      {
        auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
        mockServer.shutDownUsingMockServerIoContext ();
      }
      REQUIRE (success);
    }
    SECTION ("2 functions get called")
    {
      auto success1 = bool{};
      auto success2 = bool{};
      mockServerOption.callAtTheEndOFDestruct.push_back ([&success1] () { success1 = true; });
      mockServerOption.callAtTheEndOFDestruct.push_back ([&success2] () { success2 = true; });
      {
        auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
        mockServer.shutDownUsingMockServerIoContext ();
      }
      REQUIRE (success1);
      REQUIRE (success2);
    }
  }

  SECTION ("callOnMessageStartsWith")
  {
    auto ioContext = boost::asio::io_context{};
    SECTION ("1 callback called")
    {
      auto success = bool{};
      std::unique_ptr<my_web_socket::MockServer<my_web_socket::WebSocket> > mockServer;
      mockServerOption.callOnMessageStartsWith["shut down"] = [&mockServer, &success, &ioContext] () {
        success = true;
        mockServer->shutDownUsingMockServerIoContext ();
      };
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      my_web_socket::coSpawnTraced (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), "test");
      ioContext.run ();
      REQUIRE (success);
    }
    SECTION ("2 callbacks called")
    {
      auto success1 = bool{};
      auto success2 = bool{};
      std::unique_ptr<my_web_socket::MockServer<my_web_socket::WebSocket> > mockServer;
      mockServerOption.callOnMessageStartsWith["message1"] = [&mockServer, &success1, &success2, &ioContext] () {
        success1 = true;
        if (success1 and success2) mockServer->shutDownUsingMockServerIoContext ();
      };
      mockServerOption.callOnMessageStartsWith["message2"] = [&mockServer, &success1, &success2, &ioContext] () {
        success2 = true;
        if (success1 and success2) mockServer->shutDownUsingMockServerIoContext ();
      };
      mockServer = std::make_unique<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      my_web_socket::coSpawnTraced (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("message1"), "test");
      my_web_socket::coSpawnTraced (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("message2"), "test");
      ioContext.run_for (std::chrono::seconds{ 1 });
      REQUIRE (success1);
      REQUIRE (success2);
    }
  }

  SECTION ("shutDownServerOnMessage")
  {
    auto ioContext = boost::asio::io_context{};
    auto success = bool{};
    mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
    mockServerOption.shutDownServerOnMessage = "shut down";
    {
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      my_web_socket::coSpawnTraced (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), "test");
      ioContext.run ();
    }
    REQUIRE (success);
  }

  SECTION ("closeConnectionOnMessage")
  {
    auto ioContext = boost::asio::io_context{};
    auto success = bool{};
    mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
    mockServerOption.closeConnectionOnMessage = "shut down";
    {
      auto mockServer = std::make_shared<my_web_socket::MockServer<my_web_socket::WebSocket> > (boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0");
      my_web_socket::coSpawnTraced (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), "test", [mockServer] (auto) { mockServer->shutDownUsingMockServerIoContext (); });
      ioContext.run ();
    }
    REQUIRE (success);
  }

  SECTION ("requestResponse")
  {
    SECTION ("1 response")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestResponse["request"] = "response";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      SECTION ("one connection")
      {
        auto callBackCalled = bool{};
        my_web_socket::coSpawnTraced (ioContext,
                                        sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                                          [&callBackCalled, &mockServer] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                            REQUIRE (msg == "response");
                                                                                            callBackCalled = true;
                                                                                            mockServer.shutDownUsingMockServerIoContext ();
                                                                                          }),
                                        "test");
        ioContext.run ();
        REQUIRE (callBackCalled);
      }
      SECTION ("two connections")
      {
        auto callBackCalledCount = uint64_t{};
        my_web_socket::coSpawnTraced (ioContext,
                                        sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                                          [&mockServer, &callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                            REQUIRE (msg == "response");
                                                                                            callBackCalledCount++;
                                                                                            if (callBackCalledCount == 2)
                                                                                              {
                                                                                                mockServer.shutDownUsingMockServerIoContext ();
                                                                                              }
                                                                                          }),
                                        "test");
        my_web_socket::coSpawnTraced (ioContext,
                                        sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                                          [&mockServer, &callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                            REQUIRE (msg == "response");
                                                                                            callBackCalledCount++;
                                                                                            if (callBackCalledCount == 2)
                                                                                              {
                                                                                                mockServer.shutDownUsingMockServerIoContext ();
                                                                                              }
                                                                                          }),
                                        "test");
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalledCount == 2);
      }
    }
    SECTION ("2 responses")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestResponse["request1"] = "response1";
      mockServerOption.requestResponse["request2"] = "response2";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      auto callBackCalledCount = uint64_t{};
      my_web_socket::coSpawnTraced (ioContext,
                                      sendMessageToWebSocketStartReadingHandleResponse (std::vector{ "request1", "request2" },
                                                                                        [&mockServer, &callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                          if (msg == "response1")
                                                                                            {
                                                                                              callBackCalledCount++;
                                                                                            }
                                                                                          else if (msg == "response2")
                                                                                            {
                                                                                              callBackCalledCount++;
                                                                                            }
                                                                                          if (callBackCalledCount == 2)
                                                                                            {
                                                                                              mockServer.shutDownUsingMockServerIoContext ();
                                                                                            }
                                                                                        }),
                                      "test");
      ioContext.run_for (std::chrono::seconds{ 2 });
      REQUIRE (callBackCalledCount == 2);
    }
  }
  SECTION ("request starts with response")
  {
    SECTION ("1 response")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestStartsWithResponse["request"] = "response";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      SECTION ("one connection")
      {
        auto callBackCalled = bool{};
        my_web_socket::coSpawnTraced (ioContext,
                                        sendMessageToWebSocketStartReadingHandleResponse ("requestabc",
                                                                                          [&mockServer, &callBackCalled] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                            REQUIRE (msg == "response");
                                                                                            callBackCalled = true;
                                                                                            mockServer.shutDownUsingMockServerIoContext ();
                                                                                          }),
                                        "test");
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalled);
      }
      SECTION ("two connections")
      {
        auto callBackCalledCount = uint64_t{};
        my_web_socket::coSpawnTraced (ioContext,
                                        sendMessageToWebSocketStartReadingHandleResponse ("requestabc",
                                                                                          [&mockServer, &callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                            REQUIRE (msg == "response");
                                                                                            callBackCalledCount++;
                                                                                            if (callBackCalledCount == 2)
                                                                                              {
                                                                                                mockServer.shutDownUsingMockServerIoContext ();
                                                                                              }
                                                                                          }),
                                        "test");
        my_web_socket::coSpawnTraced (ioContext,
                                        sendMessageToWebSocketStartReadingHandleResponse ("requestabc",
                                                                                          [&mockServer, &callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                            REQUIRE (msg == "response");
                                                                                            callBackCalledCount++;
                                                                                            if (callBackCalledCount == 2)
                                                                                              {
                                                                                                mockServer.shutDownUsingMockServerIoContext ();
                                                                                              }
                                                                                          }),
                                        "test");
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalledCount == 2);
      }
    }
    SECTION ("2 responses")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestStartsWithResponse["request1"] = "response1";
      mockServerOption.requestStartsWithResponse["request2"] = "response2";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      auto callBackCalledCount = uint64_t{};
      my_web_socket::coSpawnTraced (ioContext,
                                      sendMessageToWebSocketStartReadingHandleResponse (std::vector{ "request1abc", "request2abc" },
                                                                                        [&mockServer, &callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> >) {
                                                                                          if (msg == "response1")
                                                                                            {
                                                                                              callBackCalledCount++;
                                                                                            }
                                                                                          else if (msg == "response2")
                                                                                            {
                                                                                              callBackCalledCount++;
                                                                                            }
                                                                                          if (callBackCalledCount == 2)
                                                                                            {
                                                                                              mockServer.shutDownUsingMockServerIoContext ();
                                                                                            }
                                                                                        }),
                                      "test");
      ioContext.run_for (std::chrono::seconds{ 2 });
      REQUIRE (callBackCalledCount == 2);
    }
  }

  SECTION ("mockServerRunTime")
  {
    auto ioContext = boost::asio::io_context{};
    mockServerOption.mockServerRunTime = std::chrono::microseconds{ 100 };
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    my_web_socket::coSpawnTraced (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("request"), "test");
    auto t1 = high_resolution_clock::now ();
    ioContext.run ();
    auto t2 = high_resolution_clock::now ();
    REQUIRE ((t2 - t1) < std::chrono::milliseconds{ 100 });
  }
  SECTION ("shutDownServerOnMessage with ping ")
  {
    auto ioContext = boost::asio::io_context{};
    auto success = bool{};
    mockServerOption.callAtTheEndOFDestruct.push_back ([&success] () { success = true; });
    mockServerOption.shutDownServerOnMessage = "shut down";
    {
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::make_address ("127.0.0.1"), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      my_web_socket::coSpawnTraced (ioContext, sendPingAndMessageToWebSocketStartReadingHandleResponse ("shut down"), "test");
      ioContext.run ();
    }
    REQUIRE (success);
  }
}