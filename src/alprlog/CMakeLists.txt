
# Stub out the alprlog (not log4cplus, just cout)
add_library(alprlog  alprlog_stub.cpp )



SET(ALPRLOG_MAJOR_VERSION "2")
set_target_properties(alprlog PROPERTIES SOVERSION ${ALPRLOG_MAJOR_VERSION})

