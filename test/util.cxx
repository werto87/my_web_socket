#include "util.hxx"

boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > >
createMyWebSocket (boost::asio::ip::tcp::endpoint endpoint)
{
  auto webSocket = my_web_socket::WebSocket{ co_await boost::asio::this_coro::executor };
  co_await boost::beast::get_lowest_layer (webSocket).async_connect (endpoint);
  co_await webSocket.async_handshake (endpoint.address ().to_string () + std::to_string (endpoint.port ()), "/");
  co_return std::make_shared<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > (my_web_socket::MyWebSocket{ std::move (webSocket) });
}

boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > >
createMySSLWebSocketClient (boost::beast::net::ssl::context &ctx, boost::asio::ip::tcp::endpoint endpoint)
{
  using namespace boost::asio;
  using namespace boost::beast;
  auto sslWebSocket = my_web_socket::SSLWebSocket{ co_await this_coro::executor, ctx };
  get_lowest_layer (sslWebSocket).expires_never ();
  sslWebSocket.set_option (websocket::stream_base::timeout::suggested (role_type::client));
  sslWebSocket.set_option (websocket::stream_base::decorator ([] (websocket::request_type &req) { req.set (http::field::user_agent, std::string (BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl"); }));
  co_await get_lowest_layer (sslWebSocket).async_connect (endpoint, use_awaitable);
  co_await sslWebSocket.next_layer ().async_handshake (ssl::stream_base::client, use_awaitable);
  co_await sslWebSocket.async_handshake ("localhost:" + std::to_string (endpoint.port ()), "/", use_awaitable);
  co_return std::make_shared<my_web_socket::MyWebSocket<my_web_socket::SSLWebSocket> > (my_web_socket::MyWebSocket{ std::move (sslWebSocket) });
}