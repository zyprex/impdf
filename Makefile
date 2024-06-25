TARGET_MAIN:=impdf
TARGET_MERGE:=impdf_merge
TARGET_IMAGE:=impdf_image

all: $(TARGET_MAIN) $(TARGET_MERGE) $(TARGET_IMAGE)
# all: $(TARGET_MAIN)
# all: $(TARGET_MERGE)
# all: $(TARGET_IMAGE)

ifeq ($(OS),Windows NT)
PATHS:=-I.\\ -L.\\
EX:=.exe
RM:=del
LINK_MATH:=
else
PATHS:=-I./ -L./
EX:=
LINK_MATH:=-lm
RM:=rm
endif


$(TARGET_MAIN):
	@gcc -Wall $(PATHS) $(TARGET_MAIN).c -o $(TARGET_MAIN)$(EX)  -lpdfium
	@echo $(TARGET_MAIN) done.

$(TARGET_MERGE):
	@gcc -Wall $(PATHS) $(TARGET_MERGE).c pdfgen.c -o $(TARGET_MERGE)$(EX) \
		-Wno-format-extra-args -Wno-format $(LINK_MATH)
	@echo $(TARGET_MERGE) done.

$(TARGET_IMAGE):
	@gcc -Wall $(PATHS) $(TARGET_IMAGE).c -o $(TARGET_IMAGE)$(EX) $(LINK_MATH)
	@echo $(TARGET_IMAGE) done.

.PHONY: clean
clean:
	$(RM) $(TARGET_MAIN)$(EX) $(TARGET_MERGE)$(EX) $(TARGET_IMAGE)$(EX)
