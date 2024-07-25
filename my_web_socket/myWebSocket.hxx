#ifndef FDE41782_20C3_436A_B415_E198F593F0AE
#define FDE41782_20C3_436A_B415_E198F593F0AE

#include <boost/asio/awaitable.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <fmt/color.h>
#include <memory>
#include <random>
#include <string>
namespace matchmaking_proxy
{

void printExceptionHelper (std::exception_ptr eptr);

template <class... Fs> struct overloaded : Fs...
{
  using Fs::operator()...;
};

template <class... Fs> overloaded (Fs...) -> overloaded<Fs...>;

auto const printException1 = [] (std::exception_ptr eptr) { printExceptionHelper (eptr); };

auto const printException2 = [] (std::exception_ptr eptr, auto) { printExceptionHelper (eptr); };

auto const printException = overloaded{ printException1, printException2 };

template <typename T>
T
rndNumber ()
{
  static std::random_device rd;       // Get a random seed from the OS entropy device, or whatever
  static std::mt19937_64 eng (rd ()); // Use the 64-bit Mersenne Twister 19937 generator
  std::uniform_int_distribution<T> distr{};
  return distr (eng);
}

template <class T> class MyWebsocket
{
public:
  explicit MyWebsocket (std::shared_ptr<T> webSocket_) : webSocket{ webSocket_ } {}
  MyWebsocket (std::shared_ptr<T> webSocket_, std::string loggingName_, fmt::text_style loggingTextStyleForName_, std::string id_) : webSocket{ webSocket_ }, loggingName{ std::move (loggingName_) }, loggingTextStyleForName{ std::move (loggingTextStyleForName_) }, id{ std::move (id_) } {}
  boost::asio::awaitable<std::string> async_read_one_message ();

  boost::asio::awaitable<void> readLoop (std::function<void (std::string const &readResult)> onRead);

  boost::asio::awaitable<void> async_write_one_message (std::string message);

  boost::asio::awaitable<void> writeLoop ();

  void sendMessage (std::string message);

  void close ();

  boost::asio::awaitable<void> sendPingToEndpoint ();

private:
  std::shared_ptr<T> webSocket{};
  std::string loggingName{};
  fmt::text_style loggingTextStyleForName{};
  std::string id{ std::to_string (rndNumber<uint16_t> ()) };
  std::deque<std::string> msgQueue{};
  typedef boost::asio::use_awaitable_t<>::as_default_on_t<boost::asio::basic_waitable_timer<boost::asio::chrono::system_clock> > CoroTimer;

  std::shared_ptr<CoroTimer> timer{};
};

void printTagWithPadding (std::string const &tag, fmt::text_style const &style, size_t maxLength);

}
#endif /* FDE41782_20C3_436A_B415_E198F593F0AE */
