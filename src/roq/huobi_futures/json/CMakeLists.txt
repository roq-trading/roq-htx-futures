set(TARGET_NAME "${PROJECT_NAME}-json")

set(SOURCES parser.cpp parser_2.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    basis.json
    basis_tick.json
    bbo.json
    bbo_tick.json
    close.json
    contract_info_item.json
    contract_info.json
    depth.json
    depth_tick.json
    detail.json
    detail_tick.json
    error.json
    estimated_rate.json
    estimated_rate_tick.json
    event.json
    frame.json
    frame_2.json
    funding_rate.json
    funding_rate_item.json
    index.json
    index_tick.json
    operator.json
    ping.json
    premium_index.json
    premium_index_tick.json
    quotation.json
    side.json
    status.json
    subbed.json
    topic.json
    topic_2.json
    trade.json
    trade_tick_item.json
    trade_tick.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/huobi_futures/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "hpp"
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

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}" ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library("${TARGET_NAME}" STATIC ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-json-headers)

target_link_libraries("${TARGET_NAME}" roq-core::roq-core roq-logging::roq-logging fmt::fmt-header-only)
