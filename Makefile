
TYPE = release

OBJDIR_BASE = obj
BINDIR_BASE = bin

CFGDIR := cfg
SRCDIR := src
OBJDIR = $(OBJDIR_BASE)/$(CXX)_$(TYPE)
BINDIR = $(BINDIR_BASE)/$(CXX)_$(TYPE)
GENDIR = gen
DEPDIR := ..
TESTDIR := tests

CFLAGS_debug = -g -Wall -Wextra -fsanitize=address,leak
CFLAGS_perf = -O3 -Wall -Wextra -DNDEBUG -msse4 
CFLAGS_release = -g -O3 -Wall -Wextra -fno-omit-frame-pointer -DNDEBUG -msse4 
EXTRA_CFLAGS = 
CFLAGS = $(CFLAGS_$(TYPE)) $(EXTRA_CFLAGS) --std=c++2a -D_GLIBCXX_PARALLEL -I$(DEPDIR)/freon -I$(DEPDIR)/utttil -I. -I$(SRCDIR)/ 

LD=$(CXX)
LDFLAGS = -L. `sdl2-config --cflags --libs`
LDLIBS_debug = -lasan
LDLIBS_release = 
LDLIBS_perf = 
LDLIBS = $(LDLIBS_$(TYPE)) -pthread -L. `sdl2-config --cflags --libs` -fopenmp -lpng -ljpeg 
#-L$(DEPDIR)/abseil-cpp/build/absl/container/ -L$(DEPDIR)/abseil-cpp/build/absl/synchronization/ -L$(DEPDIR)/abseil-cpp/build/absl/time
#-labsl_hashtablez_sampler -labsl_synchronization -labsl_time

TIMEOUT := timeout

# all .d files
D_FILES = $(shell find $(OBJDIR) -name "*.d" 2> /dev/null)
# all cpp files not in the root of src/
all_cpps  = $(shell find $(SRCDIR)/* -type f -name '*.cpp')
all_objs  = $(addprefix $(OBJDIR)/,$(patsubst $(SRCDIR)/%.cpp,%.o,$(all_cpps)))
# all cpp files in the root of src/ and starting with test_
all_tests = $(patsubst $(SRCDIR)/%.cpp,%,$(wildcard $(SRCDIR)/test_*.cpp))
# all cpp files in the root of src/ that don't start with test_
all_bins  = $(filter-out $(all_tests), $(patsubst $(SRCDIR)/%.cpp,%,$(wildcard $(SRCDIR)/*.cpp)))
lib       = swegl.a

all: $(all_bins) $(all_tests)

release:
	@$(MAKE) -s all TYPE=$@
debug:
	@$(MAKE) -s all TYPE=$@

thorough: 
	$(eval COMPILERS=g++-latest g++-9.3 g++-8.4 clang++-latest clang++-8.0 clang++9.0)
	$(foreach COMPILER,$(COMPILERS),$(MAKE) --no-print-directory CXX=$(COMPILER) EXTRA_CFLAGS='-Wall -Werror' TYPE=debug docker;)

perf: $(TARGET)
	sudo perf record -g bin/$(TARGET)
	sudo perf report -g

loc:
	@git ls-files              | grep -E -- 'pp$$|Makefile' | grep -v -E -- '^src/test' | grep -v -E -- '^src/io/prot' | xargs wc -l | tail -n 1 | xargs printf "Code  : %5s\n" | head -n 1
	@git ls-files              | grep -E -- 'pp$$|Makefile' | grep    -E -- '^src/test'                                | xargs wc -l | tail -n 1 | xargs printf "Tests : %5s\n" | head -n 1
	@git ls-files              | grep -E -- 'pp$$|Makefile'                                                            | xargs wc -l | tail -n 1 | xargs printf "%13s Total\n"    | head -n 1

clean:
	rm -rf $(OBJDIR_BASE)
	rm -rf $(BINDIR_BASE)
	rm -rf $(SRCDIR)/headers.hpp.gch
	rm -rf perf.data*

RED=\033[1;31m
GREEN=\033[1;32m
YELLOW=\033[1;33m
NC=\033[0m # No Color

#do_%:
#	@if [ -f "$(BINDIR_BASE)/$*" ]; then \
#		printf '\t%-64s' $*; \
#		output=`$(TIMEOUT) 60s $(BINDIR_BASE)/$* 2>&1 >/dev/null`; \
#		if [ $$? -ne 0 ]; then \
#			echo "$(RED)[FAIL]$(NC)"; \
#			echo "$$output"; \
#		else \
#			echo "$(GREEN)[OK]$(NC)"; \
#		fi; \
#	else \
#		printf '\t%-64s$(YELLOW)[Not found]$(NC)\n' $(bin); \
#	fi; \
#	
#test: $(foreach bin,$(if $(TARGET),$(TARGET),$(all_tests)), do_$(bin))

$(TESTDIR): $(all_tests)
	@#pass

-include $(D_FILES)

-include $(OBJDIR)/headers.d

$(OBJDIR)/$(lib): $(all_objs)
	@mkdir -p $(@D)
	@echo $(lib)
	@ar rcs $@ $(all_objs)
	@mkdir -p $(BINDIR)
	@cp $@ $(BINDIR)/$(lib)

headers: $(SRCDIR)/headers.hpp.gch

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/headers.hpp.gch 
	@mkdir -p $(@D)
	@echo $<
	@$(CXX) $(CFLAGS) -c -MMD -o $@ $< $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc $(SRCDIR)/headers.hpp.gch
	@mkdir -p $(@D)
	@echo $<
	@$(CC) $(CFLAGS) -c -MMD -o $@ $< $(LDFLAGS)

%: $(BINDIR)/%
	@cp $< $(BINDIR_BASE)/$@
	@echo "Available in $(GREEN)$(BINDIR_BASE)/$@$(NC)"

$(SRCDIR)/headers.hpp.gch: $(SRCDIR)/headers.hpp
	@echo Precompiling headers
	@mkdir -p $(OBJDIR)
	@$(CXX) -MMD -MF $(OBJDIR)/headers.d -MT $(SRCDIR)/headers.hpp.gch $(CFLAGS) $<
	@$(CXX) $(CFLAGS) $<

$(BINDIR)/%: $(OBJDIR)/%.o $(OBJDIR)/$(lib)
	$(eval rule := $(shell cat $(OBJDIR)/$*.d))
	$(eval hdrs := $(filter %.hpp,$(rule)))
	$(eval old_objs := $(filter %.o,$(rule)))
	$(eval cpps := $(patsubst $(SRCDIR)/%.hpp,$(SRCDIR)/%.cpp,$(hdrs)))
	$(eval cpps := $(filter $(all_cpps),$(cpps)))
	$(eval objs := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(cpps)))
	$(eval new_objs := $(filter-out $(old_objs),$(objs)))
	$(if $(new_objs), $(shell echo $@: $(new_objs) >> $(OBJDIR)/$*.d))
	$(if $(new_objs), @$(MAKE) --no-print-directory $@)
	$(if $(new_objs), , @echo "Linking $(GREEN)$@$(NC)")
	$(if $(new_objs), , @mkdir -p $(dir $@))
	$(if $(new_objs), , @$(LD) $(LDFLAGS) $(EXTRA_CFLAGS) -o $@ $(OBJDIR)/$*.o $(objs) $(BINDIR)/$(lib) $(LDLIBS))

.PHONY: clean all tests test thorough docker perf loc gen

.PRECIOUS: $(OBJDIR)/%.o
.PRECIOUS: $(BINDIR_BASE)/%
.PRECIOUS: $(BINDIR)/%
