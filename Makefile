CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g

VALGRIND = valgrind --leak-check=full --error-exitcode=1

# Directories that manage their own build (have a local Makefile)
_MANAGED     := $(shell find exercises topics -mindepth 2 -maxdepth 2 -name 'Makefile' \
                         -exec dirname {} \; 2>/dev/null)
_EXCL        := $(foreach d,$(_MANAGED),-not -path '$(d)/*.cpp')

# Single-file exercises: one .cpp per directory, built by this Makefile
SRCS := $(shell find exercises topics $(_EXCL) -name '*.cpp')
BINS := $(SRCS:.cpp=)

.PHONY: all clean valgrind

all: $(BINS)
	@for d in $(_MANAGED); do $(MAKE) -C $$d all; done

# Pattern rule: compile any standalone .cpp to a binary alongside it
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

valgrind: all
	@for bin in $(BINS); do \
		echo "--- $$bin ---"; \
		$(VALGRIND) $$bin 2>&1 | grep -E "ERROR SUMMARY|no leaks"; \
	done
	@for d in $(_MANAGED); do $(MAKE) -C $$d valgrind; done

clean:
	@find exercises topics -type f ! -name '*.cpp' ! -name '*.hpp' ! -name '*.h' ! -name '*.md' ! -name 'Makefile' -delete
	@for d in $(_MANAGED); do $(MAKE) -C $$d clean; done
