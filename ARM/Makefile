%.PHONY: help
help::
	@echo  ""
	@echo  " Makefile Usage:"
	@echo  ""
	@echo  "  make sd_card"
	@echo  "  Command calls rules: kernels graph xclbin application package, in sequence"
	@echo  "  Default TARGET is hw"
	@echo  ""
	@echo  "  make gen_gemm"
	@echo  "  Command to generate matrices"
	@echo  ""
	@echo  "  make run"
	@echo  "  Command calls rules: sd_card, in sequence"
	@echo  "  Default TARGET is hw"
	@echo  ""
	@echo  "  make all"
	@echo  "  Command calls rules: sd_card, in sequence"
	@echo  "  Default TARGET is hw"
	@echo  ""
	@echo  "  make application"
	@echo  "  Command to generate the A72 application"
	@echo  ""
	@echo  "  make package"
	@echo  "  Command to package design into an sd_card for specified target"
	@echo  "  Default TARGET is hw"
	@echo  ""
	@echo  "  make clean_target"
	@echo  "  Command to remove the generated files for a specific target"
	@echo  ""
	@echo  "  make clean_gemmsize"
	@echo  "  Command to remove the generated files for a specific GeMM Matrix Dimension"
	@echo  ""
	@echo  "  make cleanall"
	@echo  "  Command to remove the generated files for all variations"
	@echo  ""
	@echo  " OPTIONS:"
	@echo  " Use the make recipes with required values for options mentioned below-"
	@echo  "    TARGET     -specifies target: Hardware or Emulation. Values: hw_emu(unsupported), hw(default)"
	@echo  "    GEMM_SIZE  -specifies GeMM Matrix Dimension. Values: 32(default), 64, 128, 256, 512, 1024"
	@echo  "    OPT        -compiler opt level. Values: 0,1,2,3,s (default 1)"
	@echo  ""

# Print all options passed to Makefile
print-%  : ; @echo $* = $($*)

# =========================================================
# TARGET can be set as:
#   hw_emu: Hardware Emulation
#   hw    : Hardware Run
# =========================================================
TARGET := hw

# =========================================================
# Gemm Output matrix size can be set as:
# 32(32x32x32)<default>, 64(64x64x64), 128(128x128x128),
# 256(256x256x256), 512(512x512x512) and 1024(1kx1kx1k)
# =========================================================
GEMM_SIZE := 32
MAT_DIMS  := $(GEMM_SIZE)x$(GEMM_SIZE)x$(GEMM_SIZE)

# =========================================================
# Compiler optimization level (0/1/2/3/s)
# Usage: make ... OPT=3
# Default matches existing tutorial behaviour: OPT=1 (i.e., -O)
# =========================================================
OPT := 1

# =========================================================
# Source directories
# =========================================================
# Relative gemm directory
RELATIVE_PROJECT_DIR := ./

# Absolute gemm directory = <user path>/Tutorials/AI_Engine/gemm
PROJECT_REPO := $(shell readlink -f $(RELATIVE_PROJECT_DIR))

## Setting other folders relative to PROJECT_REPO
DESIGN_REPO					:= $(PROJECT_REPO)/design
HOST_APP_SRC				:= $(DESIGN_REPO)/host_app_src
DATA_SRC					:= $(DESIGN_REPO)/data
EXEC_SCRIPTS_REPO			:= $(DESIGN_REPO)/exec_scripts

BASE_BLD_DIR     := $(PROJECT_REPO)/build
GEMM_BLD_DIR     := $(BASE_BLD_DIR)/gemm_$(MAT_DIMS)
BUILD_TARGET_DIR := $(GEMM_BLD_DIR)/$(TARGET)

EMBEDDED_PACKAGE_OUT := $(BUILD_TARGET_DIR)/package
EMBEDDED_EXEC_SCRIPT := run_script.sh

