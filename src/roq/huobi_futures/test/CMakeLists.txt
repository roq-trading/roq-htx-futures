include(RoqAutogen)

set(TARGET_NAME ${PROJECT_NAME}-test)

set(SOURCES
    json_basis.cpp
    json_bbo.cpp
    json_contract_info.cpp
    json_depth.cpp
    json_detail.cpp
    json_funding_rate.cpp
    json_premium_index.cpp
    json_subbed.cpp
    json_trade.cpp
    json_utils.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(
  ${TARGET_NAME}
  ${PROJECT_NAME}-json
  roq-server::roq-server
  roq-core::roq-core
  roq-io::roq-io
  aklomp::base64
  fmt::fmt
  Catch2::Catch2
  ${RT_LIBRARIES})

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
