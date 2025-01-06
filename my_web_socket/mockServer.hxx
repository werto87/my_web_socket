#pragma once

#include "my_web_socket/coSpawnPrintException.hxx"
#include "my_web_socket/myWebSocket.hxx"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/websocket.hpp>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <thread>
#include <variant>
namespace my_web_socket
{

struct SSLSuport
{
  std::filesystem::path pathToChainFile{};
  std::filesystem::path pathToPrivateFile{};
  std::filesystem::path pathToTmpDhFile{};
  bool sslContextVerifyNone{};
};

struct MockServerOption
{
  std::map<std::string, std::function<void ()> > callOnMessageStartsWith{};
  std::vector<std::function<void ()> > callAtTheEndOFDestruct{};
  std::optional<std::string> shutDownServerOnMessage{};
  std::optional<std::string> closeConnectionOnMessage{};
  std::map<std::string, std::string> requestResponse{};
  std::map<std::string, std::string> requestStartsWithResponse{};
  std::optional<std::chrono::microseconds> mockServerRunTime{};
  std::function<boost::beast::net::ssl::context ()> createSSLContext{};
};
template <class T = WebSocket> struct MockServer
{
  MockServer (boost::asio::ip::tcp::endpoint endpoint, MockServerOption const &mockServerOption_, std::string loggingName_ = {}, fmt::text_style loggingTextStyleForName_ = {}, std::string id_ = {});
  ~MockServer ();
  boost::asio::awaitable<void> serverShutDownTime ();
  boost::asio::awaitable<void> listener (boost::asio::ip::tcp::endpoint endpoint, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_);
  bool isRunning ();

private:
  MockServerOption mockServerOption{};
  boost::asio::io_context ioContext;
  std::thread thread{};
  std::list<MyWebSocket<T> > webSockets{};
  std::mutex waitForServerStarted{};
  std::condition_variable waitForServerStartedCond;
  bool serverStarted = false;
  std::optional<boost::beast::net::ssl::context> sslContext{};
};
}
