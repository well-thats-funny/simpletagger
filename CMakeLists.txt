#
# Copyright (C) 2024 fdresufdresu@gmail.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
cmake_minimum_required(VERSION 3.29)
project(simpletagger-cxx)
include(FetchContent)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SIMPLETAGGER_CXX_DEBUG_ENABLE_SANITIZERS ON CACHE BOOL "")
if (SIMPLETAGGER_CXX_DEBUG_ENABLE_SANITIZERS)
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

FetchContent_Declare(GSL
        GIT_REPOSITORY "https://github.com/gsl-lite/gsl-lite.git"
        GIT_TAG v0.41.0
        GIT_PROGRESS TRUE
)

set(TRACY_ENABLE OFF CACHE BOOL "")
set(TRACY_NO_CALLSTACK OFF CACHE BOOL "")
FetchContent_Declare(tracy
        GIT_REPOSITORY "https://github.com/wolfpld/tracy.git"
        GIT_TAG v0.11.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_Declare(boxicons
        GIT_REPOSITORY "https://github.com/atisawd/boxicons"
        GIT_TAG 9ffa9136e8681886bb7bd2145cd4098717ce1c11
        GIT_SHALLOW TRUE
        GIT_PROCESS TRUE
)

FetchContent_MakeAvailable(GSL tracy boxicons)

FetchContent_Declare(Qt_Advanced_Docking_System
        GIT_REPOSITORY "https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git"
        GIT_TAG 4.3.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(QT_VERSION_MAJOR 6)
set(BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(Qt_Advanced_Docking_System)
unset(QT_VERSION_MAJOR)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 COMPONENTS Core Widgets LinguistTools Test REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
