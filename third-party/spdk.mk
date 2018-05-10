SPDK_ROOT_DIR := $(dir $(lastword $(MAKEFILE_LIST)))spdk
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.app.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

CFLAGS += $(ENV_CFLAGS)

LIBS += $(BLOCKDEV_MODULES_LINKER_ARGS) \
	$(COPY_MODULES_LINKER_ARGS) \
	$(SPDK_LIB_LINKER_ARGS) $(ENV_LINKER_ARGS)

CPPPATH=$(filter -I%, $(CFLAGS))

%:
	@echo $($@)


include $(SPDK_ROOT_DIR)/mk/spdk.deps.mk
