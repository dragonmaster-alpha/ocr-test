project(ocr_test)

set(CMAKE_BUILD_TYPE Debug)

cmake_minimum_required (VERSION 2.6)


FIND_PACKAGE( OpenCV REQUIRED )

SET(ONNXRUNTIME_LIBS
    /storage/projects/alpr/libraries/libonnxruntime/build/Linux/Release/libonnxruntime.so.1.4.0
)
SET(ONNXRUNTIME_INCLUDE
    /storage/projects/alpr/libraries/libonnxruntime/include/
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/alprlog
    ${CMAKE_CURRENT_SOURCE_DIR}/src/alprgpusupport
    ${CMAKE_CURRENT_SOURCE_DIR}/src/alprsupport
    ${OpenCV_INCLUDE_DIRS}
    ${ONNXRUNTIME_INCLUDE}
)

add_subdirectory(src/alprlog)
add_subdirectory(src/alprsupport)
add_subdirectory(src/alprgpusupport)

ADD_EXECUTABLE(ocr_test  
    src/ocr_test.cpp
    src/ocr.cpp
    src/alprsupport/config.cpp

    src/backend/buffer_manager.cpp
    src/backend/onnxruntime.cpp
    src/postprocess/postprocess.cpp
    src/postprocess/utility.cpp

)

TARGET_LINK_LIBRARIES(ocr_test
    alprsupport
    alprgpusupport
    alprlog
    pthread
    ${OpenCV_LIBRARIES}

    ${ONNXRUNTIME_LIBS}
)
