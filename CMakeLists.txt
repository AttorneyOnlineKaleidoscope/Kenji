cmake_minimum_required(VERSION 3.7.0)

project(akashi VERSION 1.8.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT KAL_BUILD_OUTPUT)
set(KAL_BUILD_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif()

option(KAL_AKASHI_BUILD_TESTS "(${PROJECT_NAME}) Build ${PROJECT_NAME} tests" ON)
option(KAL_AKASHI_DEBUG_NETWORK "(${PROJECT_NAME}) Debug network" OFF)

if(NOT KAL_BUILD_OUTPUT)
set(KAL_BUILD_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif()

find_package(QT NAMES Qt6)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network WebSockets Sql REQUIRED)

if(NOT KAL_QT_VERSION)
set(KAL_QT_VERSION 6.5.3)
endif()

if(NOT Qt${QT_VERSION_MAJOR}_VERSION VERSION_EQUAL KAL_QT_VERSION)
  message(FATAL_ERROR "Supported version is Qt ${KAL_QT_VERSION}, found: ${Qt${QT_VERSION_MAJOR}_VERSION}")
endif()

add_library(${PROJECT_NAME} STATIC
  src/acl_roles_handler.cpp src/acl_roles_handler.h
  src/akashiutils.h
  src/aoclient.cpp src/aoclient.h
  src/area_data.cpp src/area_data.h
  src/client/clienthandshakeprocessor.h src/client/clienthandshakeprocessor.cpp
  src/client/clientprocessor.h src/client/clientprocessor.cpp
  src/client/clientsessionprocessor.h src/client/clientsessionprocessor.cpp
  src/commands/area.cpp
  src/commands/authentication.cpp
  src/commands/casing.cpp
  src/commands/command_helper.cpp
  src/commands/messaging.cpp
  src/commands/moderation.cpp
  src/commands/music.cpp
  src/commands/roleplay.cpp
  src/command_extension.cpp src/command_extension.h
  src/config_manager.cpp src/config_manager.h
  src/data_types.h
  src/db_manager.cpp src/db_manager.h
  src/discord.cpp src/discord.h
  src/ipidutils.h src/ipidutils.cpp
  src/logger/u_logger.cpp src/logger/u_logger.h
  src/logger/writer_full.cpp src/logger/writer_full.h
  src/logger/writer_modcall.cpp src/logger/writer_modcall.h
  src/music_manager.cpp src/music_manager.h
  src/packets.cpp
  src/playerstateobserver.cpp src/playerstateobserver.h
  src/server.cpp src/server.h
  src/serverpublisher.cpp src/serverpublisher.h
  src/testimony_recorder.cpp
  src/typedefs.h
)

target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PUBLIC
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::WebSockets
  Qt${QT_VERSION_MAJOR}::Sql
  turnabout-core
  turnabout-game
  turnabout-network
)

if(KAL_AKASHI_DEBUG_NETWORK)
target_compile_definitions(${PROJECT_NAME} PRIVATE KAL_AKASHI_DEBUG_NETWORK)
endif()

add_executable(${PROJECT_NAME}-runtime
  src/main.cpp
)

target_include_directories(${PROJECT_NAME}-runtime PUBLIC src)
target_link_libraries(${PROJECT_NAME}-runtime PRIVATE ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME}-runtime PROPERTIES RUNTIME_OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME}-runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${KAL_BUILD_OUTPUT}")

if(WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET ${PROJECT_NAME}-runtime PROPERTY WIN32_EXECUTABLE true)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resource/${PROJECT_NAME}.rc")
    target_sources(${PROJECT_NAME}-runtime PRIVATE ${APP_ICON_RESOURCE_WINDOWS})
  endif()
endif()

if(KAL_AKASHI_BUILD_TESTS)
  message(WARNING "${PROJECT_NAME} tests are not yet available.")
endif()
