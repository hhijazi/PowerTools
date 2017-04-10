#set(PLPLOT_ROOT_DIR "$ENV{PLPLOT_ROOT_DIR}" CACHE PATH "PLPLOT root directory.")


#string(REGEX MATCH "[0-9]+" PLPLOT_VERSION "${PLPLOT_ROOT_DIR}")

#message("PLPLOT version ${PLPLOT_VERSION}")

#string(SUBSTRING ${PLPLOT_VERSION} 0 2 PLPLOT_VERSION_SHORT)


find_path(PLPLOT_INCLUDE_DIR
	NAMES plplot.h
	HINTS /opt/local/include/plplot 
	HINTS /usr/include/plplot
	HINTS /usr/local/opt/plplot/include/plplot
	HINTS ${PLPLOT_ROOT_DIR}/include/coin
	HINTS ${PLPLOT_ROOT_DIR}/include
)

if(APPLE)
find_library(PLPLOT_LIBRARY 
	libplplotcxx.dylib
	HINTS /opt/local/lib
	HINTS /usr/local/lib
	HINTS /usr/local/opt/plplot/lib
	HINTS ${PLPLOT_ROOT_DIR}/lib
)
elseif(UNIX)
find_library(PLPLOT_LIBRARY 
	libplplotcxxd.so 
	HINTS /usr/lib
	HINTS ${PLPLOT_ROOT_DIR}/lib
)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PLPLOT DEFAULT_MSG PLPLOT_LIBRARY PLPLOT_INCLUDE_DIR)

if(PLPLOT_FOUND)
	message("—- Found PLPLOT under ${PLPLOT_INCLUDE_DIR}")
    set(PLPLOT_INCLUDE_DIRS ${PLPLOT_INCLUDE_DIR})
    set(PLPLOT_LIBRARIES ${PLPLOT_LIBRARY})
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(PLPLOT_LIBRARIES "${PLPLOT_LIBRARIES};m;pthread")
    endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif(PLPLOT_FOUND)

mark_as_advanced(PLPLOT_LIBRARY PLPLOT_INCLUDE_DIR)
