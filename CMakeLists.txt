

IF(WIN32)
    ADD_COMPILE_DEFINITIONS(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
ENDIF()

ADD_EXECUTABLE(redis_connector redis_connector.cpp "iocp.cpp" "iocp.h")
TARGET_LINK_LIBRARIES(redis_connector hiredisd)
