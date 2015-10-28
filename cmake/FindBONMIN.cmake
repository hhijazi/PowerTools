#set(BONMIN_INCLUDE_DIRS /home/kbestuzheva/Bonmin/Bonmin/src/Interfaces /home/kbestuzheva/Bonmin/CoinUtils/src /home/kbestuzheva/Bonmin/Osi/src/Osi /home/kbestuzheva/CoinIpopt/Ipopt/src/Common /home/kbestuzheva/CoinIpopt/Ipopt/src/Interfaces)
#set(BONMIN_INCLUDE_DIRS /home/kbestuzheva/Bonmin-1.8.4/Bonmin/src/Interfaces /home/kbestuzheva/Bonmin-1.8.4/CoinUtils/src /home/kbestuzheva/Bonmin-1.8.4/Osi/src/Osi /home/kbestuzheva/Bonmin-1.8.4/Ipopt/src/Common /home/kbestuzheva/Bonmin-1.8.4/Ipopt/src/Interfaces /home/kbestuzheva/Bonmin-1.8.4/Ipopt/src/LinAlg)

#set(BONMIN_LIBRARIES /home/kbestuzheva/Bonmin-1.8.4/Ipopt/src/Interfaces/libipopt.la /home/kbestuzheva/Bonmin-1.8.4/Cbc/src/libCbcSolver.la /home/kbestuzheva/Bonmin-1.8.4/Cbc/src/libCbc.la /home/kbestuzheva/Bonmin-1.8.4/Cgl/src/libCgl.la /home/kbestuzheva/Bonmin-1.8.4/Clp/src/OsiClp/libOsiClp.la /home/kbestuzheva/Bonmin-1.8.4/Clp/src/libClpSolver.la /home/kbestuzheva/Bonmin-1.8.4/Clp/src/libClp.la /home/kbestuzheva/Bonmin-1.8.4/Osi/src/Osi/libOsi.la /home/kbestuzheva/Bonmin-1.8.4/CoinUtils/src/libCoinUtils.la /home/kbestuzheva/Bonmin-1.8.4/src/CbcBonmin/libbonmin.la /home/kbestuzheva/Bonmin-1.8.4/Bonmin/lib/libbonmin.so)

#set(BONMIN_INCLUDE_DIRS /home/kbestuzheva/Bonmin-1.8.4/include/coin )

#set(BONMIN_LIBRARIES /home/kbestuzheva/Bonmin-1.8.4/Bonmin/lib/libbonmin.so /home/kbestuzheva/Bonmin-1.8.4/Bonmin/lib/libbonmin.la)
#set(BONMIN_LIBRARIES "${BONMIN_LIBRARIES};m;pthread")

##set(BONMIN_ROOT_DIR "$ENV{BONMIN_ROOT_DIR}" CACHE PATH "BONMIN root directory.")
#set(BONMIN_ROOT_DIR "/usr/local" CACHE PATH "BONMIN root directory.")
set(BONMIN_ROOT_DIR "/home/kbestuzheva/Bonmin-1.8.4" CACHE PATH "BONMIN root directory.")

message("Looking for Bonmin in ${BONMIN_ROOT_DIR}")

#string(REGEX MATCH "[0-9]+" BONMIN_VERSION "${BONMIN_ROOT_DIR}")

#message("Ipopt version ${BONMIN_VERSION}")

#string(SUBSTRING ${BONMIN_VERSION} 0 2 BONMIN_VERSION_SHORT)


find_path(BONMIN_INCLUDE_DIR
        NAMES BonTMINLP.hpp
        HINTS /usr/local/include/coin
        HINTS ${BONMIN_ROOT_DIR}/include/coin
        HINTS ${BONMIN_ROOT_DIR}/include
)

if(APPLE)
    find_library(BONMIN_LIBRARY
            libipopt.dylib
            HINTS /usr/local/lib
            HINTS ${BONMIN_ROOT_DIR}/lib
    )
elseif(UNIX)
    find_library(BONMIN_LIBRARY
            libbonmin.so
            HINTS /usr/local/lib
            HINTS ${BONMIN_ROOT_DIR}/lib
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BONMIN DEFAULT_MSG BONMIN_LIBRARY BONMIN_INCLUDE_DIR)

if(BONMIN_FOUND)
    message("—- Found Bonmin under ${BONMIN_INCLUDE_DIR}")
    set(BONMIN_INCLUDE_DIRS ${BONMIN_INCLUDE_DIR})
    set(BONMIN_LIBRARIES ${BONMIN_LIBRARY})
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(BONMIN_LIBRARIES "${BONMIN_LIBRARIES};m;pthread")
    endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif(BONMIN_FOUND)

mark_as_advanced(BONMIN_LIBRARY BONMIN_INCLUDE_DIR)