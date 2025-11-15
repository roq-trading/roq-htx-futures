set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES encoder.cpp map.cpp parser.cpp parser_2.cpp utils.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    account_info_data_item_contract_detail_item.json
    account_info_data_item_futures_contract_detail_item.json
    account_info_data_item.json
    account_info.json
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
    frame_2.json
    frame.json
    funding_rate_item.json
    funding_rate.json
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
    topic_2.json
    topic.json
    trade.json
    trade_tick_item.json
    trade_tick.json)

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen_cpp(
  OUTPUT
  AUTOGEN_SOURCES
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} OBJECT ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} autogen-json-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
