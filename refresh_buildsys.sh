rm -rf build
mkdir build
cd build
cmake .. -GNinja
ninja
# Alternatively, using make
# cmake ..
# make