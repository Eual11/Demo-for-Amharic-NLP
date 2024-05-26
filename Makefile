release:
	mkdir -p build 
	$(shell cd build && cmake ../)
	$(shell cd build && cmake --build . --config Release)
	$(shell cp ./build/_deps/sdl-build/Release/SDL2.dll ./build/Release/)
	$(shell cp -r ./font ./build/Release/)

debug:
	mkdir -p build 
	$(shell cd build && cmake ../)
	$(shell cd build && cmake --build . --config Debug)
	$(shell cp ./build/_deps/sdl-build/Debug/SDL2d.dll ./build/Debug/)
	$(shell cp -r ./font ./build/Debug/)
