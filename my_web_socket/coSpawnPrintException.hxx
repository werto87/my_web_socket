#pragma once
#include <exception>
#include <fmt/color.h>
#include <string>
namespace my_web_socket
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

void printTagWithPadding (std::string const &tag, fmt::text_style const &style, size_t maxLength);
}