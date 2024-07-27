##
## Copyright (c) 2024 Waldemar Schneider (w-schneider1987 at web dot de)
##
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


add_library(my_web_socket myWebSocket.cxx)


add_library(myproject::my_web_socket ALIAS my_web_socket)
target_compile_definitions(my_web_socket PUBLIC 
MY_WEB_SOCKET_LOG_CO_SPAWN_PRINT_EXCEPTIONS
# MY_WEB_SOCKET_LOG_WRITE
# MY_WEB_SOCKET_LOG_READ_END
)
find_package(certify)
find_package(fmt)
find_package(Boost)
target_link_libraries(my_web_socket PUBLIC
        boost::boost
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
        DESTINATION include/my_web_socket
        )
install(TARGETS my_web_socket DESTINATION lib)
