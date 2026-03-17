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
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Coverage ..
```

And run the executable :

```bash
./src/amazons-c
```

You may also run ``make`` and ``make clean``, for your convenience.