SHELL_XSA := /home/dan/Documents/Xilinx/srs-project/DSP/build_700/gemm_1024x1024x1024/hw/vck190_dsp_gemm.hw.xsa

# ==========================================================
# Below are the names for Application executable,
# kernel executables, and xclbin
# ==========================================================
APP_ELF := gemm_arm.elf

# =========================================================
# Application Source Files repository
# =========================================================
APP_SRC_CPP := $(HOST_APP_SRC)/main.cpp

# =========================================================
# Application Compiler and Linker Flags
# =========================================================
GCC_FLAGS := -O$(OPT)
GCC_FLAGS += -c
GCC_FLAGS += -std=c++17
GCC_FLAGS += -D__linux__
GCC_FLAGS += -DGEMM_SIZE=$(GEMM_SIZE)

GCC_INC_FLAGS += -I$(SDKTARGETSYSROOT)/usr/include
GCC_INC_FLAGS += -I$(DATA_SRC)/$(MAT_DIMS)

# =========================================================
# Packaging Flags
# ========================================================
PKG_FLAGS := -t $(TARGET)
PKG_FLAGS += --save-temps
PKG_FLAGS += --temp_dir $(BUILD_TARGET_DIR)/_x
PKG_FLAGS += -f $(PLATFORM)
PKG_FLAGS += --package.rootfs $(COMMON_IMAGE_VERSAL)/rootfs.ext4
PKG_FLAGS += --package.kernel_image $(COMMON_IMAGE_VERSAL)/Image
PKG_FLAGS += --package.boot_mode=sd
PKG_FLAGS += --package.out_dir $(EMBEDDED_PACKAGE_OUT)
PKG_FLAGS += --package.image_format=ext4
PKG_FLAGS += --package.sd_file $(BUILD_TARGET_DIR)/$(APP_ELF) $(SHELL_XSA)
PKG_FLAGS += --package.sd_file $(EXEC_SCRIPTS_REPO)/$(EMBEDDED_EXEC_SCRIPT)

# =========================================================
# Application Executable File Generation
# ========================================================
application:
	mkdir -p $(BUILD_TARGET_DIR)
	@rm -rf $(BUILD_TARGET_DIR)/gemm_top_app.o $(BUILD_TARGET_DIR)/$(APP_ELF)
	$(CXX) $(GCC_FLAGS) $(GCC_INC_FLAGS) $(APP_SRC_CPP) -o $(BUILD_TARGET_DIR)/gemm_top_app.o
	$(CXX) $(BUILD_TARGET_DIR)/gemm_top_app.o -o $(BUILD_TARGET_DIR)/$(APP_ELF)

# =========================================================
# Package Generation 
# ========================================================
package:
	rm -rf $(EMBEDDED_PACKAGE_OUT)
	cd $(BUILD_TARGET_DIR);	\
	v++ -p $(PKG_FLAGS)
	cp -rf $(BUILD_TARGET_DIR)/package $(BUILD_TARGET_DIR)/package_backup

###########################################################################

gen_gemm:
	python3 $(DATA_SRC)/gen_gemm.py

# =========================================================
# Primary Build Targets
# ==> sd_card
# ==> run
# ==> all
# ==> clean_target
# ==> clean_gemmsize
# ==> cleanall
# ========================================================

#Build the design without running the hardware emulation
sd_card: application package

#Build the design
run: sd_card

#Build the design
all: gen_gemm sd_card

#Clean build target output directory...
clean_target:
	@echo "Cleaning $(TARGET) Target Build Dir..."
	rm -rf $(BUILD_TARGET_DIR)

#Clean GeMM matrix dimension output directory...
clean_gemmsize:
	@echo "Cleaning Gemm Size Builds..."
	rm -rf $(GEMM_BLD_DIR)

#Clean all builds...
cleanall:
	rm -rf $(BASE_BLD_DIR)
	rm -rf vivado* .Xil *.log vitis* core.*
