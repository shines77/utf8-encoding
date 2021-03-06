function (GetOSEnv)
    if (JLANG_CMAKE_SHOW_DETAIL)
        message(STATUS "-------------------- Operating system --------------------")
        # Host system
        message(STATUS "CMAKE_HOST_SYSTEM           : ${CMAKE_HOST_SYSTEM}")
        message(STATUS "CMAKE_HOST_SYSTEM_NAME      : ${CMAKE_HOST_SYSTEM_NAME}")
        message(STATUS "CMAKE_HOST_SYSTEM_VERSION   : ${CMAKE_HOST_SYSTEM_VERSION}")
        message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR : ${CMAKE_HOST_SYSTEM_PROCESSOR}")
        message("")
        # Target build system, enable cross compiling
        message(STATUS "CMAKE_SYSTEM           : ${CMAKE_SYSTEM}")
        message(STATUS "CMAKE_SYSTEM_NAME      : ${CMAKE_SYSTEM_NAME}")
        message(STATUS "CMAKE_SYSTEM_VERSION   : ${CMAKE_SYSTEM_VERSION}")
        message(STATUS "CMAKE_SYSTEM_PROCESSOR : ${CMAKE_SYSTEM_PROCESSOR}")
        message("")

        # Operating system attribute
        if (WIN32)
            message(STATUS "STATUS: IS_WIN32")
        endif()
        if (WINCE)
            message(STATUS "STATUS: IS_WINCE")
        endif()
        if (WINDOWS_PHONE)
            message(STATUS "STATUS: IS_WINDOWS_PHONE")
        endif()
        if (WINDOWS_STORE)
            message(STATUS "STATUS: IS_WINDOWS_STORE")
        endif()
        if (ANDROID)
            message(STATUS "STATUS: IS_ANDROID")
        endif()
        if (APPLE)
            message(STATUS "STATUS: IS_APPLE")
        endif()
        if (IOS)
            message(STATUS "STATUS: IS_IOS")
        endif()
        if (UNIX)
            message(STATUS "STATUS: IS_UNIX")
        endif()

        message(STATUS "----------------------------------------------------------")
        message("")
    endif()
endfunction(GetOSEnv)