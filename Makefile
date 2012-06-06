OBJS = cec.o
BIN = rpi-cecd

CC ?= gcc
AR ?= ar

CFLAGS ?= -I$(SDKSTAGE)/opt/vc/include/
OPTS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

LDFLAGS ?= -L$(SDKSTAGE)/opt/vc/lib/
LIBS = -lbcm_host -lvcos -lvchiq_arm

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(OPTS) -g -c $< -o $@ -Wno-deprecated-declarations

$(BIN) : $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(LDFLAGS) $(LIBS) $(OBJS) -Wl,--no-whole-archive -rdynamic

%.a: $(OBJS)
	$(AR) r $@ $^

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN)


