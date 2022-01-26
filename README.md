This project can be built on linux. You'll need to
have [cmake](https://cmake.org/install/#download-verification)
and [pcb](https://github.com/Argennon-Project/ascee/tree/main/src/util/crypto#readme)
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