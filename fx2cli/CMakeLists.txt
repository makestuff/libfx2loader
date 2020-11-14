project(fx2cli)

# Create an executable
file(GLOB SOURCES *.cpp *.c)
add_executable(${PROJECT_NAME} ${SOURCES})

# Dependencies
set(APP_DEPENDS fx2loader error usbwrap buffer argtable2)
target_link_libraries(${PROJECT_NAME} PRIVATE ${APP_DEPENDS})

# What to install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
