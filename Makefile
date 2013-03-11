CFG_DEBUG	?= 0
CFG_LITTLE 	?= 1

src := $(wildcard src/*.c src/build_in_cmds/*.c)
src_nodir := $(notdir $(src))

objs := $(src:%.c=%.o)    #this objs has src directory info
objs_objdir := $(src_nodir:%.c=obj/%.o) #this objs just put the .o file to the obj directory

CFLAGS := -c -g -I./include
LD_FLAGS := -L./obj -lshell -lpthread 

ifeq ($(strip ${CFG_DEBUG}),1)
	CFLAGS += -DDBG_MEM -DDEBUG -Wall
endif

ifeq ($(strip ${CFG_LITTLE}),1)
	CFLAGS += -DLITTLE_ENDIAN
endif

all:clean shell
#	@echo "objs = $(objs) "
#	@echo "objs_objdir = $(objs_objdir)"
#	@echo "[ LD $(notdir $(objs_objdir))	===> shell ]"
#	@cc $(objs_objdir) $(LD_FLAGS) -o shell
	@echo  compile success. 

shell:libshell 
	@echo "[ CC	main.c		===> main.o	]"
	@cc $(CFLAGS) main.c -o obj/main.o
	@echo "[ LD libshell.a main.o	===> shell	]"
	@cc	obj/main.o $(LD_FLAGS) -o shell

libshell:$(objs)
	@echo "[ LD $(notdir $(objs_objdir))	===> libshell.a ]"
	@ar cq obj/libshell.a $(objs_objdir)

$(filter %.o,$(objs)):%.o:%.c
	@echo "[ CC $(notdir $^)		==> $(notdir $@)	]"
	@cc $(CFLAGS) $< -o obj/$(notdir $@)

.PHONY:clean
clean:
	-rm -rf $(objs_objdir) shell obj/libshell.a

