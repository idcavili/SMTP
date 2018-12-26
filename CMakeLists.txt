project(smtp)

set(CMAKE_C_FLAGS "-Werror=implicit-function-declaration")

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED "glib-2.0")

include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})

pkg_check_modules(GMIME REQUIRED "gmime-2.6")

include_directories(${GMIME_INCLUDE_DIRS})
link_directories(${GMIME_LIBRARY_DIRS})

add_executable(smtp smtp.c)

target_link_libraries(smtp ${GLIB_LIBRARIES} ${GMIME_LIBRARIES} "iconv" "z")
