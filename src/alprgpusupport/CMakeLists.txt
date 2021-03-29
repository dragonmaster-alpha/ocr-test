
option(BUILD_TEST "Build test programs" OFF)

    SET(LINUX_LIBS dl)



SET(CPU_SOURCES 

  alprgpusupport.cpp
  stub/stub.cpp
  stub/gpu_image_buffer_stub.cpp
  stub/video_reader_accelerated_stub.cpp
)

 

  add_library(alprgpusupport 
          ${CPU_SOURCES}

    )

  target_include_directories(alprgpusupport PUBLIC ${AlprLog_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})



