# gmake

gmake is a Makefile generator for simple C++ projects.

# Install dependency

```
sudo apt install libboost-dev libboost-all-dev
```

# Install gmake

```
git clone https://github.com/omaraflak/gmake.git && cd gmake
git submodule update --init
make
make install
```

# Use gmake

```
cd ./your/cpp/project
gmake exec_name
```
