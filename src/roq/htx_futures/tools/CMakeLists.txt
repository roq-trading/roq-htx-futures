set(TARGET_NAME ${PROJECT_NAME}-tools)

set(SOURCES crypto.cpp rate_limit.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-web::roq-web roq-utils::roq-utils fmt::fmt)
