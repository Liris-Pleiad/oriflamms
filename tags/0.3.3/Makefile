all: .DEFAULT

.DEFAULT:
	+cd build ; make $(MAKECMDGOALS)

init:
	rm -rf build
	mkdir build
	cd build ; cmake ../src

init-gui:
	rm -rf build
	mkdir build
	cd build ; cmake-gui ../src

configure:
	cd build ; cmake-gui ../src

refresh:
	cd build ; make rebuild_cache
	+make

potfiles:
	cd src/po ; find .. -name '*.h' -or -name '*.hpp' -or -name '*.cpp' -or -name '*.cc' >POTFILES.in

cppcheck:
	cppcheck --enable=style,performance,portability --suppress=variableHidingTypedef --suppress=toomanyconfigs --inline-suppr --quiet `find src -name '*.cpp' -or -name '*.h' -or -name '*.hpp'`

uninstall:
	xargs rm < build/install_manifest.txt
