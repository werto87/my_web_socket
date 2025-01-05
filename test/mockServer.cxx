#include "util.hxx"
#include <catch2/catch.hpp>

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
        auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
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
        auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
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
      mockServerOption.callOnMessageStartsWith["shut down"] = [&success, &ioContext] () {
        success = true;
        ioContext.stop ();
      };
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), my_web_socket::printException);
      ioContext.run_for (std::chrono::seconds{ 1 });
      REQUIRE (success);
    }
    SECTION ("2 callbacks called")
    {
      auto success1 = bool{};
      auto success2 = bool{};
      mockServerOption.callOnMessageStartsWith["message1"] = [&success1, &success2, &ioContext] () {
        success1 = true;
        if (success1 and success2) ioContext.stop ();
      };
      mockServerOption.callOnMessageStartsWith["message2"] = [&success1, &success2, &ioContext] () {
        success2 = true;
        if (success1 and success2) ioContext.stop ();
      };
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("message1"), my_web_socket::printException);
      boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("message2"), my_web_socket::printException);
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
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), my_web_socket::printException);
      ioContext.run_for (std::chrono::seconds{ 1 });
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
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("shut down"), my_web_socket::printException);
      ioContext.run_for (std::chrono::seconds{ 1 });
    }
    REQUIRE (success);
  }

  SECTION ("requestResponse")
  {
    SECTION ("1 response")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestResponse["request"] = "response";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      SECTION ("one connection")
      {
        auto callBackCalled = bool{};
        boost::asio::co_spawn (ioContext,
                               sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                                 [&callBackCalled] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                                   REQUIRE (msg == "response");
                                                                                   callBackCalled = true;
                                                                                   myWebSocket->close ();
                                                                                 }),
                               my_web_socket::printException);
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalled);
      }
      SECTION ("two connections")
      {
        auto callBackCalledCount = uint64_t{};
        boost::asio::co_spawn (ioContext,
                               sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                                 [&callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                                   REQUIRE (msg == "response");
                                                                                   callBackCalledCount++;
                                                                                   myWebSocket->close ();
                                                                                 }),
                               my_web_socket::printException);
        boost::asio::co_spawn (ioContext,
                               sendMessageToWebSocketStartReadingHandleResponse ("request",
                                                                                 [&callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                                   REQUIRE (msg == "response");
                                                                                   callBackCalledCount++;
                                                                                   myWebSocket->close ();
                                                                                 }),
                               my_web_socket::printException);
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalledCount == 2);
      }
    }
    SECTION ("2 responses")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestResponse["request1"] = "response1";
      mockServerOption.requestResponse["request2"] = "response2";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      auto callBackCalledCount = uint64_t{};
      boost::asio::co_spawn (ioContext,
                             sendMessageToWebSocketStartReadingHandleResponse (std::vector{ "request1", "request2" },
                                                                               [&callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
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
                                                                                     myWebSocket->close ();
                                                                                   }
                                                                               }),
                             my_web_socket::printException);
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
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      SECTION ("one connection")
      {
        auto callBackCalled = bool{};
        boost::asio::co_spawn (ioContext,
                               sendMessageToWebSocketStartReadingHandleResponse ("requestabc",
                                                                                 [&callBackCalled] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                                   REQUIRE (msg == "response");
                                                                                   callBackCalled = true;
                                                                                   myWebSocket->close ();
                                                                                 }),
                               my_web_socket::printException);
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalled);
      }
      SECTION ("two connections")
      {
        auto callBackCalledCount = uint64_t{};
        boost::asio::co_spawn (ioContext,
                               sendMessageToWebSocketStartReadingHandleResponse ("requestabc",
                                                                                 [&callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                                   REQUIRE (msg == "response");
                                                                                   callBackCalledCount++;
                                                                                   myWebSocket->close ();
                                                                                 }),
                               my_web_socket::printException);
        boost::asio::co_spawn (ioContext,
                               sendMessageToWebSocketStartReadingHandleResponse ("requestabc",
                                                                                 [&callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
                                                                                   REQUIRE (msg == "response");
                                                                                   callBackCalledCount++;
                                                                                   myWebSocket->close ();
                                                                                 }),
                               my_web_socket::printException);
        ioContext.run_for (std::chrono::seconds{ 2 });
        REQUIRE (callBackCalledCount == 2);
      }
    }
    SECTION ("2 responses")
    {
      auto ioContext = boost::asio::io_context{};
      mockServerOption.requestStartsWithResponse["request1"] = "response1";
      mockServerOption.requestStartsWithResponse["request2"] = "response2";
      auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
      auto callBackCalledCount = uint64_t{};
      boost::asio::co_spawn (ioContext,
                             sendMessageToWebSocketStartReadingHandleResponse (std::vector{ "request1abc", "request2abc" },
                                                                               [&callBackCalledCount] (std::string const &msg, std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > myWebSocket) {
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
                                                                                     myWebSocket->close ();
                                                                                   }
                                                                               }),
                             my_web_socket::printException);
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
    auto mockServer = my_web_socket::MockServer<my_web_socket::WebSocket>{ { boost::asio::ip::tcp::v4 (), 11111 }, mockServerOption, "mock_server_test", fmt::fg (fmt::color::violet), "0" };
    boost::asio::co_spawn (ioContext, sendMessageToWebSocketStartReadingHandleResponse ("request"), my_web_socket::printException);
    auto t1 = high_resolution_clock::now ();
    ioContext.run_for (std::chrono::seconds{ 2 });
    auto t2 = high_resolution_clock::now ();
    REQUIRE ((t2 - t1) < std::chrono::milliseconds{ 100 });
  }
}