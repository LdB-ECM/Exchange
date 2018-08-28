# desired directory setup
BUILD_DIR = obj
SOURCE_DIR = src
TARGET_DIR = target

# The names of all object files that must be generated. 
# Deduced from the files in source directory.
# We filter out main.c


# Add the all .c files in src directory WITHOUT main.c
SRC_FILES = $(filter-out $(SOURCE_DIR)/main.c, $(wildcard $(SOURCE_DIR)/*.c))
# To that add all the .s files in directory
SRC_FILES += $(wildcard $(SOURCE_DIR)/*.S)

# Create an empty entry
FILES_PROCESSED := $(patsubst %.S,$(BUILD_DIR)/%.o, $(patsubst %.c,$(BUILD_DIR)/%.o, $(notdir $(SRC_FILES))))


# Rule to is to make kernel.elf 
all: kernel.elf

#Pass in the build_dir
OBJDIR := $(BUILD_DIR)

# this is the target for the both foreach loops
target = ${OBJDIR}/$(patsubst %.S,%.o, $(patsubst %.c,%.o, $(notdir ${1}) ) )

# enumerates .c files for processing to .o files .. output we call obj.c
obj.c :=
define obj
  $(call target,${1}) : ${1} | 
  obj$(suffix ${1}) += $(call target,${1})
endef

# enumerates .S files for processing to .o files .. output we call obj.S
obj.S :=
define obj1
  $(call target,${1}) : ${1} | 
  obj1$(suffix ${1}) += $(call target,${1})
endef

define SOURCES
  $(foreach i,${1},$(eval $(call obj,${i} )))
  $(foreach i,${1},$(eval $(call obj1,${i})))
endef

$(eval $(call SOURCES,${SRC_FILES}))


#all those other c files will use this rule
 ${obj.c} : % :
	@echo   other c file rule,  $^ -o $@

#all asm files will use this rule
 ${obj.S} : % :
	@echo   asm file rule,  $^ -o $@

#  rule says to build kernel.elf we need all those evaluated c objects
kernel.elf : ${obj.S} ${obj.c} 
	@echo  main file rule running
	@echo  MAIN FILE: $(SOURCE_DIR)/main.c PROCESSED FILES: $(FILES_PROCESSED)

.DEFAULT_GOAL := kernel.elf

# Control silent mode  .... we want silent in clean
.silent:clean

# cleanup temp files
clean:
	@echo  the clean file rule

