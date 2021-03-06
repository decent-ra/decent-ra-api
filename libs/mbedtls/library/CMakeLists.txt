option(USE_STATIC_MBEDTLS_LIBRARY "Build mbed TLS static library." ON)
option(USE_SHARED_MBEDTLS_LIBRARY "Build mbed TLS shared library." OFF)
option(LINK_WITH_PTHREAD "Explicitly link mbed TLS library to pthread." OFF)
option(ENCLAVE_ENVIRONMENT_VERSION "Add copies of mbed TLS library, which supports enclave (For now, just support SGX)." ON)

set(src_crypto
    aes.c
    aesni.c
    arc4.c
    aria.c
    asn1parse.c
    asn1write.c
    base64.c
    bignum.c
    blowfish.c
    camellia.c
    ccm.c
    chacha20.c
    chachapoly.c
    cipher.c
    cipher_wrap.c
    cmac.c
    ctr_drbg.c
    des.c
    dhm.c
    ecdh.c
    ecdsa.c
    ecjpake.c
    ecp.c
    ecp_curves.c
    entropy.c
    entropy_poll.c
    error.c
    gcm.c
    havege.c
    hkdf.c
    hmac_drbg.c
    md.c
    md2.c
    md4.c
    md5.c
    md_wrap.c
    memory_buffer_alloc.c
    nist_kw.c
    oid.c
    padlock.c
    pem.c
    pk.c
    pk_wrap.c
    pkcs12.c
    pkcs5.c
    pkparse.c
    pkwrite.c
    platform.c
    platform_util.c
    poly1305.c
    ripemd160.c
    rsa.c
    rsa_internal.c
    sha1.c
    sha256.c
    sha512.c
    threading.c
    timing.c
    version.c
    version_features.c
    xtea.c
)

set(src_x509
    certs.c
    pkcs11.c
    x509.c
    x509_create.c
    x509_crl.c
    x509_crt.c
    x509_csr.c
    x509write_crt.c
    x509write_csr.c
)

set(src_tls
    debug.c
    net_sockets.c
    ssl_cache.c
    ssl_ciphersuites.c
    ssl_cli.c
    ssl_cookie.c
    ssl_srv.c
    ssl_ticket.c
    ssl_tls.c
)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes")
endif(CMAKE_COMPILER_IS_GNUCC)

