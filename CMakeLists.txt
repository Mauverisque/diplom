cmake_minimum_required(VERSION 3.15.0)
project(knn_app VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

set(CV_GUI_SOURCES
  src/main_window.cpp
  src/cv_analysis.cpp
  src/sqlite_database.cpp
)

set(CV_GUI_HEADERS
  include/main_window.h
  include/cv_analysis.h
  include/sqlite_database.h
)

add_library(cv_gui STATIC ${CV_GUI_SOURCES} ${CV_GUI_HEADERS})

target_include_directories(cv_gui PUBLIC include)

target_link_libraries(cv_gui PUBLIC
  Qt6::Core
  Qt6::Widgets
  Qt6::SerialPort
  ${OpenCV_LIBS}
  ${SQLITE3_LIBRARIES}
)

add_executable(knn_app src/main.cpp)
target_link_libraries(knn_app PRIVATE cv_gui)

target_compile_options(cv_gui PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(knn_app PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS cv_gui knn_app
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CV_GUI_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cv_gui
)
