LIBOUT       = libswegl.a
TESTOUT      = test
OBJDIR       = obj
OBJDIRT      = obj/swegltest

SRCS         = $(shell find src -name '*.cpp')
DIRS         = $(shell find src -type d | sed 's/src/./g' )
OBJS         = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))

SRCST        = $(shell find swegltest -name '*.cpp')
DIRST        = $(shell find swegltest -type d | sed 's/swegltest/./g' )
OBJST        = $(patsubst swegltest/%.cpp,$(OBJDIRT)/%.o,$(SRCST))

DEPDIR       = $(OBJDIR)
DEPDIRT      = $(OBJDIRT)
$(shell mkdir -p $(OBJDIR))
$(shell for dir in $(DIRS);  do mkdir -p $(OBJDIR)/$$dir; done)
$(shell mkdir -p $(OBJDIRT))
$(shell for dir in $(DIRST); do mkdir -p $(OBJDIRT)/$$dir; done)
DEPFLAGS     = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
DEPFLAGST    = -MT $@ -MMD -MP -MF $(DEPDIRT)/$*.Td
POSTCOMPILE  = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d
POSTCOMPILET = mv -f $(DEPDIRT)/$*.Td $(DEPDIRT)/$*.d

CC           = g++
CONFIG       = -g
CFLAGS       = $(DEPFLAGS) -std=c++11 $(CONFIG) -I. -I../freon/ -L. `sdl-config --cflags --libs`
CFLAGST      = $(DEPFLAGST) -std=c++11 $(CONFIG) -I. -I../freon/ -L. `sdl-config --cflags --libs`



all: $(LIBOUT) $(TESTOUT)

Debug: all
debug: all
release: CONFIG=-O3
release: all
Release: CONFIG=-O3
Release: all

$(OBJDIR)/%.o: src/%.cpp $(DEPDIR)/%.d
	@echo $<
	@$(CC) $(CFLAGS) -c $< -o $@
	@$(POSTCOMPILE)

$(OBJDIRT)/%.o: swegltest/%.cpp $(DEPDIRT)/%.d
	@echo $<
	@$(CC) $(CFLAGST) -c $< -o $@
	@$(POSTCOMPILET)

$(LIBOUT): $(OBJDIR) $(OBJS)
	@echo ;
	ar rcs $@ $(OBJS)
	@echo ;

$(TESTOUT): $(LIBOUT) $(OBJDIRT) $(OBJST)
	@echo ;
	$(CC) -o $@ $(OBJST) $(CFLAGS) $(LIBOUT)

clean:
	@rm -Rf $(LIBOUT) $(TESTOUT) $(OBJDIR) $(OBJDIRT)

swegl: $(LIBOUT)

# prevent deletion of .d files
.SECONDARY:

$(DEPDIR)/%.d: ;

$(DEPDIRT)/%.d: ;

-include $(patsubst src/%,$(DEPDIR)/%.d,$(basename $(SRCS)))
-include $(patsubst swegltest/%,$(DEPDIRT)/%.d,$(basename $(SRCST)))
