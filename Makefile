VORBISLIBSLINUX=-llogg -lvorbisfile -lvorbis -logg
VORBISLIBSWIN=-lvorbisfile -lvorbis -logg -llogg
LINUXLIBS=-lm $(VORBISLIBSLINUX) -lalleggl `allegro-config --static` -lGL -lGLU -lglut -ltgui
WINLIBS=-lm $(VORBISLIBSWIN) -ltgui -lagl -lalleg -luser32 -lgdi32 -lopengl32 -lglu32 -lglut32
OBJFILES=pung.o input.o config.o
FLAGS=

default: linux

pung.o: pung.cpp pung.h
	g++ $(FLAGS) -c pung.cpp

input.o: input.cpp pung.h
	g++ $(FLAGS) -c input.cpp

config.o: config.cpp pung.h
	g++ $(FLAGS) -c config.cpp

linux: $(OBJFILES)
	g++ -o pung $(OBJFILES) $(LINUXLIBS)

win: $(OBJFILES)
	g++ -mwindows -o pung.exe $(OBJFILES) icon.res $(WINLIBS)

clean:
	rm -f $(OBJFILES)
	rm -f pung pung.exe pung.cfg
