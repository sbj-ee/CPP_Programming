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

# "|| exit 1" makes a failure in ANY directory fail the whole make (a plain
# shell for-loop only returns the status of its last iteration).
all: $(BINS)
	@for d in $(_MANAGED); do $(MAKE) -C $$d all || exit 1; done

# Pattern rule: compile any standalone .cpp to a binary alongside it
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

valgrind: all
	@for bin in $(BINS); do \
		echo "--- $$bin ---"; \
		$(VALGRIND) $$bin || exit 1; \
	done
	@for d in $(_MANAGED); do $(MAKE) -C $$d valgrind || exit 1; done

clean:
	@find exercises topics -type f ! -name '*.cpp' ! -name '*.hpp' ! -name '*.h' ! -name '*.md' ! -name 'Makefile' -delete
	@for d in $(_MANAGED); do $(MAKE) -C $$d clean || exit 1; done
