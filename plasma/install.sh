
if [ ! -d build ]; then
  mkdir build
fi

cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix`
make -s
sudo make -s install

# kquitapp plasma && plasma

