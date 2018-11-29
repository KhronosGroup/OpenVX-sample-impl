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

ifeq ($($(_MODULE)_TYPE),jar)

ifeq ($(SHOW_MAKEDEBUG),1)
$(info Building JAR file $(_MODULE))
endif

JAR := jar
JC  := javac

EMPTY:=
SPACE:=$(EMPTY) $(EMPTY)

###############################################################################

$(_MODULE)_BIN       := $($(_MODULE)_TDIR)/$($(_MODULE)_TARGET).jar
$(_MODULE)_CLASSES   := $(patsubst %.java,%.class,$(JSOURCES))
$(_MODULE)_OBJS      := $(foreach cls,$($(_MODULE)_CLASSES),$($(_MODULE)_ODIR)/$(cls))
ifdef CLASSPATH
$(_MODULE)_CLASSPATH := $(CLASSPATH) $($(_MODULE)_ODIR)
CLASSPATH            :=
else
$(_MODULE)_CLASSPATH := $($(_MODULE)_ODIR)
endif
ifneq ($($(_MODULE)_JAVA_LIBS),)
$(_MODULE)_JAVA_DEPS := $(foreach lib,$($(_MODULE)_JAVA_LIBS),$($(_MODULE)_TDIR)/$(lib).jar)
$(_MODULE)_CLASSPATH += $($(_MODULE)_JAVA_DEPS)
endif

ifeq ($(SHOW_MAKEDEBUG),1)
$(info CLASSPATH=$($(_MODULE)_CLASSPATH))
endif
$(_MODULE)_CLASSPATH := $(subst $(SPACE),:,$($(_MODULE)_CLASSPATH))
JC_OPTS              := -deprecation -classpath $($(_MODULE)_CLASSPATH) -sourcepath $($(_MODULE)_SDIR) -d $($(_MODULE)_ODIR)
ifeq ($(TARGET_BUILD),debug)
JC_OPTS              += -g -verbose
else ifneq ($(filter $(TARGET_BUILD),release production),)
# perform obfuscation?
endif

ifdef MANIFEST
$(_MODULE)_MANIFEST  := -m $(MANIFEST)
MANIFEST             :=
else
$(_MODULE)_MANIFEST  :=
endif
ifdef ENTRY
$(_MODULE)_ENTRY     := $(ENTRY)
ENTRY                :=
else
$(_MODULE)_ENTRY     := Main
endif
JAR_OPTS             := cvfe $($(_MODULE)_BIN) $($(_MODULE)_MANIFEST) $($(_MODULE)_ENTRY) $(foreach cls,$($(_MODULE)_CLASSES),-C $($(_MODULE)_ODIR) $(cls))

###############################################################################


define $(_MODULE)_DEPEND_CLS
$($(_MODULE)_ODIR)/$(1).class: $($(_MODULE)_SDIR)/$(1).java $($(_MODULE)_SDIR)/$(SUBMAKEFILE) $($(_MODULE)_JAVA_DEPS) $($(_MODULE)_ODIR)/.gitignore
	@echo Compiling Java $(1).java
	$(Q)$(JC) $(JC_OPTS) $($(_MODULE)_SDIR)/$(1).java
endef

define $(_MODULE)_DEPEND_JAR
uninstall::
build:: $($(_MODULE)_BIN)
	@echo Building for $($(_MODULE)_BIN)
install:: $($(_MODULE)_BIN)

$($(_MODULE)_BIN): $($(_MODULE)_OBJS) $($(_MODULE)_SDIR)/$(SUBMAKEFILE)
	@echo Jar-ing Classes $($(_MODULE)_CLASSES)
	$(Q)$(JAR) $(JAR_OPTS) $(foreach cls,$($(_MODULE)_CLASSES),-C $($(_MODULE)_ODIR) $(cls))
endef

endif

