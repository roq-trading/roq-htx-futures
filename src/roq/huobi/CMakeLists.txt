set(TARGET_NAME "${PROJECT_NAME}")

add_subdirectory(flags)

set(SOURCES
    application.cpp
    config.cpp
    drop_copy.cpp
    flags.cpp
    gateway.cpp
    market_data.cpp
    order_entry.cpp
    security.cpp
    shared.cpp
    main.cpp)

add_subdirectory("json")

include(RoqAutogen)

set(AUTOGEN_SCHEMAS drop_copy_state.json market_data_state.json order_entry_state.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/huobi"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "h"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/huobi"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_executable("${TARGET_NAME}" ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-headers autogen-json-headers)

target_link_libraries(
  "${TARGET_NAME}"
  "${PROJECT_NAME}-json"
  "${PROJECT_NAME}-flags"
  roq-server::roq-server
  roq-core::roq-core
  roq-logging::roq-logging
  absl::flags
  absl::flat_hash_map
  absl::flat_hash_set
  ${UUID_LIBRARIES}
  ${RT_LIBRARIES})

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_17)

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

target_compile_definitions(
  "${TARGET_NAME}"
  PRIVATE ROQ_PACKAGE_NAME="${TARGET_NAME}" ROQ_BUILD_VERSION="${GIT_REPO_VERSION}"
          ROQ_GIT_DESCRIBE_HASH="${GIT_DESCRIBE_HASH}" ROQ_BUILD_TYPE="${ROQ_BUILD_TYPE}")

install(TARGETS ${TARGET_NAME})
