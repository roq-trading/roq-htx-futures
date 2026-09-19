set(TARGET_NAME ${PROJECT_NAME}-protocol-json)

set(SOURCES
    encoder.cpp
    map.cpp
    parser.cpp
    parser_2.cpp
    parser_3.cpp
    parser_5.cpp
    utils.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    cancel_all_orders_ack5.json
    cancel_all_orders_ack5_data_item.json
    response5.json
    response5_single.json
    response5_multiple.json
    response5_multiple_item.json
    event5.json
    cancel_order_ack5.json
    cancel_order_ack5_data.json
    side.json
    order_type.json
    position_side.json
    time_in_force.json
    state.json
    positions5.json
    positions5_data_item.json
    account5.json
    account5_data.json
    account5_data_details_item.json
    open_orders_ack5.json
    open_orders_ack5_data_item.json
    place_order_ack5.json
    place_order_ack5_data.json
    match_orders5.json
    match_orders5_data_item.json
    contract_type.json
    order_state.json
    self_match_prevent.json
    orders5.json
    orders5_data.json
    #
    account_info_ack_data_item_contract_detail_item.json
    account_info_ack_data_item_futures_contract_detail_item.json
    account_info_ack_data_item.json
    account_info_ack.json
    accounts_cross_data_item_contract_detail_item.json
    accounts_cross_data_item.json
    accounts_cross.json
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
    close_2.json
    close.json
    contract_info_ack_data_item.json
    contract_info_ack.json
    depth.json
    depth_tick.json
    detail.json
    detail_tick.json
    direction.json
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
    margin_mode.json
    match_orders_cross.json
    match_orders.json
    match_orders_trade_item.json
    offset.json
    open_orders_ack_data.json
    open_orders_ack_data_orders_item.json
    open_orders_ack.json
    operator.json
    order_price_type.json
    orders_cross.json
    orders.json
    orders_trade_item.json
    ping.json
    place_order_ack_data.json
    place_order_ack.json
    position_mode.json
    positions_cross_data_item.json
    positions_cross.json
    positions_data_item.json
    positions.json
    premium_index.json
    premium_index_tick.json
    quotation.json
    rate_limit.json
    response_data_errors_item.json
    response_data.json
    response.json
    role.json
    status.json
    subbed.json
    sub.json
    topic_2.json
    topic_5.json
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

add_custom_target(protocol-json-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

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

add_dependencies(${TARGET_NAME} protocol-json-autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
