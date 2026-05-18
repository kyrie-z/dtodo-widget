cmake_minimum_required(VERSION 3.16)

project(dtk-todo-widget VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(Dtk6Core REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Dtk6Widget REQUIRED)

add_executable(dtk-todo-widget
    main.cpp
    todowidget.cpp
    todowidget.h
    todoitem.cpp
    todoitem.h
)

target_link_libraries(dtk-todo-widget PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
)

target_include_directories(dtk-todo-widget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
