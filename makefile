IMGUI_SRCS=$(wildcard imgui/*.cpp)
IMGUI_OBJS=$(patsubst imgui/%.cpp,output/imgui/%.o,$(IMGUI_SRCS))
IMGUI_HDRS=$(wildcard imgui/*.h)

CORE_SRCS=$(wildcard core/*.cpp)
CORE_OBJS=$(patsubst core/%.cpp,output/core/%.o,$(CORE_SRCS))
CORE_HDRS=$(wildcard core/*.h)

APP_SRCS=$(wildcard app/*.cpp app/*/*.cpp)
APP_OBJS=$(patsubst app/%.cpp,output/app/%.o,$(APP_SRCS))
APP_HDRS=$(wildcard app/*.h app/*/*.h)

CPPFLAGS=-lGLEW -lGL -lglfw -lpthread --std=c++17 -I. -g -lstdc++fs

EXE_NAME = pattern_designer

$(EXE_NAME):$(IMGUI_OBJS) $(CORE_OBJS) $(APP_OBJS)
	g++ -Wformat -Wint-to-pointer-cast $(IMGUI_OBJS) $(CORE_OBJS) $(APP_OBJS) $(CPPFLAGS) -o $@


output/imgui/%.o: imgui/%.cpp $(IMGUI_HDRS)
	mkdir -p output/imgui/
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@


output/core/%.o: core/%.cpp $(IMGUI_HDRS) $(CORE_HDRS)
	mkdir -p output/core/
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

output/app/%.o: app/%.cpp $(IMGUI_HDRS) $(CORE_HDRS) $(APP_HDRS)
	mkdir -p output/app/ output/app/path
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

clean:
	rm -f $(APP_OBJS) $(CORE_OBJS) $(EXE_NAME)

assets/srcipts/dist/save_as_psd.exe : assets/scripts/save_as_psd.py
	pyinstall.exe --onefile assets/scripts/save_as_psd.py