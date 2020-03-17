# $Id: Makefile,v 2.0 2008/10/08 18:55:40 Update $ 
prog_name = testtcp #目标文件名
###################################### 
# 
# Generic makefile 
# 
# by Jackie Xie 
# email: jackie.CPlusPlus@gmail.com 
# 
# Copyright (c) 2008 Jackie Xie 
# All rights reserved. 
# 
# No warranty, no liability; 
# you use this at your own risk. 
# 
# You are free to modify and 
# distribute this without giving 
# credit to the original author. 
# 
###################################### 
### Customising 
# 
# Adjust the following if necessary; EXECUTABLE is the target 
# executable"s filename, and LIBS is a list of libraries to link in 
# (e.g. alleg, stdcx, iostr, etc). You can override these on make"s 
# command line of course, if you prefer to do it that way. 
# 
EXECUTABLE := $(prog_name) 
LIBS := -lpthread #库文件   没有可不写
# Now alter any implicit rules" variables if you like, e.g.: 
# 
CFLAGS := -g -Wall -O
CXXFLAGS := $(CFLAGS) 
#CC := arm-linux-gcc   #编译器
CC := gcc   #编译器
# The next bit checks to see whether rm is in your djgpp bin 
# directory; if not it uses del instead, but this can cause (harmless) 
# `File not found" error messages. If you are not using DOS at all, 
# set the variable to something which will unquestioningly remove 
# files. 
# 
ifneq ($(wildcard $(DJDIR)/bin/rm),) 
RM-F := rm -f 
else 
RM-F := del 
endif 
# You shouldn"t need to change anything below this point. 
# 
# ................... 
SOURCE := $(wildcard *.cpp) $(wildcard *.c) 
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SOURCE))) 
DEPS := $(patsubst %.o,%.d,$(OBJS)) 
MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS)) 
MISSING_DEPS_SOURCES := $(wildcard $(patsubst %.d,%.cpp,$(MISSING_DEPS)) $(patsubst %.d,%.cpp,$(MISSING_DEPS))) 
CPPFLAGS += -MD 
.PHONY : everything deps objs clean veryclean rebuild 
everything : $(EXECUTABLE) 
deps : $(DEPS) 
objs : $(OBJS) 
clean :
	@$(RM-F) *.o 
	@$(RM-F) *.d
 
veryclean: clean
	@$(RM-F) $(EXECUTABLE)
	@$(RM-F) *.*~ *~ 
distclean: veryclean 
rebuild: veryclean everything 
ifneq ($(MISSING_DEPS),) 
$(MISSING_DEPS) :
	@$(RM-F) $(patsubst %.d,%.o,$@) 
endif 
-include $(DEPS)
 
$(EXECUTABLE) : $(OBJS)
	$(CC) -o $(EXECUTABLE) $(OBJS) $(addprefix ,$(LIBS))
