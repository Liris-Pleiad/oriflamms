# Oriflamms build script

configure_file(
	"${PROJECT_SOURCE_DIR}/oriflamms/oriflamms_config.h.in"
	"${PROJECT_BINARY_DIR}/oriflamms/oriflamms_config.h"
	)
# add path to oriflamms_config.h
include_directories("${PROJECT_BINARY_DIR}/oriflamms")
# add path to oriflamms
include_directories("${PROJECT_SOURCE_DIR}/oriflamms")
# add path to dependencies
include_directories(${LIBGTKCRNMM_INCLUDE_DIRS})
link_directories(${LIBGTKCRNMM_LIBRARY_DIRS})

# set sources
file(GLOB_RECURSE ORIFLAMMS_HEADERS "*.h")
source_group("Headers" FILES ${ORIFLAMMS_HEADERS})
file(GLOB_RECURSE ORIFLAMMS_SOURCES "*.cpp")
source_group("Sources" FILES ${ORIFLAMMS_SOURCES})
file(GLOB_RECURSE ORIFLAMMS_MISC "*.in")
list(APPEND ORIFLAMMS_MISC "CMakeLists.txt")
source_group("Misc" FILES ${ORIFLAMMS_MISC})

# create Titus
add_executable(oriflamms ${ORIFLAMMS_SOURCES} ${ORIFLAMMS_HEADERS})
set_target_properties(oriflamms PROPERTIES COMPILE_FLAGS "${GTKCRNMM_CFLAGS_OTHER}" LINK_FLAGS "${GTKCRNMM_LDFLAGS_OTHER} -lstdc++")
# add dependencies
target_link_libraries(oriflamms ${LIBGTKCRNMM_LIBRARIES})

# installation
install(TARGETS oriflamms DESTINATION ${RUNTIME_INSTALL_PATH})
install(FILES "icon.png" DESTINATION ${ORIFLAMMS_DATA_PATH})
#install(FILES "oriflamms64.png" DESTINATION ${ORIFLAMMS_DATA_PATH})
if(WIN32)
	install(FILES "oriflamms.ico" DESTINATION ${RUNTIME_INSTALL_PATH})
endif(WIN32)

install(FILES "orisig.xml" DESTINATION ${ORIFLAMMS_DATA_PATH})

if(UNIX)
	configure_file(
		"${PROJECT_SOURCE_DIR}/oriflamms/Oriflamms.desktop.in"
		"${PROJECT_BINARY_DIR}/oriflamms/Oriflamms.desktop"
		)
	install(FILES "${PROJECT_BINARY_DIR}/oriflamms/Oriflamms.desktop" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
endif(UNIX)