if(CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes -Wdocumentation -Wno-documentation-deprecated-sync -Wunreachable-code")
endif(CMAKE_COMPILER_IS_CLANG)

if(UNSAFE_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
    set(CMAKE_C_FLAGS_ASAN "${CMAKE_C_FLAGS_ASAN} -Wno-error")
    set(CMAKE_C_FLAGS_ASANDBG "${CMAKE_C_FLAGS_ASANDBG} -Wno-error")
endif(UNSAFE_BUILD)

if(WIN32)
    set(libs ${libs} ws2_32)
endif(WIN32)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

if(HAIKU)
    set(libs ${libs} network)
endif(HAIKU)

if(USE_PKCS11_HELPER_LIBRARY)
    set(libs ${libs} pkcs11-helper)
endif(USE_PKCS11_HELPER_LIBRARY)

if(ENABLE_ZLIB_SUPPORT)
    set(libs ${libs} ${ZLIB_LIBRARIES})
endif(ENABLE_ZLIB_SUPPORT)

if(LINK_WITH_PTHREAD)
    set(libs ${libs} pthread)
endif()

if (NOT USE_STATIC_MBEDTLS_LIBRARY AND NOT USE_SHARED_MBEDTLS_LIBRARY)
    message(FATAL_ERROR "Need to choose static or shared mbedtls build!")
endif(NOT USE_STATIC_MBEDTLS_LIBRARY AND NOT USE_SHARED_MBEDTLS_LIBRARY)

if(ENCLAVE_ENVIRONMENT_VERSION AND NOT USE_STATIC_MBEDTLS_LIBRARY)
    message(FATAL_ERROR "Enclave environment must use static mbed TLS library!")
endif()

if(USE_STATIC_MBEDTLS_LIBRARY AND USE_SHARED_MBEDTLS_LIBRARY)
    set(mbedtls_static_target "mbedtls_static")
    set(mbedx509_static_target "mbedx509_static")
    set(mbedcrypto_static_target "mbedcrypto_static")

    if(ENCLAVE_ENVIRONMENT_VERSION)
        set(mbedtls_static_target_enclave "mbedtls_static_enclave")
        set(mbedx509_static_target_enclave "mbedx509_static_enclave")
        set(mbedcrypto_static_target_enclave "mbedcrypto_static_enclave")
    endif()

elseif(USE_STATIC_MBEDTLS_LIBRARY)
    set(mbedtls_static_target "mbedtls")
    set(mbedx509_static_target "mbedx509")
    set(mbedcrypto_static_target "mbedcrypto")

    if(ENCLAVE_ENVIRONMENT_VERSION)
        set(mbedtls_static_target_enclave "mbedtls_enclave")
        set(mbedx509_static_target_enclave "mbedx509_enclave")
        set(mbedcrypto_static_target_enclave "mbedcrypto_enclave")
    endif()

endif()

set(mbedtls_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if(USE_STATIC_MBEDTLS_LIBRARY)
    add_library(${mbedcrypto_static_target} STATIC ${src_crypto})
    set_target_properties(${mbedcrypto_static_target} PROPERTIES OUTPUT_NAME mbedcrypto)
    target_include_directories(${mbedcrypto_static_target} INTERFACE
        $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/mbedtls>
    )
    set_target_properties(${mbedcrypto_static_target} PROPERTIES FOLDER "mbedTLS")
    target_link_libraries(${mbedcrypto_static_target} ${libs})

    add_library(${mbedx509_static_target} STATIC ${src_x509})
    set_target_properties(${mbedx509_static_target} PROPERTIES OUTPUT_NAME mbedx509)
    target_include_directories(${mbedx509_static_target} INTERFACE
        $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/mbedtls>
    )
    set_target_properties(${mbedx509_static_target} PROPERTIES FOLDER "mbedTLS")
    target_link_libraries(${mbedx509_static_target} ${libs} ${mbedcrypto_static_target})

    add_library(${mbedtls_static_target} STATIC ${src_tls})
    set_target_properties(${mbedtls_static_target} PROPERTIES OUTPUT_NAME mbedtls)
    target_include_directories(${mbedtls_static_target} INTERFACE
        $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/mbedtls>
    )
    set_target_properties(${mbedtls_static_target} PROPERTIES FOLDER "mbedTLS")
    target_link_libraries(${mbedtls_static_target} ${libs} ${mbedx509_static_target})

    if(ENCLAVE_ENVIRONMENT_VERSION)
        add_library(${mbedcrypto_static_target_enclave} STATIC ${src_crypto})
        set_target_properties(${mbedcrypto_static_target_enclave} PROPERTIES OUTPUT_NAME mbedcrypto_enclave)
        target_include_directories(${mbedcrypto_static_target_enclave} INTERFACE
            $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:include/mbedtls>
        )
        set_target_properties(${mbedcrypto_static_target_enclave} PROPERTIES FOLDER "mbedTLS")
        target_link_libraries(${mbedcrypto_static_target_enclave} ${libs})
        target_compile_definitions(${mbedcrypto_static_target_enclave} PRIVATE ENCLAVE_ENVIRONMENT)

        add_library(${mbedx509_static_target_enclave} STATIC ${src_x509})
        set_target_properties(${mbedx509_static_target_enclave} PROPERTIES OUTPUT_NAME mbedx509_enclave)
        target_include_directories(${mbedx509_static_target_enclave} INTERFACE
            $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:include/mbedtls>
        )
        set_target_properties(${mbedx509_static_target_enclave} PROPERTIES FOLDER "mbedTLS")
        target_link_libraries(${mbedx509_static_target_enclave} ${libs} ${mbedcrypto_static_target_enclave})
        target_compile_definitions(${mbedx509_static_target_enclave} PRIVATE ENCLAVE_ENVIRONMENT)

        add_library(${mbedtls_static_target_enclave} STATIC ${src_tls})
        set_target_properties(${mbedtls_static_target_enclave} PROPERTIES OUTPUT_NAME mbedtls_enclave)
        target_include_directories(${mbedtls_static_target_enclave} INTERFACE
            $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:include/mbedtls>
        )
        set_target_properties(${mbedtls_static_target_enclave} PROPERTIES FOLDER "mbedTLS")
        target_link_libraries(${mbedtls_static_target_enclave} ${libs} ${mbedx509_static_target_enclave})
        target_compile_definitions(${mbedtls_static_target_enclave} PRIVATE ENCLAVE_ENVIRONMENT)
    endif()

    if(ENCLAVE_ENVIRONMENT_VERSION)
    install(TARGETS ${mbedtls_static_target} ${mbedx509_static_target} ${mbedcrypto_static_target} ${mbedtls_static_target_enclave} ${mbedx509_static_target_enclave} ${mbedcrypto_static_target_enclave}
            DESTINATION ${LIB_INSTALL_DIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    else()
    install(TARGETS ${mbedtls_static_target} ${mbedx509_static_target} ${mbedcrypto_static_target}
            DESTINATION ${LIB_INSTALL_DIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
endif(USE_STATIC_MBEDTLS_LIBRARY)

if(USE_SHARED_MBEDTLS_LIBRARY)
    add_library(mbedcrypto SHARED ${src_crypto})
    set_target_properties(mbedcrypto PROPERTIES VERSION 2.16.7 SOVERSION 3)
    target_include_directories(mbedcrypto INTERFACE
        $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/mbedtls>
    )
    target_link_libraries(mbedcrypto ${libs})

    add_library(mbedx509 SHARED ${src_x509})
    set_target_properties(mbedx509 PROPERTIES VERSION 2.16.7 SOVERSION 0)
    target_include_directories(mbedx509 INTERFACE
        $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/mbedtls>
    )
    target_link_libraries(mbedx509 ${libs} mbedcrypto)

    add_library(mbedtls SHARED ${src_tls})
    set_target_properties(mbedtls PROPERTIES VERSION 2.16.7 SOVERSION 12)
    target_include_directories(mbedtls INTERFACE
        $<BUILD_INTERFACE:${mbedtls_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/mbedtls>
    )
    target_link_libraries(mbedtls ${libs} mbedx509)

    install(TARGETS mbedtls mbedx509 mbedcrypto
            DESTINATION ${LIB_INSTALL_DIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif(USE_SHARED_MBEDTLS_LIBRARY)

add_custom_target(lib DEPENDS mbedcrypto mbedx509 mbedtls)
if(USE_STATIC_MBEDTLS_LIBRARY AND USE_SHARED_MBEDTLS_LIBRARY)
    add_dependencies(lib mbedcrypto_static mbedx509_static mbedtls_static)
endif()
