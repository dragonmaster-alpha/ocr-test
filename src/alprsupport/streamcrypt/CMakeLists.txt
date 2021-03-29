FIND_PACKAGE( Cryptopp REQUIRED )

# Encryption utility
ADD_EXECUTABLE( encdecfile  
  encrypt_file.cpp
  filecryptstream_cryptopp.cpp
  salsa20.cpp
)

TARGET_LINK_LIBRARIES(encdecfile
  ${Cryptopp_LIBRARIES}

)

target_include_directories(encdecfile PRIVATE ${Cryptopp_INCLUDE_DIRS})

set_target_properties(encdecfile PROPERTIES EXCLUDE_FROM_ALL TRUE)
