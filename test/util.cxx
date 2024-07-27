#include "util.hxx"

boost::asio::awaitable<std::shared_ptr<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > >
createMyWebSocket (boost::asio::ip::tcp::endpoint endpoint)
{
  auto webSocket = my_web_socket::WebSocket{ co_await boost::asio::this_coro::executor };
  co_await boost::beast::get_lowest_layer (webSocket).async_connect (endpoint);
  co_await webSocket.async_handshake (endpoint.address ().to_string () + std::to_string (endpoint.port ()), "/");
  co_return std::make_shared<my_web_socket::MyWebSocket<my_web_socket::WebSocket> > (my_web_socket::MyWebSocket{ std::move (webSocket) });
}