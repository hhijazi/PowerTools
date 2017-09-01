set(IGRAPH_ROOT_DIR "/home/kbestuzheva/igraph-0.7.1" CACHE PATH "IGRAPH root directory.")

message("Looking for igraph in ${IGRAPH_ROOT_DIR}")

find_path(IGRAPH_INCLUDE_DIR
        NAMES igraph.h
        HINTS /usr/local/include/
        HINTS ${IGRAPH_ROOT_DIR}/include/
        )

find_library(IGRAPH_LIBRARY
        libigraph.so
        HINTS /usr/local/lib
        HINTS ${IGRAPH_ROOT_DIR}/lib
        )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IGRAPH DEFAULT_MSG IGRAPH_LIBRARY IGRAPH_INCLUDE_DIR)

if(IGRAPH_FOUND)
    message("—- Found igraph under ${IGRAPH_INCLUDE_DIR}")
    message("—- Library ${IGRAPH_LIBRARY}")
    set(IGRAPH_INCLUDE_DIRS ${IGRAPH_INCLUDE_DIR} ${IGRAPH_ROOT_DIR})
    set(IGRAPH_LIBRARIES "${IGRAPH_LIBRARY}")
endif(IGRAPH_FOUND)