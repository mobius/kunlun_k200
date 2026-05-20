XDNN_PATH:=$(XDNN_PATH)
XRE_PATH:=$(XRE_PATH)
LINK_TYPE:=$(LINK_TYPE)

LINKER_BEGIN=-Wl,--start-group
LINKER_END=-Wl,--end-group
# add -g to preserve debug symbol
export CFLAGS = -std=c++11 -O2 -fPIC -msse2 -fpermissive $(WARNINGS) $(MACROS)
export LDFLAGS = -lrt -lm -lpthread -ldl

INCLUDE := -I$(XDNN_PATH)/include -I$(XDNN_PATH)/plugin/include -I$(XRE_PATH)/include
ifeq ($(LINK_TYPE), static)
LINK_LIB := -L$(XDNN_PATH)/lib -L$(XDNN_PATH)/plugin/lib -L$(XRE_PATH)/lib -lxpuapi -lxpuplugin -lxpurt
else
LINK_LIB := -L$(XDNN_PATH)/so -L$(XDNN_PATH)/plugin/so -L$(XRE_PATH)/so -lxpuapi -lxpuplugin -lxpurt
endif
RPATH := -Wl,-rpath,$(XDNN_PATH)/so:$(XDNN_PATH)/plugin/so:$(XRE_PATH)/so

SRC := $(wildcard *.cpp)
BIN := $(patsubst %.cpp, %, $(SRC))

all: $(BIN)

%: %.cpp
	@mkdir -p $(@D)
	@if [ $(CXX) = "" ]; \
	then \
		CXX=g++; \
	else \
		CXX=$(CXX); \
	fi;\
	$$CXX $< $(CFLAGS) $(INCLUDE) -o $@ $(LDFLAGS) $(LINKER_BEGIN) $(LINK_LIB) $(LINKER_END) $(RPATH)

clean:
	@rm -rf *_test
