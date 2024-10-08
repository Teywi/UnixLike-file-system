# Makefile for CS-212 course - spring 2023
# Author: Jean-Cédric Chappelier & Édouard Bugnion
# Date: May 2022

# main targets
# 	all: build the u6fs (early unix filesystem) program
# 	style: astyle formatting
# 	doc: documentation
#       feedback: execute tests within a container from source repo
#       check: local unit tests

# Note: builds with address sanitizer by default
TARGETS += u6fs

# ======================================================================
# Compiler and Link

CC = clang
CFLAGS += -g
CFLAGS += -pedantic -Wall
CFLAGS += -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings \
-Wconversion -Wunreachable-code -Wcast-qual -W -Wformat=2 -Winit-self -Wuninitialized

CFLAGS += -Wcast-align

## may require: export ASAN_OPTIONS=allocator_may_return_null=1
#               export ASAN_OPTIONS=verify_asan_link_order=0
# different -fsanitize options are available, including -fmemory
CPPFLAGS += -fsanitize=address
LDFLAGS  += -fsanitize=address
LDLIBS   += -fsanitize=address

# FUSE
CFLAGS += $(shell pkg-config fuse --cflags)
LDLIBS += $(shell pkg-config fuse --libs)

ifdef DEBUG
# add the debug flag, may need to comment this line when doing make feedback
#TODO : Make feedback should build with -UDEBUG
CPPFLAGS += -DDEBUG
endif

.PHONY: all

all:: $(TARGETS)

SRCS = u6fs.c error.c u6fs_utils.c

# WEEK 4 (provided as an example)
SRCS += mount.c
SRCS += sector.c inode.c

# WEEK 5
SRCS += filev6.c

# WEEK 6
SRCS += direntv6.c

# WEEK 8
SRCS += u6fs_fuse.c

# WEEK 10
SRCS += bmblock.c
#########################################################################
# DO NOT EDIT BELOW THIS LINE
#

LDLIBS += -lm -lssl -lcrypto


# FUSE "hello (world) example and compilation (in week 8)

FUSE_CFLAGS += -D_FILE_OFFSET_BITS=64
hello.o: hello.c
	$(CC) -D_DEFAULT_SOURCE $(FUSE_CFLAGS) -o $@ -c $<

hello: hello.o
	$(LINK.o) -o $@ $^ $(LDLIBS) $$(pkg-config fuse --libs)


OBJS=$(subst .c,.o,$(SRCS))

u6fs: $(OBJS)
	$(LINK.o) -o $@ $(OBJS) $(LDLIBS)

.PHONY: depend clean new static-check check release doc

# automatically generate the dependencies
# including .h dependencies !

depend: .depend

.depend: $(SRCS)
	bash -c "cat $(foreach SRC,$(SRCS),<($(CC) $(CFLAGS) -MM -MT $(SRC:.c=.o) $(SRC))) > ./.depend"

ifneq ($(MAKECMDGOALS),clean)
-include .depend
endif

clean::
	-@/bin/rm -f *.o *~  .depend $(TARGETS)

new: clean all

static-check:
	CCC_CC=$(CC) scan-build -analyze-headers --status-bugs -maxloop 64 make -j1 new

style:
	astyle -n -o -A8 -xt0 *.[ch]

TEST_DIR = ../provided/tests

unit-tests:
	make SRC_DIR=$${PWD} -B -C $(TEST_DIR)/unit

define e2e_test
robot -v srcdir:. -v datadir:$(TEST_DIR)/data $(TEST_DIR)/end-to-end/$1 ;
endef
end2end-tests: $(TARGETS)
	$(call e2e_test,week04.robot)
	$(call e2e_test,week05.robot)
	$(call e2e_test,week06.robot)
	$(call e2e_test,week08.robot)
	$(call e2e_test,week10.robot)
	$(call e2e_test,week11.robot)
	$(call e2e_test,week12.robot)
check: end2end-tests unit-tests

## --------------------------------------------------
# target to do all checks before releasing a new version by staff
release: new check style static-check clean

## --------------------------------------------------
# target to make documentation
doc: Doxyfile
	doxygen Doxyfile

## ======================================================================
## Feedback

DOCKER_OPTIONS = --cap-add SYS_ADMIN --device /dev/fuse --security-opt apparmor:unconfined
DOCKER_OPTIONS += -e REPO=$(REPO) -v $(SSH_DIR):/opt/.ssh $(IMAGE)

IMAGE := chappeli/pps23-feedback:latest
## Note: vous pouvez changer le tag latest pour week04, ou week05, etc.

REPO := $(shell git config --get remote.origin.url)
SSH_DIR := $(HOME)/.ssh

.PHONY: feedback feedback-pull feedback-debug feedback-VM-CO feedback-debug-VM-CO clone-ssh clean-fake-ssh

feedback-pull:
	@echo Will use $(REPO) inside container
	@docker pull $(IMAGE)
feedback: feedback-pull
	@docker run -it --rm $(DOCKER_OPTIONS)
feedback-debug: feedback-pull
	@docker run -it --rm --entrypoint=./debugmode.sh $(DOCKER_OPTIONS)


clone-ssh:
	@-$(eval SSH_DIR := $(HOME)/.$(shell date "+%s;$$"|sha256sum|cut -c-32))
	@cp -r $(HOME)/.ssh/. $(SSH_DIR)

clean-fake-ssh:
	@case $(SSH_DIR) in $(HOME)/\.????????????????????????????????) $(RM) -fr $(SSH_DIR) ;; *) echo "Dare not remove \"$(SSH_DIR)\"" ;; esac

feedback-VM-CO: clone-ssh feedback clean-fake-ssh

feedback-debug-VM-CO: clone-ssh feedback-debug clean-fake-ssh

## ======================================================================
## Submit

SUBMIT_SCRIPT=../provided/submit.sh

.PHONY: submit1 submit2 submit

submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit3: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 3

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1", "make submit2" or "make submit3"...\n'
