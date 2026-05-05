#include "my_web_socket/coSpawnTraced.hxx"

// mostly ai generated start
namespace my_web_socket
{

void
print_exception (const std::exception &e)
{
  if (auto const *me = dynamic_cast<const boost::asio::multiple_exceptions *> (&e))
    {
      try
        {
          if (std::exception_ptr first = me->first_exception ())
            {
              std::rethrow_exception (first);
            }
        }
      catch (const std::exception &inner)
        {
          // for some reason we only can print the first exception
          spdlog::info ("exception: multiple exceptions (first: {})", inner.what ());
          return;
        }
    }
  spdlog::info ("exception: {}", e.what ());
}
}
// mostly ai generated stop