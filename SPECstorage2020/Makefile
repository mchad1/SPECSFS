#  -*- mode: makefile; -*-
# Makefile for SPECstorage Solution 2020.
#
# As one adds new targets one needs to specify REMSH if the system
# uses remsh instead of rsh to perform remote shell commands.
# Also add any libraries that are needed.
#
#
# Figure out the name of the target platform Operating system.
#
# Variable refinement based on distribution type and os type.
# include makefile_prod.in

export TRUNK_DIR := $(PWD)

include makefile.in

PRODS := dist pro lite
PROD_DIRS := $(foreach prod,$(PRODS),$(wildcard netmist/$(prod)))
PRODS := $(foreach prod_dir,$(PROD_DIRS),$(notdir $(prod_dir)))
ifndef PROD
PROD := $(notdir $(firstword $(PROD_DIRS)))
endif

.DEFAULT_GOAL := prod

.PHONY: all prod check-os netmist-prod netmist-all install
prod: yaml netmist-prod install

all: yaml netmist-all install

yaml:	redistributable_sources/libyaml-master/src/.libs/libyaml.a

redistributable_sources/libyaml-master/src/.libs/libyaml.a:
	./bootstrap_yaml

netmist-prod:
	cd netmist && ./version.sh
	cd netmist && $(MAKE) $(PROD)

netmist-all:
	cd netmist && ./version.sh
	cd netmist && $(MAKE) all

CLEAN_DEPS := $(CLEAN_DEPS) 

BUILD_PROD := build/$(PROD)
BUILD_COMMON := build/common
NETMIST_BINS := netmist netmist_monitor netmist_modify pump_carbon pump_csv
PROD_BIN_INSTALL := $(addprefix bin/,$(NETMIST_BINS))
SCRIPTS := $(wildcard bin.in/common/*) $(wildcard bin.in/*$(PROD)*/*)

.PHONY: install $(PROD_BIN_INSTALL) 
install: netmist-prod create-bin $(PROD_BIN_INSTALL) 
	cp $(SCRIPTS) bin

create-bin:
	mkdir -p bin

$(PROD_BIN_INSTALL): bin/%: $(BUILD_PROD)/$(OS)/$(ARCH)/%
	cp $< $@

clean-netmist:
	rm -rf redistributable_sources/libyaml-master
	cd netmist && $(MAKE) clean

CLEAN_DEPS := $(CLEAN_DEPS) clean-netmist

.PHONY: clean
clean: $(CLEAN_DEPS)
	rm -rf build bin
	cp bin.in/dist_pro/SM2020 .
	cp bin.in/dist_pro/storage2020.yml .
	cp bin.in/dist_pro/sfs_rc .
	cp bin.in/dist_pro/SpecReport .
	cp bin.in/dist_pro/storage2020result.css .

MAKE_RELEASE_IN := $(wildcard make-release.in)
ifneq ($(MAKE_RELEASE_IN),)
include make-release.in
endif
