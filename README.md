# TODO:

1. why doesnt `PUT` work
2.





## Changes from HW 2

## TCP Server




### Building
I installed boost into `/usr/local/boost_1_72_0`, then used bootstrap to make a copy in `/vagrant/systems/boost`. In our makefile, these are included in lines 2 and 21. If you install boost elsewhere, or don't use the VM provided by Eitan, then change these lines.

Note that in our makefile we only specify `program_options` to the linker. This is because the other boost libraries we used are "header only" and there is no library archive to link to the compiler, simply using the `-I` flag to add the headers to the include path is sufficient.

## TCP Client
