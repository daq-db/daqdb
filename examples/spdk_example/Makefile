SPDK_ROOT_DIR := $(abspath $(CURDIR)/../../third-party/spdk)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.app.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

APP = spdk_example

CFLAGS += $(ENV_CFLAGS)

C_SRCS := spdk_example.c

SPDK_LIB_LIST = event_bdev event_copy
SPDK_LIB_LIST += nvmf event log trace conf util bdev copy rpc jsonrpc json
SPDK_LIB_LIST += app_rpc log_rpc bdev_rpc

LIBS += $(BLOCKDEV_MODULES_LINKER_ARGS) \
	$(COPY_MODULES_LINKER_ARGS) \
	$(SPDK_LIB_LINKER_ARGS) $(ENV_LINKER_ARGS)

all : $(APP)

$(APP) : $(OBJS) $(SPDK_LIB_FILES) $(SPDK_WHOLE_LIBS) $(BLOCKDEV_MODULES_FILES) $(LINKER_MODULES) $(ENV_LIBS)
	$(LINK_C)

clean :
	$(CLEAN_C) $(APP)

include $(SPDK_ROOT_DIR)/mk/spdk.deps.mk