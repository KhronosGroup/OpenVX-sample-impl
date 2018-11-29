# 

# Copyright (c) 2012-2017 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


# @author Erik Rainey
# @url http://github.com/emrainey/Concerto

# Tools
LATEX := latex
PDFLATEX := pdflatex
MSCGEN := mscgen
DOT := dot
EPSTOPDF := epstopdf
BIBTEX := bibtex

PDFNAME ?= $(TARGET).pdf

BIB_OBJS := $(addprefix $(ODIR)/,$(BIBFILES))
DOT_OBJS := $(DOTFILES:%.dot=$(ODIR)/%.pdf)
MSC_OBJS := $(MSCFILES:%.msc=$(ODIR)/%.pdf)
TEX_OBJS := $(TEXFILES:%.tex=$(ODIR)/%.pdf)
STY_OBJS := $(addprefix $(ODIR)/,$(STYFILES))
BST_OBJS := $(addprefix $(ODIR)/,$(BSTFILES))
IMG_OBJS := $(addprefix $(ODIR)/,$(IMGFILES))

$(_MODULE)_SUPPORT := $(BIBFILES) $(BSTFILES) $(STYFILES)
$(_MODULE)_SUPPORT_SRCS := $(foreach sup,$(SUPPORT),$(SDIR)/$(sup))
$(_MODULE)_SUPPORT_OBJS := $(foreach sup,$(SUPPORT),$(ODIR)/$(sup))

$(_MODULE)_BIN := $(TDIR)/$(PDFNAME)
$(_MODULE)_SRCS := $(DOTFILES) $(MSCFILES) $(TEXFILES) $(BIBFILES) $(BSTFILES) $(STYFILES)
$(_MODULE)_OBJS := $(DOT_OBJS) $(MSC_OBJS) $(BIB_OBJS) $(TEX_OBJS) $(STY_OBJS) $(BST_OBJS) $(IMG_OBJS)
# in case the name gets duplicated 
$(_MODULE)_OBJS := $(filter-out $(ODIR)/$(PDFNAME),$($(_MODULE)_OBJS))

ifeq ($(SHOW_COMMANDS),1)
DFLAGS := --halt-on-error
endif

ifeq ($(SHOW_MAKEDEBUG),1)
$(info Latex SUPPORT_OBJS=$(SUPPORT_OBJS))
endif

define $(_MODULE)_COMPILE_TOOLS

$($(_MODULE)_BIN): $(ODIR)/$(PDFNAME) $(TDIR)/.gitignore
	@echo [COPY] $$(notdir $$<)
	$(Q)$(COPY) $$< $$@

$(ODIR)/%.png: $(SDIR)/%.png $(ODIR)/.gitignore
	@echo [COPY] $$(notdir $$<)
	$(Q)$(COPY) $$< $$@	

$(ODIR)/%.pdf: $(SDIR)/%.dot $(ODIR)/.gitignore
	@echo [DOT] $$(notdir $$<)
ifeq ($(USE_SVG),true)	
	$(Q)$(DOT) -T svg -o $(ODIR)/$$(notdir $$(basename $$<)).svg $$<
	$(Q)xsltproc -o $(ODIR)/$$(notdir $$(basename $$<))_new.svg $(CONCERTO_ROOT)/tools/notuglydot.xsl $(ODIR)/$$(notdir $$(basename $$<)).svg 
	$(Q)inkscape -z -D --file $(ODIR)/$$(notdir $$(basename $$<))_new.svg --export-pdf=$$@
else	
	$(Q)$(DOT) -T pdf -o $$@ $$<
endif

$(ODIR)/%.eps: $(SDIR)/%.msc $(ODIR)/.gitignore
	@echo [MSC] $$(notdir $$<)
	$(Q)$(MSCGEN) -T eps -i $$< -o $$(basename $$@).eps

$(ODIR)/%.pdf: $(ODIR)/%.eps $(ODIR)/.gitignore
	@echo [PDF] Convert $$(notdir $$<)
	$(Q)$(EPSTOPDF) $$< --outfile=$$@

$(ODIR)/%.tex: $(SDIR)/%.tex $(ODIR)/.gitignore
	@echo [COPY] $$(notdir $$<)
	$(Q)$(COPY) $$< $$@

$(ODIR)/%.sty: $(SDIR)/%.sty $(ODIR)/.gitignore
	@echo [COPY] $$(notdir $$<)
	$(Q)$(COPY) $$< $$@

$(ODIR)/%.bst: $(SDIR)/%.bst $(ODIR)/.gitignore
	@echo [COPY] $$(notdir $$<)
	$(Q)$(COPY) $$< $$@

$(ODIR)/%.bib: $(SDIR)/%.bib $(ODIR)/.gitignore $(ODIR)/.gitignore
	@echo [COPY] $$(notdir $$<)
	$(Q)$(COPY) $$< $$@

$(ODIR)/%.pdf: $(ODIR)/%.tex $($(_MODULE)_OBJS) $(ODIR)/.gitignore
	@echo [TEX] $$(notdir $$<)
	$(Q)cd $(ODIR);$(PDFLATEX) $(DFLAGS) -shell-escape -output-format=pdf --output-directory=$(ODIR) $$<
	@echo [BIB] Fixing bibliography
	$(Q)cd $(ODIR);$(BIBTEX) $$(basename $$(notdir $$<))
	@echo [TEX] $$(notdir $$<) Post-bib-fixup
	$(Q)cd $(ODIR);$(PDFLATEX) $(DFLAGS) -shell-escape -output-format=pdf --output-directory=$(ODIR) $$<
	$(Q)cd $(ODIR);$(PDFLATEX) $(DFLAGS) -shell-escape -output-format=pdf --output-directory=$(ODIR) $$<

$(ODIR)/$(PDFNAME): $(TEX_OBJS)
	$(Q)$(COPY) $$< $$@

latex:: $($(_MODULE)_BIN)

clean_latex::
	$(Q)rm $($(_MODULE)_OBJS) $($(_MODULE)_BIN)
 
endef
