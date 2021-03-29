




set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        

add_library(alprsupport     
    filesystem.cpp
    platform.cpp
    timing.cpp
    string_utils.cpp
    version.cpp
    config_base.cpp
    config_base_impl.cpp
    config_helper.cpp
    profiler.cpp

    cpu_detect_arm.cpp
  )


TARGET_LINK_LIBRARIES(alprsupport
    ${OpenCV_LIBRARIES}

)


target_include_directories(alprsupport PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/alprsupport
    ${CMAKE_CURRENT_SOURCE_DIR}/re2
    ${OpenCV_INCLUDE_DIRS}
    )


