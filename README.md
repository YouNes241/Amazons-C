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
cmake -B build -S . -DENABLE_COVERAGE=OFF && cmake --build build --parallel $(nproc) && cp build/amazons-c .
```

And run the executable :

```bash
./amazons-c
```

You may also run ``make`` and ``make clean``, for your convenience. ``make run`` is also available to run the project once built.
