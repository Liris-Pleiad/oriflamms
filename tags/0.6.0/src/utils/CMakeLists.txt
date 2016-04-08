# oriflamms build script

# add path to oriflamms_config.h
include_directories("${PROJECT_BINARY_DIR}/oriflamms")
# add path to oriflamms
include_directories("${PROJECT_SOURCE_DIR}/utils")
# add path to dependencies
include_directories(${GTKCRNMM2_INCLUDE_DIRS})

# create signatures
add_executable(signatures signatures.cpp)
set_target_properties(signatures PROPERTIES LINK_FLAGS "-lstdc++")
target_link_libraries(signatures ${GTKCRNMM2_LIBRARIES})

