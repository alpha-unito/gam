# ---------------------------------------------------------------------------
#  
#  This file is part of gam.
#
#  gam is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  gam is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with gam. If not, see <http://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------

#########################################################################
#
# the following paths are used only for building and launching:
# in a cluster environment, these paths are relative to frontend
#
#########################################################################
GAM_HOME             ?= ../../gam
GAM_INCS             = $(GAM_HOME)/include
GAM_RUN              = $(GAM_HOME)/bin/gamrun
GAM_CONF             ?= $(GAM_HOME)/conf/local.conf

ifdef GAM_TIMEOUT
TIMEOUT_PREFIX       = timeout -s HUP $(GAM_TIMEOUT)s
endif
GAM_CMD              = $(TIMEOUT_PREFIX) $(GAM_RUN)

VERBOSE              = #-v

#########################################################################
#
# application parameters
#
#########################################################################
ifdef STREAM_LEN
APPFLAGS += -DSTREAM_LEN=$(STREAM_LEN)
endif
ifdef BATCH_SIZE
APPFLAGS += -DBATCH_SIZE=$(BATCH_SIZE)
endif
ifdef NWORKERS
APPFLAGS += -DNWORKERS=$(NWORKERS)
endif

GAMCARD = $(shell echo $$(( $(NWORKERS) + 2 )))

#########################################################################
#
# some possible options to exercise:
#  - DGAM_LOG               enable logging
#  - DDFF2_LOG              enable logging of DFF2 programs
#  - DGAM_DBG               enable internal debugging
#
#########################################################################
CXX 		         = g++ -std=c++11
NVCC                 = /usr/local/cuda/bin/nvcc -ccbin=g++ -std=c++11
LINK_OPT             = 
VERSION              = 
OPTIMIZE_FLAGS       = -O3 -g0
CXXFLAGS             = -DDFF2_PROFILE #-Wall -DGAM_LOG -DGAM_DBG -DDFF2_LOG
LDFLAGS              = 
INCS                 = -I$(GAM_INCS) `pkg-config --cflags libfabric`
LIBS                 = -lpthread `pkg-config --libs libfabric`
ARCH                 = -march=$(shell uname -m)

INCLUDES             = -I. $(INCS)
TARGET               = stock_market baseline_stock_market

.PHONY: all clean distclean
.SUFFIXES: .cpp .o

%.o: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTIMIZE_FLAGS) $(APPFLAGS) -c -o $@ $<
%: %.o
	$(CXX) $< -o $@ $(LDFLAGS) $(LIBS)

all: $(TARGET)

stock_market: stock_market.o
baseline_stock_market: baseline_stock_market.o

#########################################################################
#
# test targets
#
#########################################################################

# assume all nodes can reach executables from $(PWD)
test: stock_market
	$(GAM_CMD) $(VERBOSE) -n $(GAMCARD) -f $(GAM_CONF) $(PWD)/stock_market

BASELINE_PREFIX=logs/baseline_stock_market
BASELINE_LOG=$(BASELINE_PREFIX)/$(shell date +%s)

baseline-test: baseline_stock_market
	mkdir -p $(BASELINE_LOG)
	./baseline_stock_market > $(BASELINE_LOG)/usr.out 2> $(BASELINE_LOG)/usr.err

#########################################################################
#
# cleanup
#
#########################################################################
clean: 
	-rm -fr *.o *~

distclean: clean
	-rm -fr $(TARGET)
	-rm -fr *.out *.err *.log
	-rm -fr *.dSYM *.btr

