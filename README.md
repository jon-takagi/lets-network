## Changes from HW 2

## TCP Server

### Building
I installed boost into `/usr/local/boost_1_72_0`, then used bootstrap to make a copy in `/vagrant/systems/boost`. In our makefile, these are included in lines 2 and 21. If you install boost elsewhere, or don't use the VM provided by Eitan, then change these lines. Finally, after `make`ing our source files, ensure the library can be found at runtime by adding the copy you made with bootstrap to your `LD_LIBRARY_PATH`.

The command we used was `export LD_LIBRARY_PATH=/vagrant/systems/boost:$LD_LIBRARY_PATH`

## TCP Client
