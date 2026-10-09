#!/usr/bin/make -f
NAME = TrapLimit
FILES_DSP = TrapLimitPlugin.cpp
FILES_UI  = TrapWinBetonUI.cpp
DPF_PATH = ../dpf
DPF_TARGET_DIR = ../bin
DPF_BUILD_DIR = ../build/$(NAME)
include $(DPF_PATH)/Makefile.plugins.mk
TARGETS += vst3 clap
ifeq ($(MACOS),true)
TARGETS += au
endif
all: $(TARGETS)
