#pragma once
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/system_error.hpp>

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <syncstream>
#include <type_traits>
#include <utility>

// mostly ai generated
namespace my_web_socket
{

namespace detail
{

template <typename T> struct is_awaitable_helper : std::false_type
{
};

template <typename T, typename Executor> struct is_awaitable_helper<boost::asio::awaitable<T, Executor> > : std::true_type
{
};

template <typename T> inline constexpr bool is_awaitable_v = is_awaitable_helper<std::decay_t<T> >::value;

template <typename T, typename = void> struct is_awaitable_factory_helper : std::false_type
{
};

template <typename T> struct is_awaitable_factory_helper<T, std::enable_if_t<std::is_invocable_v<T> > > : is_awaitable_helper<std::invoke_result_t<T> >
{
};

template <typename T> inline constexpr bool is_awaitable_factory_v = is_awaitable_factory_helper<std::decay_t<T> >::value;

template <typename Awaitable> decltype (auto) makeAwaitable (Awaitable &&aw, std::enable_if_t<is_awaitable_v<Awaitable>, int> = 0) { return std::forward<Awaitable> (aw); }

template <typename Factory>
auto
makeAwaitable (Factory &&f, std::enable_if_t<!is_awaitable_v<Factory> && is_awaitable_factory_v<Factory>, int> = 0)
{
  return std::invoke (std::forward<Factory> (f));
}

}

template <typename Executor, typename AwaitableOrFactory, typename CompletionHandler = std::nullptr_t>
void
coSpawnTraced (Executor ex, AwaitableOrFactory &&awaitableOrFactory, std::string name, CompletionHandler onCompletion = nullptr)
{
  boost::asio::co_spawn (
      ex,
      [awaitableOrFactory = std::forward<AwaitableOrFactory> (awaitableOrFactory), name] () mutable -> boost::asio::awaitable<void> {
        std::osyncstream (std::cerr) << "[" << name << "] start\n";
        co_await detail::makeAwaitable (std::move (awaitableOrFactory));
      },
      [name, onCompletion] ([[maybe_unused]] std::exception_ptr ep) {
        if constexpr (!std::is_same_v<CompletionHandler, std::nullptr_t>) onCompletion (ep);
        std::osyncstream (std::cerr) << "[" << name << "] finished\n";
#ifdef MY_WEB_SOCKET_LOG_CO_SPAWN_PRINT_EXCEPTIONS
        if (ep)
          {
            try
              {
                std::rethrow_exception (ep);
              }
            catch (std::exception const &e)
              {
                std::osyncstream (std::cerr) << "[" << name << "] exception: " << e.what () << "\n";
              }
          }
#endif
      });
}

template <typename AwaitableOrFactory, typename CompletionHandler = std::nullptr_t>
void
coSpawnTraced (boost::asio::io_context &ioContext, AwaitableOrFactory &&awaitableOrFactory, std::string name, CompletionHandler onCompletion = nullptr)
{
  coSpawnTraced (ioContext.get_executor (), std::forward<AwaitableOrFactory> (awaitableOrFactory), std::move (name), onCompletion);
}

} // namespace my_web_socket
