CXX := clang++
CC := clang

BASEDIR = ../..
PHONELIBS = ../../phonelibs

CXXFLAGS := -g -O3 -fPIC -std=c++11 -Wall -Wextra -Wshadow -Weffc++ -Wstrict-aliasing -Wpedantic -Werror -MMD

LDLIBS=-lm -lstdc++ -lrt -lpthread

UNAME_M := $(shell uname -m)

YAML_FLAGS = -I$(PHONELIBS)/yaml-cpp/include
YAML_LIB = $(abspath $(PHONELIBS)/yaml-cpp/lib/libyaml-cpp.a)

ifeq ($(UNAME_M),aarch64)
	LDFLAGS += -llog -lgnustl_shared
	ZMQ_LIBS = /usr/lib/libzmq.a
endif
ifeq ($(UNAME_M),x86_64)
	ZMQ_FLAGS = -I$(BASEDIR)/phonelibs/zmq/x64/include
	ZMQ_LIBS = $(abspath $(BASEDIR)/phonelibs/zmq/x64/lib/libzmq.a)
	YAML_DIR = $(PHONELIBS)/yaml-cpp/x64/lib/
	YAML_LIB = $(abspath $(PHONELIBS)/yaml-cpp/x64/lib/libyaml-cpp.a)
endif

ifdef ASAN
	CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
  LDFLAGS += -fsanitize=address
endif

CXXFLAGS += $(ZMQ_FLAGS) $(YAML_FLAGS)

OBJS := messaging.o impl_zmq.o impl_msgq.o msgq.o
DEPS=$(OBJS:.o=.d)

.PRECIOUS: $(OBJS)
.PHONY: all clean
all: bridge messaging.a messaging_pyx.so messaging.so

demo: messaging.a demo.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -L. -l:messaging.a -o '$@'

bridge: messaging.a bridge.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -L. -l:messaging.a -o '$@'

messaging_pyx.so: messaging.a messaging_pyx_setup.py messaging_pyx.pyx messaging.pxd
	python3 messaging_pyx_setup.py build_ext --inplace
	rm -rf build
	rm -f messaging_pyx.cpp

messaging.so: $(OBJS)
	@echo "[ LINK ] $@"
	mkdir -p libs_so; \
		cd libs_so; \
		ar -x $(ZMQ_LIBS); \
		ar -x $(YAML_LIB);

	$(CXX) -shared $(LDFLAGS) $^ $(LDLIBS) libs_so/*.o  -o '$@'
	chmod 644 '$@'
	rm -r libs_so

%.a: $(OBJS)
	@echo "[ LINK ] $@"
	mkdir -p libs_a; \
		cd libs_a; \
		ar -x $(ZMQ_LIBS); \
		ar -x $(YAML_LIB);

	ar rcsD '$@' $^ libs_a/*.o
	rm -r libs_a

clean:
	@echo "[ CLEAN ]"
	rm -rf *.so *.a bridge demo libs_a libs_so $(OBJS) $(DEPS)

-include $(DEPS)
