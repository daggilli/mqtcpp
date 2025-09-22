set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/subscriber/$<CONFIG>")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

set(EXE mqttsubscriber)

add_executable(${EXE} ${EXE}.cpp)

target_compile_options(${EXE} PRIVATE
  $<$<CONFIG:Debug>:-g -O1 -fno-omit-frame-pointer>
  $<$<CONFIG:Release>: -O4 -DNODEBUG -ffunction-sections -fdata-sections -fno-plt>
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_ASAN}>>:-fsanitize=address>
)

target_link_options(${EXE} PRIVATE
  $<$<CONFIG:Release>: -Wl,--gc-sections -Wl,--as-needed>
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_ASAN}>>:-fsanitize=address>
)

target_precompile_headers(${EXE} PRIVATE <print> <json/value.h> <json/reader.h>)

if (TARGET jsoncpp_lib)
  set_property(TARGET jsoncpp_lib PROPERTY MAP_IMPORTED_CONFIG_DEBUG Release)
  set_property(TARGET jsoncpp_lib PROPERTY MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
  target_link_libraries(${EXE} PRIVATE jsoncpp_lib)
endif ()

target_link_libraries(${EXE} PRIVATE ${PROJECT_NAME})
target_link_libraries(${EXE} PRIVATE PahoMqttCpp::paho-mqttpp3 ${PAHO_C_TARGET})
