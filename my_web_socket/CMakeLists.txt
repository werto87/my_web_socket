##
## Copyright (c) 2024 Waldemar Schneider (w-schneider1987 at web dot de)
##
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


add_library(my_web_socket
myWebSocket.cxx
coSpawnPrintException.cxx
mockServer.cxx
)


add_library(myproject::my_web_socket ALIAS my_web_socket)
set(Logging)
if(LOG_CO_SPAWN_PRINT_EXCEPTIONS)
  list(APPEND Logging MY_WEB_SOCKET_LOG_CO_SPAWN_PRINT_EXCEPTIONS)
endif()
if(LOG_WRITE)
  list(APPEND Logging MY_WEB_SOCKET_LOG_WRITE)
endif()
if(LOG_READ)
  list(APPEND Logging MY_WEB_SOCKET_LOG_READ)
endif()
if(LOG_BOOST_ASIO)
  list(APPEND Logging BOOST_ASIO_ENABLE_HANDLER_TRACKING)
endif()

target_compile_definitions(my_web_socket PUBLIC 
${Logging}
)
find_package(certify)
find_package(fmt)
find_package(Boost)
target_link_libraries(my_web_socket PUBLIC
        Boost::headers
        certify::core
        fmt::fmt
)

target_include_directories(my_web_socket
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        )

target_compile_features(my_web_socket PUBLIC cxx_std_20)

set_target_properties(
        my_web_socket
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

install(FILES
        myWebSocket.hxx
        mockServer.hxx
        coSpawnPrintException.hxx
        DESTINATION include/my_web_socket
        )
install(TARGETS my_web_socket DESTINATION lib)
