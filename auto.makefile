#make -f star_auto.makefile KERNEL=$(KERNEL) TARGET_ROOTFS_DIR=$(TARGET_ROOTFS_DIR) CROSS_COMPILE=${CROSS_COMPILE}
#
# Initial parameters (don't change)
#
PWD=$(shell pwd)
MYPATH=$(shell dirname ${PWD})
MYNAME=$(shell basename ${PWD})


# 
# Received Parameter (don't change)
#  

#TARGET_ROOTFS_DIR is the installation target directory,
#ROOTFS_DIR_FOR_LIB is the places to collect all cumulative rootfs, includes libs
#Usually TARGET_ROOTFS_DIR and ROOTFS_DIR_FOR_LIB are the same, but there are situations that they are not.
#
TARGET_ROOTFS_DIR?=$(shell pwd)/target

KERNEL?=$(shell pwd)/../../kernels/linux
CROSS_COMPILE?=arm-linux-

#LSDK_KERNELRELEASE=2.6.31-cavm1
#V: verbose mode 
ifeq ($V,1)
V_PREFIX=
V_POSTFIX=
V_POSTFIX_E=
V_PARAM=-v
else
V_PREFIX=@
V_POSTFIX= 1>/dev/null
V_POSTFIX_E= 1>/dev/null 2>&1
V_PARAM=
endif


#toolchain bin directory =    ${TOOLCHAIN_INSTALL_PATH}/${TOOLCHAIN_BIN}
#toolchain lib directory =    ${TOOLCHAIN_INSTALL_PATH}/${TOOLCHAIN_LIB}
#toolchain usr/lib directory =    ${TOOLCHAIN_INSTALL_PATH}/${TOOLCHAIN_USR_LIB}
#toolchain header directory = ${TOOLCHAIN_INSTALL_PATH}/${TOOLCHAIN_H}
info:
	@echo ==================
	@echo MYPATH=${MYPATH}
	@echo MYNAME=${MYNAME}
	@echo PWD=${PWD}
	@echo TARGET_ROOTFS_DIR=${TARGET_ROOTFS_DIR}
	@echo ROOTFS_DIR_FOR_LIB=${ROOTFS_DIR_FOR_LIB}
	@echo KERNEL=${KERNEL}
	@echo LSDK_KERNELRELEASE=${LSDK_KERNELRELEASE}
	@echo V=${V}
	@echo TOOLCHAIN=${TOOLCHAIN}
	@echo TOOLCHAIN_INSTALL_PATH=${TOOLCHAIN_INSTALL_PATH}
	@echo TOOLCHAIN_BIN=${TOOLCHAIN_BIN}
	@echo TOOLCHAIN_LIB=${TOOLCHAIN_LIB}
	@echo TOOLCHAIN_USR_LIB=${TOOLCHAIN_USR_LIB}
	@echo TOOLCHAIN_H=${TOOLCHAIN_H}
	@echo ==================

#
#
#
# Application specific application, change as needed
#
CC=${CROSS_COMPILE}gcc
AS=${CROSS_COMPILE}as
LD=${CROSS_COMPILE}ld
STRIP=${CROSS_COMPILE}strip
AR=${CROSS_COMPILE}ar
SIZE=${CROSS_COMPILE}size
RANLIB=${CROSS_COMPILE}ranlib
CXX=${CROSS_COMPILE}g++

COMPILE_ENV= CC=${CC} LD=${LD} STRIP=${STRIP} AS=${AS} AR=${AR} SIZE=${SIZE} CXX=${CXX} RANLIB=${RANLIB} CROSS_COMPILE=${CROSS_COMPILE}

#PARAM=DESTDIR=${TARGET_ROOTFS_DIR} ${COMPILE_ENV}
PARAM=${COMPILE_ENV}

#
# build: configure, then build
#
build: info
	@echo action=$@
	cp include/configs/cavium_cns3420vb2x_parallel.h include/configs/cavium_cns3000.h	
	make cavium_cns3000_config ${PARAM}
	make env ${PARAM}

clean: info
	@echo action=$@
	make ${PARAM} $@
	#if [ -e Makefile ] ; then make distclean ${PARAM}||exit 1; fi

install: info
	@echo action=$@
	mkdir ${TARGET_ROOTFS_DIR}/sbin -p
	cp tools/env/fw_printenv ${TARGET_ROOTFS_DIR}/sbin
	ln -s fw_printenv ${TARGET_ROOTFS_DIR}/sbin/fw_setenv
	
