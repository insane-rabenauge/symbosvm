APP=symbosvm
SOURCES=\
main.c\
simz80.c\
portio.c\
blitter.c\
video.c\
audio.c\
vmz80.c\
drives.c\
config.c\
system-sdl2.c\

ifeq ($(CROSS),w32)
CC=i686-w64-mingw32-gcc
SDLPATH=$(CROSSLIB)/SDL2-2.30.5/i686-w64-mingw32/bin/
XLDFLAGS=-Wl,-subsystem,console
SDLLIBS=`$(SDLPATH)sdl2-config --libs`
TARGETDIR=bin/win32
OBJDIR=obj/win32
else ifeq ($(CROSS),w64)
CC=x86_64-w64-mingw32-gcc
SDLPATH=$(CROSSLIB)/SDL2-2.30.5/x86_64-w64-mingw32/bin/
XLDFLAGS=-Wl,-subsystem,console
SDLLIBS=`$(SDLPATH)sdl2-config --libs`
TARGETDIR=bin/win64
OBJDIR=obj/win64
else
CC=gcc
SDLPATH=
XLDFLAGS=
SDLLIBS=`$(SDLPATH)sdl2-config --libs`
TARGETDIR=bin/linux
OBJDIR=obj/linux
endif

TARGET=$(TARGETDIR)/$(APP)
#CFLAGS=-Wall -Wno-switch -g
CFLAGS=-Wall -Wno-switch -Ofast
LDFLAGS=-lm
SDLFLAGS=`$(SDLPATH)sdl2-config --cflags`
CFILES=$(filter %.c,$(SOURCES))
OBJECTS=release.h $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))
DEPFILES=$(addprefix $(OBJDIR)/,$(addsuffix .d,$(basename $(CFILES))))
DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

.PHONY:	all
all:	$(TARGET)

.PHONY:	run
run:	$(TARGET)
	cd $(TARGETDIR) && ./$(APP)

.PHONY: clean
clean:	
	@rm -f $(DEPFILES) $(OBJECTS)

.PHONY:	backup
backup:
	find . -maxdepth 1 -type f ! -name SDL2.dll ! -name $(TARGET) ! -name $(TARGET).exe -print0 | tar cfz `date +old/v%Y%m%d%H%M.tar.gz` --null -T -
#	find . -path ./old -prune -o -type f ! -name SDL2.dll ! -name $(TARGET) ! -name $(TARGET).exe -print0 | tar cfz `date +old/v%Y%m%d%H%M.tar.gz` --null -T -

.PHONY: release.h
release.h:
	@date +"static const unsigned long v_release=%y%m%d%H%M;" > release.h
	@date +"static const char c_release[]=\"%y%m%d%H%M\";" >> release.h
	@date +"static const char c_build[]=\"%Y%m%d.%H%M\";" >> release.h

$(TARGET): $(OBJECTS) release.h
	@mkdir -p $(@D)
	@echo " LD $@"
	@$(CC) $(OBJECTS) -s -o $@ $(LDFLAGS) $(LDLIBS) $(XLDFLAGS) $(SDLLIBS)

$(OBJDIR)/%.o : %.c $(OBJDIR)/%.d
	@mkdir -p $(@D)
	@echo " CC $<"
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(SDLFLAGS) -c $< -o $@

$(DEPFILES):
include $(wildcard $(DEPFILES))
