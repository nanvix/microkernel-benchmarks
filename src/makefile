#
# MIT License
#
# Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

#===============================================================================
# Toolchain Configuration 
#===============================================================================

# Libraries
export LIBS  = -Wl,--whole-archive $(LIBNANVIX) $(LIBKERNEL) $(LIBHAL)
export LIBS += -Wl,--no-whole-archive $(THEIR_LIBS)

#===============================================================================
# Common Sources
#===============================================================================

# C Source Files
export SRC += $(wildcard $(CURDIR)/utils/*.c)

#===============================================================================
# Generic Rules
#===============================================================================

# Builds everything.
all: all-buffer all-fork-join all-gf all-kcall-local all-kcall-remote all-mm \
all-perf all-stream all-tsp all-upcall

# Cleans everything.
clean: clean-buffer clean-fork-join clean-gf clean-kcall-local \
clean-kcall-remote clean-mm clean-perf clean-stream clean-tsp clean-upcall

# Builds all images:
images: image-buffer image-fork-join image-gf image-kcall-local \
image-kcall-remote image-mm image-perf image-stream image-tsp image-upcall

#===============================================================================
# buffer
#===============================================================================

# Builds buffer.
all-buffer:
	@$(MAKE) -C buffer all

# Cleans object files.
clean-buffer:
	@$(MAKE) -C buffer clean

# Build Buffer Benchmark Binary Image
image-buffer:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/buffer.img $(BINDIR) buffer

#===============================================================================
# fork-join
#===============================================================================

# Builds fork-join.
all-fork-join:
	@$(MAKE) -C fork-join all

# Cleans object files.
clean-fork-join:
	@$(MAKE) -C fork-join clean

# Build Buffer Benchmark Binary Image
image-fork-join:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/fork-join.img $(BINDIR) fork-join

#===============================================================================
# gf
#===============================================================================

# Builds gf.
all-gf:
	@$(MAKE) -C gf all

# Cleans object files.
clean-gf:
	@$(MAKE) -C gf clean

# Build Buffer Benchmark Binary Image
image-gf:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/gf.img $(BINDIR) gf

#===============================================================================
# kcall-local
#===============================================================================

# Builds kcall-local.
all-kcall-local:
	@$(MAKE) -C kcall-local all

# Cleans object files.
clean-kcall-local:
	@$(MAKE) -C kcall-local clean

# Build Buffer Benchmark Binary Image
image-kcall-local:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/kcall-local.img $(BINDIR) kcall-local

#===============================================================================
# kcall-remote
#===============================================================================

# Builds kcall-remote.
all-kcall-remote:
	@$(MAKE) -C kcall-remote all

# Cleans object files.
clean-kcall-remote:
	@$(MAKE) -C kcall-remote clean

# Build Buffer Benchmark Binary Image
image-kcall-remote:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/kcall-remote.img $(BINDIR) kcall-remote

#===============================================================================
# mm
#===============================================================================

# Builds mm.
all-mm:
	@$(MAKE) -C mm all

# Cleans object files.
clean-mm:
	@$(MAKE) -C mm clean

# Build Buffer Benchmark Binary Image
image-mm:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/mm.img $(BINDIR) mm

#===============================================================================
# perf
#===============================================================================

# Builds perf.
all-perf:
	@$(MAKE) -C perf all

# Cleans object files.
clean-perf:
	@$(MAKE) -C perf clean

# Build Buffer Benchmark Binary Image
image-perf:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/perf.img $(BINDIR) perf

#===============================================================================
# stream
#===============================================================================

# Builds stream.
all-stream:
	@$(MAKE) -C stream all

# Cleans object files.
clean-stream:
	@$(MAKE) -C stream clean

# Build Buffer Benchmark Binary Image
image-stream:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/stream.img $(BINDIR) stream

#===============================================================================
# tsp
#===============================================================================

# Builds tsp.
all-tsp:
	@$(MAKE) -C tsp all

# Cleans object files.
clean-tsp:
	@$(MAKE) -C tsp clean

# Build Buffer Benchmark Binary Image
image-tsp:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/tsp.img $(BINDIR) tsp

#===============================================================================
# upcall
#===============================================================================

# Builds upcall.
all-upcall:
	@$(MAKE) -C upcall all

# Cleans object files.
clean-upcall:
	@$(MAKE) -C upcall clean

# Build Buffer Benchmark Binary Image
image-upcall:
	bash $(UTILSDIR)/nanvix-build-image.sh $(IMGDIR)/upcall.img $(BINDIR) upcall
