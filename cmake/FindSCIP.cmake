set(SCIP_DIR "/home/kbestuzheva/scip-3.2.1" CACHE PATH "IPOPT root directory.")
message("Looking for SCIP in ${SCIP_DIR}")

find_path(SCIP_INCLUDE_DIR cppmain.cpp HINTS "${SCIP_DIR}/src")

find_library(ZIMPL_LIBRARY libzimpl.linux.x86_64.gnu.opt.a HINTS /home/kbestuzheva/zimpl-3.3.2/lib)
find_library(SOPLEX_LIBRARY libsoplex.a HINTS /home/kbestuzheva/soplex-2.0.1/lib)
find_library(NLPI_LIBRARY libnlpi.cppad.so HINTS ${SCIP_DIR}/lib)
find_library(SPX_LIBRARY liblpispx.so HINTS ${SCIP_DIR}/lib)
find_library(SCIP_LIBRARY libscip.so HINTS ${SCIP_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SCIP DEFAULT_MSG SOPLEX_LIBRARY SPX_LIBRARY NLPI_LIBRARY SCIP_LIBRARY SCIP_INCLUDE_DIR)

if(SCIP_FOUND)
    set(SCIP_INCLUDE_DIRS ${SCIP_INCLUDE_DIR})
    set(SCIP_LIBRARIES ${SCIP_LIBRARY} ${ZIMPL_LIBRARY} ${SPX_LIBRARY} ${SOPLEX_LIBRARY} ${NLPI_LIBRARY} z readline gmp)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SCIP_LIBRARIES "${SCIP_LIBRARIES}")
    endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif(SCIP_FOUND)

mark_as_advanced(SCIP_LIBRARY SCIP_INCLUDE_DIR)