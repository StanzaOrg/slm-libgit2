
.conan2:
	conan config install ../conan-config

.PHONY: all
all: .conan2
	conan build -vtrace --profile:build=linux-x86_64-default --profile:host=linux-x86_64-gcc-11 --output-folder build --build never .
	cp -a build/build/Release/lgtest .
	ls -l lgtest

.PHONY: clean
clean:
	rm -rf build .conan2 lg2
