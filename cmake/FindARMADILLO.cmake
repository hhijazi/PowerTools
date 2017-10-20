set(LAPACK_ROOT_DIR "/home/kbestuzheva/lapack-3.7.1" CACHE PATH "LAPACK root directory.")

find_path(ARMADILLO_INCLUDE_DIR
        NAMES armadillo
        HINTS /usr/include
        HINTS /usr/local/include/
        )

find_path(LAPACKE_INCLUDE_DIR
        NAMES lapacke.h
        HINTS ${LAPACK_ROOT_DIR}/LAPACKE/include
        )

find_library(ARMADILLO_LIBRARY
        libarmadillo.so
        HINTS /usr/lib/x86_64-linux-gnu/
        )

find_library(LAPACKE_LIBRARY
        liblapacke.a
        HINTS ${LAPACK_ROOT_DIR}
        )

find_library(LAPACK_LIBRARY
        liblapack.a
        HINTS ${LAPACK_ROOT_DIR}
        )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ARMADILLO DEFAULT_MSG ARMADILLO_LIBRARY LAPACKE_LIBRARY LAPACK_LIBRARY ARMADILLO_INCLUDE_DIR LAPACKE_INCLUDE_DIR)

if(ARMADILLO_FOUND)
    message("—- Found Armadillo under ${ARMADILLO_INCLUDE_DIR}")
    message("—- Library ${ARMADILLO_LIBRARY}")
    set(ARMADILLO_INCLUDE_DIRS ${ARMADILLO_INCLUDE_DIR} ${LAPACKE_INCLUDE_DIR})
    set(ARMADILLO_LIBRARIES ${ARMADILLO_LIBRARY} ${LAPACKE_LIBRARY} ${LAPACKE_LIBRARY})
endif(ARMADILLO_FOUND)