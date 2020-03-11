main:
	mkdir -p bin
	g++ main.cpp -o bin/usbman -lstdc++fs
	
clean:
	rm -rf bin
