# Copyright (c) 2018 Cornell University.
#
# Author: Ted Yin <tederminant@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# This module will define:
# - Libevent_FOUND - if the library is found
# - LIBEVENT_INC - include directories
# - LIBEVENT_LIB - the libraries required to use libevent

set(LIBEVENT_PREFIXES /usr/local /opt/local)

foreach(prefix ${LIBEVENT_PREFIXES})
    list(APPEND LIBEVENT_LIB_PATH "${prefix}/lib")
    list(APPEND LIBEVENT_INC_PATH "${prefix}/include")
endforeach()

find_library(LIBEVENT_LIB NAMES event PATHS ${LIBEVENT_LIB_PATH})
find_path(LIBEVENT_INC event.h PATHS ${LIBEVENT_INC_PATH})

if (LIBEVENT_LIB AND LIBEVENT_INC)
    set(Libevent_FOUND TRUE)
    if (NOT Libevent_FIND_QUIETLY)
        message(STATUS "Found libevent: ${LIBEVENT_LIB}")
    endif ()
else ()
    set(Libevent_FOUND FALSE)
    if (Libevent_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find libevent.")
    endif ()
    message(STATUS "libevent NOT found.")
endif ()
