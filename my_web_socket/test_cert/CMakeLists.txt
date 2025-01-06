target_sources(my_web_socket PUBLIC
    testCertClient.cxx
    testCertServer.cxx
)

install(FILES
        testCertServer.hxx
        testCertClient.hxx
        DESTINATION include/my_web_socket/test_cert
        )
install(TARGETS my_web_socket DESTINATION lib)