include(RoqAutogen)

set(TARGET_NAME ${PROJECT_NAME}-test)

set(SOURCES
    crypto.cpp
    json_account_info_ack.cpp
    json_accounts.cpp
    json_accounts_cross.cpp
    json_auth.cpp
    json_basis.cpp
    json_bbo.cpp
    json_cancel_all_orders_ack.cpp
    json_cancel_order_ack.cpp
    json_contract_info_ack.cpp
    json_depth.cpp
    json_detail.cpp
    json_error_2.cpp
    json_error_3.cpp
    json_funding_rate.cpp
    json_match_orders.cpp
    json_match_orders_cross.cpp
    json_open_orders_ack.cpp
    json_orders.cpp
    json_orders_cross.cpp
    json_place_order_ack.cpp
    json_positions.cpp
    json_positions_cross.cpp
    json_premium_index.cpp
    json_subbed.cpp
    json_sub.cpp
    json_trade.cpp
    json_utils.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-json Catch2::Catch2)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
