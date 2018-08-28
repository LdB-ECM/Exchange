# desired directory setup
BUILD_DIR = obj
SOURCE_DIR = src
TARGET_DIR = target

# The names of all object files that must be generated. 
# Deduced from the files in source directory.
C_FILES :=   $(wildcard $(SOURCE_DIR)/*.c)

# Rule to is to make kernel.elf 
all: kernel.elf

#this emuerates the c file list changing .c to .o and stripping filepath and adding in build_dir
OBJDIR := $(BUILD_DIR)
target = ${OBJDIR}/$(patsubst %.c,%.o,$(notdir ${1}))
obj.c :=
define obj
  $(call target,${1}) : ${1} | 
  obj$(suffix ${1}) += $(call target,${1})
endef

define SOURCES
  $(foreach i,${1},$(eval $(call obj,${i})))
endef

$(eval $(call SOURCES,${C_FILES}))


#all those other c files will use this rule
 ${obj.c} : % :
	@echo   other c file rule,  $^ -o $@


#  rule says to build kernel.elf we need all those evaluated c objects
kernel.elf : ${obj.c} 
	@echo  main file rule
	@echo	$(C_FILES)

# Control silent mode  .... we want silent in clean
.silent:clean

# cleanup temp files
clean:
	@echo  the clean file rule

