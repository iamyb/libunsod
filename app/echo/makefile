all: unsod_echo unsod_echo_udp

unsod_echo: unsod_echo.c
	gcc -O3 -std=gnu99 unsod_echo.c -o unsod_echo -I../../lib/include -L../../lib -Wl,--whole-archive -lunsod -Wl,--no-whole-archive -ldl -lpthread -lcrypto -lrt
	gcc -O3 -std=gnu99 unsod_echo_udp.c -o unsod_echo_udp -I../../lib/include -L../../lib -Wl,--whole-archive -lunsod -Wl,--no-whole-archive -ldl -lpthread -lcrypto -lrt	
	
clean:
	rm unsod_echo unsod_echo_udp