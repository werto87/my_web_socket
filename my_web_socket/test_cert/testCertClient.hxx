//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_EXAMPLE_COMMON_ROOT_CERTIFICATES_HPP
#define BOOST_BEAST_EXAMPLE_COMMON_ROOT_CERTIFICATES_HPP

#include <boost/asio/ssl.hpp>

/*
    PLEASE READ

    These root certificates here are included just to make the
    SSL client examples work. They are NOT intended to be
    illustrative of best-practices for performing TLS certificate
    verification.

    A REAL program which needs to verify the authenticity of a
    server IP address resolved from a given DNS name needs to
    consult the operating system specific certificate store
    to validate the chain of signatures, compare the domain name
    properly against the domain name in the certificate, check
    the certificate revocation list, and probably do some other
    things.

    ALL of these operations are entirely outside the scope of
    both Boost.Beast and Boost.Asio.

    See (work in progress):
        https://github.com/djarek/certify

    tl;dr: root_certificates.hpp should not be used in production code
*/
/*
  edit by Waldemar Schneider
  function rename
  file rename
  split in hxx and cxx
  changed namespace
*/
namespace my_web_socket
{
namespace detail
{

void test_load_client_certificate (boost::asio::ssl::context &ctx, boost::system::error_code &ec);

} // detail

// Load the root certificates into an ssl::context

void test_load_client_certificate (boost::asio::ssl::context &ctx, boost::system::error_code &ec);

void test_load_client_certificate (boost::asio::ssl::context &ctx);
}
#endif