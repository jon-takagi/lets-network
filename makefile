CXX=g++-8
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O0 -g -I /usr/local/boost_1_72_0/ -lpthread
LDFLAGS=$(CXXFLAGS)
OBJ=$(SRC:.cc=.o)

all: main.bin

main.bin: cache_lib.o test_cache_lib.o fifo_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	-rm *.o $(objects)
	-rm *.bin

touch:
	find . -type f -exec touch {} +
server.bin: cache_lib.o fifo_evictor.o
	$(CXX) $(LDFLAGS) cache_server.cc -o $@ $^ -L/vagrant/systems/boost/lib -lboost_program_options
