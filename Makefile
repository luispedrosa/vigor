# Skeleton Makefile for Vigor NFs
# This file dispatches the build process to the other Makefiles
#
# Variables that should be defined by inheriting Makefiles:
# - NF_AUTOGEN_SRCS := <NF files that are inputs to auto-generation>
# - NF_FILES := <NF files for both runtime and verif-time, automatically includes state and autogenerated files, and shared NF files>
# - NF_LAYER := <network stack layer at which the NF operates, default 2>
# - NF_BENCH_NEEDS_REVERSE_TRAFFIC := <whether the NF needs reverse traffic for meaningful benchmarks, default false>
# Variables that can be passed when running:
# - NF_DPDK_ARGS - will be passed as DPDK part of the arguments
# See Makefile for the rest of the variables

SELF_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
NF_DIR := $(shell if [ '$(notdir $(shell pwd))' = 'build' ]; then echo '..'; else echo '.'; fi)

# Check that the NF doesn't use clock_gettime directly, which would violate Vigor's expectations
ifeq (true,$(shell if grep -q clock_gettime $(addprefix $(NF_DIR)/,$(NF_FILES)); then echo 'true'; fi))
$(error Please use the Vigor nf_time header instead of clock_gettime)
endif

# Default values for arguments
NF_AUTOGEN_SRCS += $(SELF_DIR)/libvig/stubs/ether_addr.h
NF_LAYER ?= 2
NF_BENCH_NEEDS_REVERSE_TRAFFIC ?= false

NF_FILES += $(subst .h,.h.gen.c,$(NF_AUTOGEN_SRCS))
# Add state.c to the NF files, but only if it will be generated (so that we can compile stateless NFs)
ifneq (,$(wildcard $(NF_DIR)/dataspec.ml))
NF_FILES += state.c
endif

# Define this for the dpdk and dsos makefiles
NF_ARGS := --no-shconf $(NF_DPDK_ARGS) -- $(NF_ARGS)

ifeq (click,$(findstring click,$(shell pwd)))
# Click baselines
include $(SELF_DIR)/Makefile.click
else ifeq (,$(findstring dsos-,$(MAKECMDGOALS)))
# DPDK-based NFs
include $(SELF_DIR)/Makefile.dpdk
else
# DSOS-related targets
include $(SELF_DIR)/Makefile.dsos
endif



# =======
# Autogen
# =======

# note that DPDK's weird makefiles call this twice, once in the proper dir and once in build/, we only care about the former
autogen:
	@if [ '$(NF_DIR)' == '.' ]; then \
	  $(SELF_DIR)/codegen/generate.sh $(NF_AUTOGEN_SRCS); \
	  if [ -e 'dataspec.ml' ]; then $(SELF_DIR)/codegen/gen-loop-boilerplate.sh dataspec.ml; fi \
	fi



# ============
# Benchmarking
# ============

benchmark-%:
	@cd "$(SELF_DIR)/bench"; ./bench.sh "$(shell pwd)" $(subst benchmark-,,$@) || true
	@mv ../bench/$@.results . || true
	@printf '\n\nDone! Results are in $@.results, log file in $@.log\n\n'
# bench scripts use these to autodetect the NF type
_print-layer:
	@echo $(NF_LAYER)
_print-needsreverse:
	@echo $(NF_BENCH_NEEDS_REVERSE_TRAFFIC)
