CXX=g++-8
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O0 -g -I /usr/local/boost_1_72_0/ -pthread
LDFLAGS=$(CXXFLAGS)
OBJ=$(SRC:.cc=.o)

all: server.bin client.bin

test.bin: cache_lib.o test_cache_lib.o fifo_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	-rm *.o $(objects)
	-rm *.bin

touch:
	find . -type f -exec touch {} +
server.bin: cache_lib.o fifo_evictor.o
	$(CXX) $(LDFLAGS) cache_server.cc -o $@ $^ /vagrant/systems/boost/lib/libboost_program_options.a

echo_serv.bin: cache_lib.o fifo_evictor.o
	$(CXX) $(LDFLAGS) echo_serv.cc -o $@ $^ /vagrant/systems/boost/lib/libboost_program_options.a
