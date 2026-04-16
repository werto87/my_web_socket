#include <my_web_socket/coSpawnTraced.hxx>

namespace my_web_socket
{

void
logException (std::exception_ptr ep, std::string const &name)
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
          spdlog::info ("[{}] exception\n{}", name, e.what ());
        }
    }
#endif
}
}