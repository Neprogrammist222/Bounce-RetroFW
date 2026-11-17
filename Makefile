BUILDROOT ?= $(HOME)/retrofw-buildroot/output/host
CROSS_COMPILE ?= $(BUILDROOT)/bin/mipsel-RetroFW-linux-uclibc-

CXX_HOST = g++
CXX_MIPS = $(CROSS_COMPILE)g++

CXXFLAGS = -O2 -std=c++17 -Wall
LDFLAGS  = -lSDL -lSDL_image

TARGET = bounce

host: main.cpp
	$(CXX_HOST) $(CXXFLAGS) main.cpp -o $(TARGET)_host $(LDFLAGS)

mips: main.cpp
	$(CXX_MIPS) $(CXXFLAGS) main.cpp -o $(TARGET) $(LDFLAGS)
	$(CROSS_COMPILE)strip $(TARGET)

opk: mips
	rm -rf opk_build bounce.opk
	mkdir -p opk_build
	cp $(TARGET) opk_build/
	cp -r icons levels opk_build/
	printf '[Desktop Entry]\nType=Application\nName=Bounce\nComment=Nokia Bounce Port\nExec=bounce\nIcon=icons/bouncesplash\nCategories=games;\nTerminal=false\n' > opk_build/default.retrofw.desktop
	cp opk_build/default.retrofw.desktop opk_build/default.gcw0.desktop
	mksquashfs opk_build bounce.opk -all-root -noappend -no-exports -no-xattrs
	rm -rf opk_build

clean:
	rm -f $(TARGET) $(TARGET)_host bounce.opk