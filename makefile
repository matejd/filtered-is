all:
	clang -g3 -Wall -o build/comp.exe main.cpp app.cpp common.cpp renderer.cpp stb_image.cpp -std=c++11 -lm -lGLEW -lpthread `pkg-config --cflags libglfw` `pkg-config --libs libglfw` -lGL -lstdc++

emscripten:
	emcc main.cpp app.cpp common.cpp renderer.cpp stb_image.cpp -s TOTAL_MEMORY=134217728 -s EXPORTED_FUNCTIONS="['_main','_setAppValue']" -o build/index_plain.html -std=c++11 -I. --preload-file assets
