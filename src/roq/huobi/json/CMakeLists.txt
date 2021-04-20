set(TARGET_NAME "${PROJECT_NAME}-json")

set(SOURCES market_stream_parser.cpp user_stream_parser.cpp utils.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    account.json
    agg_trade.json
    balance.json
    balance_update.json
    balances_item.json
    book_ticker.json
    cancel_order.json
    depth.json
    depth_item.json
    depth_update.json
    error.json
    event_type.json
    exchange_info.json
    execution_report.json
    execution_type.json
    field.json
    fill.json
    filter_type.json
    listen_key.json
    mini_ticker.json
    new_order.json
    order_status.json
    order_type.json
    outbound_account_info.json
    outbound_account_position.json
    result.json
    side.json
    stream.json
    symbol.json
    symbol_status.json
    ticker.json
    time_in_force.json
    trade.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/huobi/json"
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
  "roq/huobi/json"
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
