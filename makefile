
TARGET := usbtest
BUILDDIR := build-$(TARGET)

NOECHO ?= @

# compiler flags, default libs to link against
COMPILEFLAGS := -g -O2
CFLAGS := -std=gnu99
CXXFLAGS :=
ASMFLAGS :=
LDFLAGS :=
LDLIBS := -lusb-1.0

UNAME := $(shell uname -s)
ARCH := $(shell uname -m)

# switch any platform specific stuff here
# ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# ...
# endif
ifeq ($(UNAME),Darwin)
COMPILEFLAGS += -I/opt/local/include
LDFLAGS += -L/opt/local/lib
CC := clang
endif

OBJS := main.o usb-libusb.o

OBJS := $(addprefix $(BUILDDIR)/,$(OBJS))

DEPS := $(OBJS:.o=.d)

.PHONY: all
all: $(BUILDDIR)/$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

spotless:
	rm -rf build-*

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

$(BUILDDIR)/%.o: %.c
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(COMPILEFLAGS) $(CFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.cpp
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(COMPILEFLAGS) $(CXXFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.S
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(COMPILEFLAGS) $(ASMFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/$(TARGET): $(OBJS)
	@$(MKDIR)
	@echo linking $<
	$(NOECHO)$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif
