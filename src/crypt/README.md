The [PBC library](https://crypto.stanford.edu/pbc/) needs the GMP library. So you need to first install the GMP. After
installing the GMP download and extract the [PBC source](https://crypto.stanford.edu/pbc/download.html) and run:

```shell
$ ./configure
```

Install any package that is missing. Then run:

```shell
$ make
$ make install
```

By default, the library is installed in `/usr/local/lib`. On some systems, this may not be in the library path. One way
to fix this is to edit `/etc/ld.so.conf` and run `ldconfig`.

For compiling, don't forget to add the `include` directory of PCB to your include path, and linked against the PBC
library and the GMP library:

```shell
$ gcc main.cpp -I ~/pbc-0.5.14/include -lpbc -lgmp
```

or

```shell
$ gcc main.cpp -I ~/pbc-0.5.14/include -static -lpbc -lgmp
```