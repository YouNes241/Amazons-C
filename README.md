# amazons-c

## Prerequisites

To build this project, you will need to install the following dependencies.

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    cmake \
    git \
    libgtk-4-dev \
    libgtest-dev \
    libgmock-dev \
    libreadline-dev \
    googletest \
    doxygen \
    graphviz \
    gcovr \
    gettext \
    clang-format
```

After successfully installing them, you can simply build the project via :

```bash
 mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Coverage .. && make
```

And run the executable :

```bash
./amazons <options>
```

If you want to use tensorflow, build the project like this instead : (please note tests will take longer and that this may not work on all architectures, as it uses tensorflow x86.)

```bash
 mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Coverage -DENABLE_ML=ON .. && make
```
