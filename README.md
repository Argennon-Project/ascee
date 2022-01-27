This project can be built on linux. You'll need to
have [cmake](https://cmake.org/install/#download-verification)
and [pcb](#installing-pbc)
installed on your system in order to be able to build the project completely.

For building the project first clone its repository:

```shell
git clone https://github.com/Argennon-Project/ascee.git
```

Next, create a build directory and navigate to it:

```shell
cd ascee
mkdir cmake-build && cd cmake-build
```

Now you can build the project:

```shell
cmake ..
```

```shell
cmake --build .
```

If the build was successful, you should be able to run the main executable: (
which is a small test right now)

```shell
./ascee_run
```

For being able to run tests, first you'll need to copy the `param` directory
from the main directory of the project to the build directory:

```shell
cp -r ../param/ param
```

Then to run tests **navigate to `tests` directory** and run tests:

```shell
cd tests
```

```shell
./Google_Tests_run
```

### Installing PBC

The [PBC library](https://crypto.stanford.edu/pbc/) needs the GMP library. So
you need to first install the GMP. After installing the GMP download and extract
the [PBC source](https://crypto.stanford.edu/pbc/download.html) and run:

```shell
./configure
```

Install any package that is missing. Then run:

```shell
make
make install
```

By default, the library is installed in `/usr/local/lib`. On some systems, this
may not be in the library path. One way to fix this is to edit `/etc/ld.so.conf`
and run `ldconfig` after adding the path.

Then copy the `include` directory of PCB to your system's include directory:

```shell
sudo cp -r <pbc-install>/pbc-0.5.14/include/ /usr/include/pbc
```
