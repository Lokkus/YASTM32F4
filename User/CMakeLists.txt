# User application layer

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rtc_clock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/user_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_stdin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_stdout.c
)
