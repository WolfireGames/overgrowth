PROCS ?= $(shell nproc)

default: setup build
setup:
	mkdir -p Build
	cd Build; cmake ../Projects
setup-release:
	mkdir -p Build
	cd Build; cmake -DCMAKE_BUILD_TYPE=Release -DNO_TELEMETRY=On -DBUILD_OGDA=Off ../Projects

setup-telemetry:
	mkdir -p Build
	cd Build; cmake -DCMAKE_BUILD_TYPE=Release -DNO_TELEMETRY=Off -DBUILD_OGDA=Off ../Projects

setup-debug:
	mkdir -p Build
	cd Build; cmake -DCMAKE_BUILD_TYPE=Debug -DNO_TELEMETRY=On -DBUILD_OGDA=Off ../Projects

setup-clang-release:
	mkdir -p Build
	cd Build; cmake -DCMAKE_BUILD_TYPE=Release  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ../Projects

activate-ogda:
	mkdir -p Build
	cd Build; cmake -DBUILD_OGDA=On ../Projects

deactivate-ogda:
	mkdir -p Build
	cd Build; cmake -DBUILD_OGDA=Off ../Projects

build: 
	$(MAKE) --no-print-directory -j$(PROCS) -C Build
run:
	cd Build; ./Overgrowth.bin.x86_64

debug:
	cd Build; cgdb ./Overgrowth.bin.x86_64

test: 
	cd Build; ./Overgrowth.bin.x86_64 --run-unit-tests
clean:
	$(MAKE) --no-print-directory -j$(PROCS) -C Build clean
