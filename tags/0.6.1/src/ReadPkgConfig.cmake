# Reads a PkgConfig configuration

find_program(PKG_CONFIG_BIN pkg-config DOC "Path of pkg-config utility")
mark_as_advanced(PKG_CONFIG_BIN)

function(READ_PKG_CONFIG libversion prefix)
	# check for PkgConfig installation
	if(PKG_CONFIG_BIN STREQUAL "PKG_CONFIG_BIN-NOTFOUND")
		message(SEND_ERROR "PkgConfig not found.")
	endif(PKG_CONFIG_BIN STREQUAL "PKG_CONFIG_BIN-NOTFOUND")

	# check for lib version
	execute_process(COMMAND ${PKG_CONFIG_BIN} --modversion "${libversion}"
		ERROR_QUIET
		OUTPUT_VARIABLE PKG_CONFIG_FOUND_VERSION
		RESULT_VARIABLE PKG_CONFIG_EXEC)
	string(REGEX REPLACE "\n$" "" PKG_CONFIG_FOUND_VERSION "${PKG_CONFIG_FOUND_VERSION}") # removing the last \n 
	if(PKG_CONFIG_EXEC)
		message(SEND_ERROR "Package ${libversion} not found.")
	endif(PKG_CONFIG_EXEC)

	# read cflags
	execute_process(COMMAND ${PKG_CONFIG_BIN} --cflags "${libversion}"
		ERROR_QUIET OUTPUT_VARIABLE PKG_CONFIG_CFLAGS)
	string(REGEX REPLACE "\n" " " PKG_CONFIG_CFLAGS "${PKG_CONFIG_CFLAGS}") # removing the last \n
	string(STRIP PKG_CONFIG_CFLAGS "${PKG_CONFIG_CFLAGS}") # removing spaces
	set(${prefix}_CFLAGS "${PKG_CONFIG_CFLAGS}" PARENT_SCOPE) # export raw string
	string(REPLACE " " ";" PKG_CONFIG_CFLAGS "${PKG_CONFIG_CFLAGS}") # removing the last \n
	foreach(token ${PKG_CONFIG_CFLAGS})
		string(LENGTH "${token}" tsize)
		if (tsize GREATER 1) # not empty
			string(SUBSTRING "${token}" 0 2 opt) # first characters
			if (opt STREQUAL "-I") # include path
				string(REPLACE "-I" "" token "${token}")
				list(APPEND INCLUDE_DIRS "${token}")
			else (opt STREQUAL "-I") # other
				set(CFLAGS_OTHER "${CFLAGS_OTHER} ${token}")
			endif (opt STREQUAL "-I")
		endif(tsize GREATER 1)
	endforeach(token ${PKG_CONFIG_CFLAGS})
	set(${prefix}_INCLUDE_DIRS "${INCLUDE_DIRS}" PARENT_SCOPE) # export
	set(${prefix}_CFLAGS_OTHER "${CFLAGS_OTHER}" PARENT_SCOPE)

	# read ldflags
	execute_process(COMMAND ${PKG_CONFIG_BIN} --libs "${libversion}"
		ERROR_QUIET OUTPUT_VARIABLE PKG_CONFIG_LDFLAGS)
	string(REGEX REPLACE "\n" " " PKG_CONFIG_LDFLAGS "${PKG_CONFIG_LDFLAGS}") # removing the last \n
	string(STRIP PKG_CONFIG_LDFLAGS "${PKG_CONFIG_LDFLAGS}") # removing spaces
	set(${prefix}_LDFLAGS "${PKG_CONFIG_LDFLAGS}" PARENT_SCOPE) # export raw string
	string(REPLACE " " ";" PKG_CONFIG_LDFLAGS "${PKG_CONFIG_LDFLAGS}") # removing the last \n
	foreach(token ${PKG_CONFIG_LDFLAGS})
		string(LENGTH "${token}" tsize)
		if (tsize GREATER 1) # not empty
			string(SUBSTRING "${token}" 0 2 opt) # first characters
			if (opt STREQUAL "-L") # library path
				string(REPLACE "-L" "" token "${token}")
				list(APPEND LIBRARY_DIRS "${token}")
			else (opt STREQUAL "-L")
				if (opt STREQUAL "-l") # library dependency
					string(REPLACE "-l" "" token "${token}")
					list(APPEND LIBRARIES "${token}")
				else (opt STREQUAL "-l") # other
					set(LDFLAGS_OTHER "${LDFLAGS_OTHER} ${token}")
				endif (opt STREQUAL "-l")
			endif (opt STREQUAL "-L")
		endif(tsize GREATER 1)
	endforeach(token ${PKG_CONFIG_LDFLAGS})
	set(${prefix}_LIBRARY_DIRS "${LIBRARY_DIRS}" PARENT_SCOPE) # export
	set(${prefix}_LIBRARIES "${LIBRARIES}" PARENT_SCOPE)
	set(${prefix}_LDFLAGS_OTHER "${LDFLAGS_OTHER}" PARENT_SCOPE)

endfunction(READ_PKG_CONFIG)
