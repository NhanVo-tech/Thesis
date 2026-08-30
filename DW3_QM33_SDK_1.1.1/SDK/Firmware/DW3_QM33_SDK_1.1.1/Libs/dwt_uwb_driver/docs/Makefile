#
# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
TAGS          ?= LLD FIRA PCTT NFCC_COEX RADAR
SPHINXOPTS    = -W --keep-going $(TAGS:%=-t %)
SPHINXBUILD   = sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build
srctree       = ..
export srctree

SPHINXBUILD_TARGETS = \
	 html dirhtml singlehtml pickle json htmlhelp qthelp devhelp \
	 epub latex latexpdf latexpdfja text man texinfo info gettext \
	 changes xml pseudoxml linkcheck doctest coverage

# User-friendly check for sphinx-build
HAVE_SPHINX := $(shell if which $(SPHINXBUILD) >/dev/null 2>&1; then echo 1; else echo 0; fi)

ifeq ($(HAVE_SPHINX),0)

.DEFAULT_GOAL = without-sphinx

without-sphinx:help-install
	@false

help:help-install

# Those pseudo-targets implemented by $(SPHINX_BUILD) won't work
# either, so let's deviate them to help-install too:
$(SPHINXBUILD_TARGETS) clean: help-install
	@false

else

.DEFAULT_GOAL = help

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@printf '  %-11s %s\n' 'help-install' 'to learn about our documentation prerequisites'

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
$(SPHINXBUILD_TARGETS):
	mkdir -p $(BUILDDIR)
	# regenerate the source docs using Doxygen
	doxygen
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)


clean:
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)


_build/latex/%.pdf: latex
	@$(MAKE) -C _build/latex $*.pdf

endif # HAVE_SPHINX


help-install:
	@echo "The '$(SPHINXBUILD)' command was not found. Make sure you have"
	@echo "Sphinx installed and in PATH, or set the SPHINXBUILD make"
	@echo "variable to point to the full path of the '$(SPHINXBUILD)'"
	@echo "executable."
	@echo "If something is wrong on your setup and you are not using the"
	@echo "documentation-builder to build the documentation, please have"
	@echo "a look to its Dockerfile to https://gitlab.com/qorvo/tech/uwb/uwb-stack/docker-images/documentation-builder"
	@echo
	@echo "You can install it in a venv like this:"
	@echo
	@echo "    sudo apt-get install -y python3-venv python3-pip texlive-font-utils texlive-latex-extra librsvg2-bin \\"
	@echo "                            plantuml python3-sphinxcontrib.apidoc linuxdoc-tools wget \\"
	@echo "                            libnotify4 libsecret-1-0 libsecret-common xvfb inkscape git"
	@echo "    wget -O /tmp/drawio.deb https://github.com/jgraph/drawio-desktop/releases/download/v19.0.3/drawio-amd64-19.0.3.deb"
	@echo "    dpkg -i /tmp/drawio.deb && rm -rf /tmp/drawio.deb"
	@echo "    python3 -m venv sphinx-venv"
	@echo "    source sphinx-venv/bin/activate"
	@echo "    pip install -r sphinx/requirements.txt"
	@echo

.PHONY: help help-install without-sphinx $(SPHINXBUILD_TARGETS)
