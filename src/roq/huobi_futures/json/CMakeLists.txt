set(TARGET_NAME "${PROJECT_NAME}-json")

set(SOURCES parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    bbo_frame.json
    bbo.json
    contract_info_item.json
    contract_info.json
    error.json
    frame.json
    ping.json
    quotation.json
    side.json
    status.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/huobi_futures/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "h"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/huobi_futures/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

set(GITIGNORE "${CMAKE_CURRENT_SOURCE_DIR}/.gitignore")

file(WRITE "${GITIGNORE}" "")

foreach(path ${AUTOGEN_SCHEMAS})
  get_filename_component(name ${path} NAME_WE)
  file(APPEND "${GITIGNORE}" "${name}.h\n${name}.cpp\n")
endforeach()

add_library("${TARGET_NAME}" STATIC ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-json-headers)

target_link_libraries("${TARGET_NAME}" roq-core::roq-core roq-logging::roq-logging fmt::fmt)
