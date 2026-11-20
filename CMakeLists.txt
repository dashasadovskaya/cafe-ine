cmake_minimum_required(VERSION 3.20)
project(cafe-in C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate wxWidgets package and required components.
# README suggests the wxWidgets_ROOT environment variable on Windows, while
# CMake's FindwxWidgets looks for wxWidgets_ROOT_DIR, so bridge the two.
if(NOT wxWidgets_ROOT_DIR AND DEFINED ENV{wxWidgets_ROOT})
    set(wxWidgets_ROOT_DIR "$ENV{wxWidgets_ROOT}" CACHE PATH
        "wxWidgets installation directory")
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(Threads REQUIRED)

enable_testing()

# Embedded SQLite used by the database layer.
add_library(sqlite3 STATIC
    SQLite/sqlite3.c
    SQLite/sqlite3.h
)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/SQLite)

# Application logic is independent of the GUI and can be tested on its own.
add_library(Logic STATIC app/logic.cpp app/logic.h)
target_include_directories(Logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app)

if(MSVC)
    # Source files contain Russian UTF-8 string literals.
    target_compile_options(Logic PUBLIC /utf-8)
endif()

# GUI executable. All screens live in gui/; main.cpp only starts the app.
set(GUI_SOURCES
    gui/utils.cpp
    gui/main_frame.cpp
    gui/menu_panel.cpp
    gui/cart_panel.cpp
    gui/checkout_panel.cpp
    gui/profile_panel.cpp
    gui/about_panel.cpp
)

# WIN32 hides the console window on Windows.
if(WIN32)
    add_executable(cafe-in WIN32 main.cpp ${GUI_SOURCES})
else()
    add_executable(cafe-in main.cpp ${GUI_SOURCES})
endif()

target_include_directories(cafe-in PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    # GUI sources also contain Russian UTF-8 string literals.
    target_compile_options(cafe-in PRIVATE /utf-8)
endif()

target_link_libraries(cafe-in PRIVATE
    Logic
    sqlite3
    ${wxWidgets_LIBRARIES}
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Keep images/ next to the executable so relative paths such as
# "images/espresso.png" work both from the build tree and from an IDE.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/images)
    add_custom_command(TARGET cafe-in POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/images
            $<TARGET_FILE_DIR:cafe-in>/images
        COMMENT "Copying images next to the executable")
endif()

# On Windows the wxWidgets binary distribution is built as DLLs; copy them
# next to the executable so it can be launched directly from the build tree.
if(WIN32 AND wxWidgets_LIB_DIR)
    file(GLOB WX_RUNTIME_DLLS "${wxWidgets_LIB_DIR}/*.dll")
    if(WX_RUNTIME_DLLS)
        add_custom_command(TARGET cafe-in POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${WX_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:cafe-in>
            COMMENT "Copying wxWidgets runtime DLLs")
    endif()
endif()

# Console self-test / demo for the application logic.
add_executable(test_logic app/main.cpp)
target_link_libraries(test_logic PRIVATE Logic)

add_test(NAME logic_self_test COMMAND test_logic)
