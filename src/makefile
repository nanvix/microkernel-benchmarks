#
# MIT License
#
# Copyright(c) 2011-2019 The Maintainers of Nanvix
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
# Generic Rules
#===============================================================================

# Builds Binary Files
all: all-buffer all-fork-join all-kcall-local all-kcall-remote all-noise \
		all-perf all-server all-stream

# Cleans Object Files
clean: clean-buffer clean-fork-join clean-kcall-local clean-kcall-remote \
		clean-noise clean-perf clean-server clean-stream

# Cleans Everything
distclean: distclean-buffer distclean-fork-join distclean-kcall-local \
		distclean-kcall-remote distclean-noise distclean-perf \
		distclean-server distclean-stream

# Builds multibinary images
image: image-buffer image-fork-join image-kcall-local image-kcall-remote \
		image-noise image-perf image-server image-stream

#===============================================================================
# buffer
#===============================================================================

# Builds buffer.
all-buffer:
	@$(MAKE) -C buffer all

# Cleans object files.
clean-buffer:
	@$(MAKE) -C buffer clean

# Cleans object files.
distclean-buffer:
	@$(MAKE) -C buffer distclean

# Builds multibinary image.
image-buffer:
	@$(MAKE) -C buffer image

#===============================================================================
# fork-join
#===============================================================================

# Builds fork-join.
all-fork-join:
	@$(MAKE) -C fork-join all

# Cleans object files.
clean-fork-join:
	@$(MAKE) -C fork-join clean

# Cleans object files.
distclean-fork-join:
	@$(MAKE) -C fork-join distclean

# Builds multibinary image.
image-fork-join:
	@$(MAKE) -C fork-join image

#===============================================================================
# kcall-local
#===============================================================================

# Builds kcall-local.
all-kcall-local:
	@$(MAKE) -C kcall-local all

# Cleans object files.
clean-kcall-local:
	@$(MAKE) -C kcall-local clean

# Cleans object files.
distclean-kcall-local:
	@$(MAKE) -C kcall-local distclean

# Builds multibinary image.
image-kcall-local:
	@$(MAKE) -C kcall-local image

#===============================================================================
# kcall-remote
#===============================================================================

# Builds kcall-remote.
all-kcall-remote:
	@$(MAKE) -C kcall-remote all

# Cleans object files.
clean-kcall-remote:
	@$(MAKE) -C kcall-remote clean

# Cleans object files.
distclean-kcall-remote:
	@$(MAKE) -C kcall-remote distclean

# Builds multibinary image.
image-kcall-remote:
	@$(MAKE) -C kcall-remote image

#===============================================================================
# noise
#===============================================================================

# Builds noise.
all-noise:
	@$(MAKE) -C noise all

# Cleans object files.
clean-noise:
	@$(MAKE) -C noise clean

# Cleans object files.
distclean-noise:
	@$(MAKE) -C noise distclean

# Builds multibinary image.
image-noise:
	@$(MAKE) -C noise image

#===============================================================================
# perf
#===============================================================================

# Builds perf.
all-perf:
	@$(MAKE) -C perf all

# Cleans object files.
clean-perf:
	@$(MAKE) -C perf clean

# Cleans object files.
distclean-perf:
	@$(MAKE) -C perf distclean

# Builds multibinary image.
image-perf:
	@$(MAKE) -C perf image

#===============================================================================
# server
#===============================================================================

# Builds server.
all-server:
	@$(MAKE) -C server all

# Cleans object files.
clean-server:
	@$(MAKE) -C server clean

# Cleans object files.
distclean-server:
	@$(MAKE) -C server distclean

# Builds multibinary image.
image-server:
	@$(MAKE) -C server image

#===============================================================================
# stream
#===============================================================================

# Builds stream.
all-stream:
	@$(MAKE) -C stream all

# Cleans object files.
clean-stream:
	@$(MAKE) -C stream clean

# Cleans object files.
distclean-stream:
	@$(MAKE) -C stream distclean

# Builds multibinary image.
image-stream:
	@$(MAKE) -C stream image

#===============================================================================
# upcall
#===============================================================================

# Builds upcall.
all-upcall:
	@$(MAKE) -C upcall all

# Cleans object files.
clean-upcall:
	@$(MAKE) -C upcall clean

# Cleans object files.
distclean-upcall:
	@$(MAKE) -C upcall distclean

# Builds multibinary image.
image-upcall:
	@$(MAKE) -C upcall image
