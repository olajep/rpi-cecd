OBJS = cec.o
BIN = rpi-cecd

CXX ?= g++

CXXFLAGS ?= \
	-I=/usr/include/interface/vcos/pthreads \
	-I=/opt/vc/include/ \
	-I=/opt/vc/include/interface/vcos/pthreads

OPTS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS            \
	   -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	   -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -ftree-vectorize     \
	   -pipe -DUSE_VCHIQ_ARM -Wno-psabi

LDFLAGS ?= -L=/opt/vc/lib/
LIBS = -lbcm_host -lvcos -lvchiq_arm -lcurl

all: $(BIN)

%.o: %.cpp
	@rm -f $@
	$(CXX) --sysroot=$(SYSROOT) $(CXXFLAGS) $(OPTS) -g -c $< -o $@ -Wall

$(BIN) : $(OBJS)
	$(CXX) --sysroot=$(SYSROOT) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) -rdynamic

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN)


