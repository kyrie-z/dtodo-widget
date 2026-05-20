cmake_minimum_required(VERSION 3.16)

project(dtodo-widget VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(Dtk6Core REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(X11 REQUIRED)

add_executable(dtodo-widget
    main.cpp
    todowidget.cpp
    todowidget.h
    todoitem.cpp
    todoitem.h
    resources.qrc
)

target_link_libraries(dtodo-widget PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
    X11
)

target_include_directories(dtodo-widget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${X11_INCLUDE_DIR}
)

# Install rules
install(TARGETS dtodo-widget RUNTIME DESTINATION bin)
install(FILES dtodo-widget.desktop DESTINATION share/applications)
install(FILES icons/dtodo-widget.svg DESTINATION share/icons/hicolor/scalable/apps)
