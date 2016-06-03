# Reads a Visual Studio 10 property sheet
# Fills prefix_INCLUDE_DIRS, prefix_CFLAGS_OTHER, prefix_LIBRARIES and prefix_LIBRARY_DIRS
function(READ_VS10_PROPS filename prefix)
	# Open file
	file(READ ${filename} content)
	# Read include dirs
	string(REGEX MATCH "<AdditionalIncludeDirectories>.*</AdditionalIncludeDirectories>" tmpstr "${content}")
	string(REPLACE "<AdditionalIncludeDirectories>" "" tmpstr "${tmpstr}")
	string(REPLACE "</AdditionalIncludeDirectories>" "" tmpstr "${tmpstr}")
	string(REPLACE ";%(AdditionalIncludeDirectories)" "" tmpstr "${tmpstr}")
	string(REGEX REPLACE "\\$\\(([^)]+)\\)" "\$ENV{\\1}" tmpstr "${tmpstr}")
	string(CONFIGURE "${tmpstr}" tmpstr)
	set(${prefix}_INCLUDE_DIRS "${tmpstr}" PARENT_SCOPE) #CACHE INTERNAL "Include directories for ${prefix}")
	# Read defines
	string(REGEX MATCH "<PreprocessorDefinitions>.*</PreprocessorDefinitions>" tmpstr "${content}")
	string(REPLACE "<PreprocessorDefinitions>" "" tmpstr "${tmpstr}")
	string(REPLACE "</PreprocessorDefinitions>" "" tmpstr "${tmpstr}")
	string(REPLACE ";%(PreprocessorDefinitions)" "" tmpstr "${tmpstr}")
	foreach(symbol ${tmpstr})
		set(tmpstr2 "${tmpstr2} /D${symbol}")
	endforeach(symbol)
	string(STRIP "${tmpstr2}" tmpstr)
	set(${prefix}_CFLAGS_OTHER "${tmpstr}" PARENT_SCOPE) #CACHE INTERNAL "Preprocessor directives for ${prefix}")
	# Read dependencies
	string(REGEX MATCH "<AdditionalDependencies>.*</AdditionalDependencies>" tmpstr "${content}")
	string(REPLACE "<AdditionalDependencies>" "" tmpstr "${tmpstr}")
	string(REPLACE "</AdditionalDependencies>" "" tmpstr "${tmpstr}")
	string(REPLACE ";%(AdditionalDependencies)" "" tmpstr "${tmpstr}")
	#string(REPLACE ".lib" "" tmpstr "${tmpstr}")
	set(${prefix}_LIBRARIES "${tmpstr}" PARENT_SCOPE) #CACHE INTERNAL "Dependencies for ${prefix}")
	# Read lib dirs
	string(REGEX MATCH "<AdditionalLibraryDirectories>.*</AdditionalLibraryDirectories>" tmpstr "${content}")
	string(REPLACE "<AdditionalLibraryDirectories>" "" tmpstr "${tmpstr}")
	string(REPLACE "</AdditionalLibraryDirectories>" "" tmpstr "${tmpstr}")
	string(REPLACE ";%(AdditionalLibraryDirectories)" "" tmpstr "${tmpstr}")
	string(REGEX REPLACE "\\$\\(([^)]+)\\)" "\$ENV{\\1}" tmpstr "${tmpstr}")
	string(CONFIGURE "${tmpstr}" tmpstr)
	set(${prefix}_LIBRARY_DIRS "${tmpstr}" PARENT_SCOPE) # CACHE INTERNAL "Library directories for ${prefix}")
endfunction(READ_VS10_PROPS)

