# Note: Do not include spaces in the directory name
.DEFAULT_GOAL := static_lib

CC := g++ # compiler
abs_paths := $(shell pwd)
lib_name := $(notdir $(abs_paths))

# Compile static library
lib_srcs := $(shell find src -name '*.c')
lib_objs := $(patsubst src/%.c,objs/%.o,$(lib_srcs))

lib_include_paths := $(abs_paths)/interface
lib_I_flag := $(lib_include_paths:%=-I%)

lib_compile_optinons := -g -O3 -w $(lib_I_flag)

objs/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) -c $^ -o $@ $(lib_compile_optinons)

../lib$(lib_name).a: $(lib_objs)
	@mkdir -p $(dir $@)
	@ar -crv $@ $^

static_lib: ../lib$(lib_name).a
	@rm -rf objs

clean: 
	@rm -rf objs ../lib$(lib_name).a

debug:
	@echo $(lib_name)

.PHONY : static_lib