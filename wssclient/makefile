ifeq (Linux, $(shell uname -s))

complie := g++
symbols := strip
depends := ldd -r
binarys := ../bin/$(notdir $(shell pwd))
ccflags := -O3 -std=c++17 -pthread -static -static-libstdc++ -static-libgcc -lstdc++fs -Wl,--as-needed -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-value

sources := .
sources += ../core

all.hpp := $(foreach d, $(sources), $(foreach i, $(shell find $(d) -type d), $(wildcard $(i)/*.h)))
all.cpp := $(foreach d, $(sources), $(foreach i, $(shell find $(d) -type d), $(wildcard $(i)/*.cpp)))

$(binarys): $(all.hpp) $(all.cpp)
	$(complie) $(all.cpp) $(ccflags) -o $(binarys)
	-@$(depends) $(binarys) || true

clean:
	-@rm -fr $(binarys) || true

endif
