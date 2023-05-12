IMGUI_SRCS=$(wildcard imgui/*.cpp)
IMGUI_OBJS=$(patsubst imgui/%.cpp,output/imgui/%.o,$(IMGUI_SRCS))
IMGUI_HDRS=$(wildcard imgui/*.h)

CORE_SRCS=$(wildcard core/*.cpp)
CORE_OBJS=$(patsubst core/%.cpp,output/core/%.o,$(CORE_SRCS))
CORE_HDRS=$(wildcard core/*.h)

NVH_SRCS=$(wildcard nvh/*.cpp)
NVH_OBJS=$(patsubst nvh/%.cpp,output/nvh/%.o,$(NVH_SRCS))
NVH_HDRS=$(wildcard nvh/*.hpp)

NVGL_SRCS=$(wildcard nvgl/*.cpp)
NVGL_OBJS=$(patsubst nvgl/%.cpp,output/nvgl/%.o,$(NVGL_SRCS))
NVGL_HDRS=$(wildcard nvgl/*.hpp)

APP_SRCS=$(wildcard app/*.cpp app/*/*.cpp)
APP_OBJS=$(patsubst app/%.cpp,output/app/%.o,$(APP_SRCS))
APP_HDRS=$(wildcard app/*.h app/*/*.h)

LIB_OBJS=$(IMGUI_OBJS) $(NVH_OBJS) $(NVGL_OBJS)
MY_OBJS=$(CORE_OBJS) $(APP_OBJS)
OUTPUT_OBJS=$(LIB_OBJS) $(MY_OBJS) 

CPPFLAGS=-lGLEW -lGL -lglfw -lpthread --std=c++17 -O3 -I. -lstdc++fs

EXE_NAME = command_list_sample

$(EXE_NAME):$(OUTPUT_OBJS)
	g++ -Wformat -Wint-to-pointer-cast $(OUTPUT_OBJS) $(CPPFLAGS) -o $@


output/imgui/%.o: imgui/%.cpp $(IMGUI_HDRS)
	mkdir -p output/imgui/
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

output/core/%.o: core/%.cpp $(IMGUI_HDRS) $(CORE_HDRS)
	mkdir -p output/core/
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

output/app/%.o: app/%.cpp $(IMGUI_HDRS) $(CORE_HDRS) $(APP_HDRS) $(NVH_HDRS) $(NVGL_HDRS) 
	mkdir -p output/app/ output/app/path
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

output/nvh/%.o: nvh/%.cpp $(NVH_HDRS)
	mkdir -p output/nvh/
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

output/nvgl/%.o: nvgl/%.cpp $(NVH_HDRS) $(NVGL_HDRS)
	mkdir -p output/nvgl/
	g++ -c -Wformat -Wint-to-pointer-cast $< $(CPPFLAGS) -o $@

clean:
	rm -f $(MY_OBJS)

clean_all:
	rm -f $(OUTPUT_OBJS)

assets/srcipts/dist/save_as_psd.exe : assets/scripts/save_as_psd.py
	pyinstall.exe --onefile assets/scripts/save_as_psd.py