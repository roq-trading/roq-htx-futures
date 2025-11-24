set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES encoder.cpp map.cpp parser.cpp parser_2.cpp utils.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    account_info_data_item_contract_detail_item.json
    account_info_data_item_futures_contract_detail_item.json
    account_info_data_item.json
    account_info.json
    accounts_data_item.json
    accounts.json
    auth_data.json
    auth.json
    basis.json
    basis_tick.json
    bbo.json
    bbo_tick.json
    cancel_all_orders_ack_data.json
    cancel_all_orders_ack.json
    cancel_order_ack_data.json
    cancel_order_ack.json
    close.json
    contract_info_data_item.json
    contract_info.json
    depth.json
    depth_tick.json
    detail.json
    detail_tick.json
    error_2.json
    error.json
    estimated_rate.json
    estimated_rate_tick.json
    event.json
    frame.json
    funding_rate_item.json
    funding_rate.json
    index.json
    index_tick.json
    match_orders.json
    match_orders_trade_item.json
    offset.json
    operator.json
    order_price_type.json
    orders.json
    orders_trade_item.json
    ping.json
    place_order_ack_data.json
    place_order_ack.json
    positions_data_item.json
    positions.json
    premium_index.json
    premium_index_tick.json
    quotation.json
    side.json
    status.json
    subbed.json
    sub.json
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
