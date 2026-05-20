#
# This Makefile was automatically generated; do not edit.
#

###########################################################################
# Makefile for KUNLUN Linux GPU driver kernel modules
###########################################################################

  .PHONY: modules module clean clean_conftest modules_install
  modules clean modules_install:
	@$(MAKE) -C kunlun_module $@
	@if [ "$@" = "modules" ]; then \
		cp  -rf kunlun_module/*.ko .; \
	fi
  # Compatibility target for scripts that may be directly calling the
  # "module" target from the old build system.

  module: modules

