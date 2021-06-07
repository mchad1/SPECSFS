#  -*- mode: makefile; -*-
# Makefile for netmist.
#

PRODS := $(wildcard dist pro lite)
PROD_DIRS := $(foreach prod,$(PRODS),$(wildcard $(prod)))
PRODS := $(foreach prod_dir,$(PROD_DIRS),$(notdir $(prod_dir)))

ifndef PROD
PROD := $(notdir $(firstword $(PROD_DIRS)))
endif

.DEFAULT_GOAL: $(PROD)

.PHONY: all dist pro lite
all: $(PRODS)

$(PRODS):
	cd $@ && $(MAKE)

CLEAN_PRODS := $(addprefix clean-,$(PRODS))

$(CLEAN_PRODS):
	cd $(subst clean-,,$@) && $(MAKE) clean

.PHONY: clean $(CLEAN_PRODS)
clean: $(CLEAN_PRODS)
