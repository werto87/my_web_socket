#include "my_web_socket/coSpawnTraced.hxx"

// mostly ai generated start
namespace my_web_socket
{

void
printException (std::exception_ptr ep, std::string const &name)
{
#ifdef MY_WEB_SOCKET_LOG_CO_SPAWN_PRINT_EXCEPTIONS
  if (ep)
    {
      try
        {
          std::rethrow_exception (ep);
        }
      catch (std::exception const &e)
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
              catch (std::exception &const inner)
                {
                  // for some reason we only can print the first exception
                  spdlog::info ("exception: multiple exceptions (first: {})", inner.what ());
                  return;
                }
            }
          spdlog::info ("exception: {}", e.what ());
        }
      catch (...)
        {
          spdlog::info ("[{}] unknown non-std exception", name);
        }
    }

#endif
}

void
printStart (std::string const &name)
{
#ifdef MY_WEB_SOCKET_LOG_CO_SPAWN_START_AND_FINISH
  spdlog::info ("[{}] finished", name);
#endif
}

void
printEnd (std::string const &name)
{
#ifdef MY_WEB_SOCKET_LOG_CO_SPAWN_START_AND_FINISH
  spdlog::info ("[{}] finished", name);
#endif
}

}
// mostly ai generated stop