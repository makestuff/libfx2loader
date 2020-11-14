project(fx2loader)

# Create a shared library
file(GLOB SOURCES src/*.cpp src/*.c)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Ensure clients can find the includes
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Dependencies
set(LIB_DEPENDS common error usbwrap buffer)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_DEPENDS})

# What to install
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION bin
)
install(
  DIRECTORY include/makestuff DESTINATION include
)

# CLI tool
add_subdirectory(fx2cli)

# Maybe build tests
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
