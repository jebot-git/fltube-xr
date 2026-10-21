# The DomeD frontend uses the patched FLTK and native menu bridge via CMake.
BUILD_DIR ?= build-release
FLTK_PREFIX ?= $(abspath ../fltk-vr/_toolkit)
PREFIX ?= /usr/local
.PHONY: all install clean compile_fluid
all:
	cmake -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DCMAKE_PREFIX_PATH="$(FLTK_PREFIX)" -DCMAKE_INSTALL_PREFIX="$(PREFIX)"
	cmake --build "$(BUILD_DIR)" --parallel
install: all
	cmake --install "$(BUILD_DIR)" --strip
clean:
	cmake --build "$(BUILD_DIR)" --target clean
compile_fluid:
	cd fluid && fluid -o ../src/FLTube_View.cxx -h ../include/FLTube_View.h -c FLTube_View.fl
