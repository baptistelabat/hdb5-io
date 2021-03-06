
find_package(HighFive QUIET)

if (NOT highfive_FOUND)
    include(FetchContent)

    #    set(FETCHCONTENT_QUIET OFF)

    FetchContent_Declare(highfive
            GIT_REPOSITORY ${highfive_URL}
            GIT_TAG ${highfive_TAG}
            )

    FetchContent_GetProperties(highfive)
    if(NOT highfive_POPULATED)
        message(STATUS "Downloading, Configuring and Generating 'HighFive' dependency")
        FetchContent_Populate(highfive)

        # Highfive BUILD OPTIONS
#        set(HIGHFIVE_USE_BOOST OFF CACHE BOOL "" FORCE)
#        set(HIGHFIVE_USE_EIGEN ON CACHE BOOL "" FORCE)
#        set(HIGHFIVE_USE_XTENSOR OFF CACHE BOOL "" FORCE)
#        set(HIGHFIVE_UNIT_TESTS OFF CACHE BOOL "" FORCE)

        add_subdirectory(${highfive_SOURCE_DIR} ${highfive_BINARY_DIR})
    else()
        message(STATUS "HighFive already populated")
    endif()
endif()

if (TARGET HighFive)
    get_target_property(INC HighFive INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "Found HighFive : ${INC}")
else()
    message(STATUS "HighFive target NOT FOUND")
endif()
