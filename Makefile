#Edit only these lines
LIBNAME = swegl


#All the stuff below is internals
LIBOUT       = lib/lib$(LIBNAME).a
TESTDIR      = $(LIBNAME)test
TESTOUT      = test
OBJDIR       = obj
OBJDIRT      = obj/$(TESTDIR)

SRCS         = $(shell find src -name '*.cpp')
DIRS         = $(shell find src -type d | sed 's/src/./g' )
OBJS         = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))

SRCST        = $(shell find $(TESTDIR) -name '*.cpp')
DIRST        = $(shell find $(TESTDIR) -type d | sed 's/$(TESTDIR)/./g' )
OBJST        = $(patsubst $(TESTDIR)/%.cpp,$(OBJDIRT)/%.o,$(SRCST))

DEPDIR       = $(OBJDIR)
DEPDIRT      = $(OBJDIRT)
$(shell mkdir -p $(OBJDIR))
$(shell for dir in $(DIRS);  do mkdir -p $(OBJDIR)/$$dir; done)
$(shell mkdir -p $(OBJDIRT))
$(shell for dir in $(DIRST); do mkdir -p $(OBJDIRT)/$$dir; done)
DEPFLAGS     = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
DEPFLAGST    = -MT $@ -MMD -MP -MF $(DEPDIRT)/$*.Td
POSTCOMPILE  = mv -f $(DEPDIR)/$*.Td  $(DEPDIR)/$*.d
POSTCOMPILET = mv -f $(DEPDIRT)/$*.Td $(DEPDIRT)/$*.d

CC           = g++
CONFIG       = -g
CFLAGS       = $(DEPFLAGS)  -Wall -std=c++2a $(CONFIG) -I. -I../freon/ -I../utttil/ -L. `sdl2-config --cflags --libs`
CFLAGST      = $(DEPFLAGST) -Wall -std=c++2a $(CONFIG) -I. -I../freon/ -I../utttil/ -L. `sdl2-config --cflags --libs`


all: $(LIBOUT) $(TESTOUT)

debug: all
release: CONFIG=-O3
release: all

$(OBJDIR)/%.o: src/%.cpp $(DEPDIR)/%.d
	@echo $<
	@$(CC) $(CFLAGS) -c $< -o $@
	@$(POSTCOMPILE)

$(OBJDIRT)/%.o: $(TESTDIR)/%.cpp $(DEPDIRT)/%.d
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

$(LIBNAME): $(LIBOUT)

# prevent deletion of .d files
.SECONDARY:

$(DEPDIR)/%.d: ;

$(DEPDIRT)/%.d: ;

-include $(patsubst src/%,$(DEPDIR)/%.d,$(basename $(SRCS)))
-include $(patsubst $(TESTDIR)/%,$(DEPDIRT)/%.d,$(basename $(SRCST)))
