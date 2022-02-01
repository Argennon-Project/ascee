This project can be built on linux. You'll need to have [PBC](#installing-pbc)
library installed on your system in order to be able to build the project
completely.

If you are using an IDE, like [CLion](https://www.jetbrains.com/clion/), that
supports CMake and GoogleTest, after installing [PBC](#installing-pbc) you'll
just need to clone the project. The IDE will do the rest for you.

If you want to build the project without an IDE you'll need to
have [CMake](https://cmake.org/download/) >= 3.2 installed on your system.

For building the project without an IDE, first clone its repository:

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
cmake --build .
```

If the build was successful, you should be able to run the main executable: (
which is a small test right now)

```shell
./ascee_run
```

For running tests **navigate to `tests` directory** and run tests:

```shell
cd tests
./Google_Tests_run
```

```shell
./Google_Mock_run
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
and run `ldconfig` after adding the library path.

Then copy the `include` directory of PBC to your system's include directory:

```shell
sudo cp -r <pbc-install>/pbc-0.5.14/include/ /usr/include/pbc
```
