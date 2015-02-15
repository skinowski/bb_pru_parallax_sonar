

PRU_INC=/home/tceylan/bbb/git/am335x_pru_package/pru_sw/app_loader/include
PRU_LIB=/home/tceylan/bbb/git/am335x_pru_package/pru_sw/app_loader/lib
PRU_PASM=/home/tceylan/bbb/git/am335x_pru_package/pru_sw/utils/pasm

NAME=sample
CPP=arm-linux-gnueabihf-g++-4.6
CPPFLAGS=-g -O2 -MMD -fno-exceptions -fno-rtti -I$(PRU_INC)
LDFLAGS=-lrt -L$(PRU_LIB) -lprussdrv -static

OBJDIR = obj
SOURCES := $(wildcard *.cpp)

.PHONY: clean push all

OBJECTS := $(SOURCES:%.cpp=$(OBJDIR)/%.o)

$(OBJDIR)/%.o : %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/$(NAME) : $(OBJECTS)
	$(CPP) -o $@ $^ $(LDFLAGS)

#
# compile pru binary using PRU assembler 
#
$(OBJDIR)/sonar.bin : sonar.p sonar.hp sonar_common.hp
	$(PRU_PASM) -V3 -b sonar.p
	mv -f sonar.bin $(OBJDIR)/sonar.bin
	chmod gou+rx $(OBJDIR)/sonar.bin

all: $(OBJDIR)/$(NAME) $(OBJDIR)/sonar.bin

clean :
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d $(OBJDIR)/$(NAME) $(OBJDIR)/sonar.bin

-include $(OBJECTS:.o=.d)
