# Put the binaries in the main output dir, not a subdirectory


INCLUDE_DIRECTORIES(
  ${OpenCV_INCLUDE_DIRS}
  ${AlprSupport_INCLUDE_DIR}
  ${AlprLog_INCLUDE_DIRS}
  ${OnnxRuntime_INCLUDE_DIR}
)

add_library(alprgpusupportcuda SHARED
  stub.cpp
  gpu_image_buffer_stub.cpp
  video_reader_accelerated_stub.cpp
)

TARGET_LINK_LIBRARIES(alprgpusupportcuda pthread)

set_target_properties(alprgpusupportcuda PROPERTIES LIBRARY_OUTPUT_NAME "alprgpusupportcuda")


