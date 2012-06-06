OBJS=cec.o
BIN=rpi-cecd

CFLAGS += \
	-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS 			  \
	-DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE 	  \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -ftree-vectorize -pipe  \
	-DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

LDFLAGS += -L$(SDKSTAGE)/opt/vc/lib/ -lbcm_host -lvchiq_arm

INCLUDES += -I$(SDKSTAGE)/opt/vc/include/

all: $(BIN) $(LIB)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations -Wall

$(BIN) : $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)


