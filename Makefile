.PHONY : clean all

CC = g++
D_INC = include .
D_BUILD = build
CXXFLAGS = 
LIB = 

ifeq ($(OS),Windows_NT)
	CXXFLAGS += -D_WIN32_WINNT=0x0601
	LIB += wsock32 ws2_32
else
	ifeq ($(shell uname), Linux)
		LIB += pthread
	endif
endif

SOURCES = $(wildcard *.cpp)
TARGETS = $(basename $(notdir $(SOURCES)))
OBJECTS = $(addprefix $(D_BUILD)/,$(TARGETS:%=%.o)) 

all : $(TARGETS)

$(TARGETS) : % : $(D_BUILD)/%.o
	$(CC) $^ $(LIB:%=-l%) -o $(D_BUILD)/$@

$(OBJECTS) : $(D_BUILD)/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(addprefix -I,$(D_INC)) $(CXXFLAGS) -MMD -c $< -o $@

-include $(OBJECTS:%.o=%.d)

clean :
	rm -rf build