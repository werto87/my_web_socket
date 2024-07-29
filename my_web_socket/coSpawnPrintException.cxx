
#include "my_web_socket/coSpawnPrintException.hxx"
#include <boost/numeric/conversion/cast.hpp>
#include <fmt/color.h>
namespace my_web_socket
{

#ifdef MY_WEB_SOCKET_LOG_CO_SPAWN_PRINT_EXCEPTIONS
void
printExceptionHelper (std::exception_ptr eptr)
{
  try
    {
      if (eptr)
        {
          std::rethrow_exception (eptr);
        }
    }
  catch (std::exception const &e)
    {
      std::cout << "co_spawn exception: '" << e.what () << "'" << std::endl;
    }
}
#else
void
printExceptionHelper (std::exception_ptr)
{
}
#endif

void
printTagWithPadding (std::string const &tag, fmt::text_style const &style, size_t maxLength)
{
  if (maxLength < 3) throw std::logic_error{ "maxLength should be min 3" };
  if (tag.length () > maxLength)
    {
      fmt::print (style, fmt::runtime ("[{:<" + std::to_string (maxLength) + "}]"), std::string{ tag.begin (), tag.begin () + boost::numeric_cast<int> (maxLength) - 3 } + "...");
    }
  else
    {
      fmt::print (style, "[{}]{}", tag, std::string (maxLength - tag.size (), '-'));
    }
}
}