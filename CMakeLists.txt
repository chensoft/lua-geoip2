# lua-geoip2: geoip2 for lua
# Jian Chen <admin@chensoft.com>
# http://chensoft.com
cmake_minimum_required(VERSION 3.1)
project(geoip2 VERSION 1.0.0 LANGUAGES C)

add_library(geoip2 MODULE "")

# environment
set_target_properties(geoip2 PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
set_target_properties(geoip2 PROPERTIES PREFIX "")

# source codes
file(GLOB PROJ_INC src/*.h)
file(GLOB PROJ_SRC src/*.c)

# third party
find_package(Lua REQUIRED)
find_library(maxminddb REQUIRED)

# generate lib
target_include_directories(geoip2 PRIVATE ${LUA_INCLUDE_DIR})
target_sources(geoip2 PRIVATE ${PROJ_INC} ${PROJ_SRC})
target_link_libraries(geoip2 maxminddb ${LUA_LIBRARIES})

# install lib
install(TARGETS geoip2 DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")