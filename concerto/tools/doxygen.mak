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

DOXYGEN := doxygen
DOXYFILE ?= Doxyfile

PDFNAME ?= refman.pdf

$(_MODULE)_DOXYFILE := $($(_MODULE)_SDIR)/$(DOXYFILE)
$(_MODULE)_DOXYFILE_MOD := $(TARGET_DOC)/$(_MODULE)/$(DOXYFILE)
$(_MODULE)_BIN := $($(_MODULE)_TDIR)/$($(_MODULE)_TARGET).tar.gz

$(_MODULE)_HTML := $(TARGET_DOC)/$(_MODULE)/html/index.html
$(_MODULE)_TEX  := $(TARGET_DOC)/$(_MODULE)/latex/refman.tex
$(_MODULE)_PDFNAME := $(PDFNAME)
$(_MODULE)_PDF := $(TDIR)/$($(_MODULE)_PDFNAME)

#$(info Modified Doxyfile should be in $($(_MODULE)_DOXYFILE_MOD))

define $(_MODULE)_DOCUMENTS

$(TARGET_DOC)/$(_MODULE)/.gitignore:
	$(Q)$(MKDIR) $(TARGET_DOC)/$(_MODULE)

$($(_MODULE)_DOXYFILE_MOD): $($(_MODULE)_DOXYFILE) $(TARGET_DOC)/$(_MODULE)/.gitignore
	$(Q)$(COPY) $($(_MODULE)_DOXYFILE) $(TARGET_DOC)/$(_MODULE)
	$(Q)$(PRINT) OUTPUT_DIRECTORY=$(TARGET_DOC)/$(_MODULE) >> $($(_MODULE)_DOXYFILE_MOD)
	$(Q)$(PRINT) WARN_LOGFILE=$(TARGET_DOC)/$(_MODULE)/warnings.txt >> $($(_MODULE)_DOXYFILE_MOD)
	$(Q)$(PRINT) GENERATE_TAGFILE=$(TARGET_DOC)/$(_MODULE)/$(_MODULE).tags >> $($(_MODULE)_DOXYFILE_MOD)  
ifneq ($(SCM_VERSION),)
	-$(Q)$(PRINT) PROJECT_NUMBER=$(SCM_VERSION) >> $($(_MODULE)_DOXYFILE_MOD)
endif

$($(_MODULE)_HTML): $($(_MODULE)_DOXYFILE_MOD)
	$(Q)$(CLEANDIR) $(TARGET_DOC)/$(_MODULE)/html
	$(Q)$(DOXYGEN) $($(_MODULE)_DOXYFILE_MOD)

$($(_MODULE)_TEX): $($(_MODULE)_DOXYFILE_MOD)
	$(Q)$(CLEANDIR) $(TARGET_DOC)/$(_MODULE)/latex
	$(Q)$(DOXYGEN) $($(_MODULE)_DOXYFILE_MOD)

$($(_MODULE)_PDF): $($(_MODULE)_TEX)
	-$(Q)cp docs/images/* $(TARGET_DOC)/$(_MODULE)/latex/; \
	 cd $(TARGET_DOC)/$(_MODULE)/latex/; \
	 pdflatex refman; \
	 bibtex refman; \
	 makeindex refman.idx; \
	 pdflatex refman; 
	-$(Q)cd $(TARGET_DOC)/$(_MODULE)/latex; cp refman.pdf $($(_MODULE)_PDFNAME); cp refman.pdf $($(_MODULE)_PDF)

$(_MODULE)_docs:: $($(_MODULE)_HTML) $($(_MODULE)_PDF) $($(_MODULE)_ODIR)/.gitignore
	$(Q)tar zcf $($(_MODULE)_BIN) $(TARGET_DOC)/$(_MODULE)/html

$($(_MODULE)_BIN):: $(_MODULE)_docs

docs:: $(_MODULE)_docs
	$(Q)echo Building docs for $(_MODULE)

clean_docs::
	$(Q)$(CLEANDIR) docs/$(_MODULE)/latex
	$(Q)$(CLEANDIR) docs/$(_MODULE)/html

endef


