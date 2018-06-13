include_directories(${PROJECT_INCLUDE_DIR})


file(GLOB HEADERS ${PROJECT_INCLUDE_DIR} *.h)
file(GLOB SOURCES . *.cpp ${PROJECT_INCLUDE_DIR}/PowerTools++/*.h)
if (NOT ENABLE_GUROBI)
  list(REMOVE_ITEM SOURCES ${PROJECT_SOURCE_DIR}/src/GurobiProgram.cpp)
endif()

list(REMOVE_ITEM SOURCES ${PROJECT_SOURCE_DIR}/src/IpoptProgram.cpp)
#find_library(BLAS_LIBRARIES NAMES blas coinblas PATHS ENV LIBRARY_PATH)
#find_library(LAPACK_LIBRARIES NAMES lapack coinlapack PATHS ENV LIBRARY_PATH)
add_library(PowerTools++ ${SOURCES} ${HEADERS})



#target_link_libraries(PowerTools++ ${PLPLOT_LIBRARIES} ${GUROBI_LIBRARIES} ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES} ${IPOPT_LIBRARIES})


add_executable(PowerTools main.cpp)
target_link_libraries(PowerTools PowerTools++)
