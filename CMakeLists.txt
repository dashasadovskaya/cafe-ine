cmake_minimum_required(VERSION 3.20)
project(cafe-in C CXX)

# Требуется C++17 для std::optional, structured binding и filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Поиск потоков (необходимы для SQLite и wxWidgets)
find_package(Threads REQUIRED)

# Поиск библиотеки wxWidgets для графического интерфейса
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

enable_testing()

# ---------------------------------------------------------------------------
# 1. Встраиваемый SQLite (SQLite/sqlite3.c, SQLite/sqlite3.h)
# ---------------------------------------------------------------------------
add_library(sqlite3 STATIC
    SQLite/sqlite3.c
    SQLite/sqlite3.h
)

target_include_directories(sqlite3 PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/SQLite
)

# Для работы SQLite на Linux/POSIX требуются библиотеки dl и pthread
target_link_libraries(sqlite3 PUBLIC 
    Threads::Threads 
    ${CMAKE_DL_LIBS}
)

# ---------------------------------------------------------------------------
# 2. Библиотека бизнес-логики и работы с БД (Logic)
# ---------------------------------------------------------------------------
add_library(Logic STATIC
    app/logic.cpp
    app/logic.h
    SQLite/database.cpp
    SQLite/database.h
)

target_include_directories(Logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/SQLite
)

target_link_libraries(Logic PUBLIC
    sqlite3
    Threads::Threads
)

# Корректная обработка русских UTF-8 строковых литералов в MSVC
if(MSVC)
    target_compile_options(Logic PUBLIC /utf-8)
    target_compile_options(sqlite3 PRIVATE /utf-8)
endif()

# ---------------------------------------------------------------------------
# 3. Графическое приложение (wxWidgets GUI)
# ---------------------------------------------------------------------------
add_executable(cafe-in 
    main.cpp
)

target_link_libraries(cafe-in PRIVATE
    Logic
    sqlite3
    ${wxWidgets_LIBRARIES}
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(MSVC)
    target_compile_options(cafe-in PRIVATE /utf-8)
endif()

# ---------------------------------------------------------------------------
# 4. Консольные тесты и демонстрационная программа БД
# ---------------------------------------------------------------------------

# Демонстрация работы бизнес-логики и персистентности базы данных
add_executable(cafe_db_demo 
    ./main.cpp
)

target_link_libraries(cafe_db_demo PRIVATE 
    Logic
)

if(MSVC)
    target_compile_options(cafe_db_demo PRIVATE /utf-8)
endif()

# Регистрация в CTest
add_test(NAME database_logic_test COMMAND cafe_db_demo)
