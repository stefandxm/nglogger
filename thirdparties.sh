cd thirdparty

cd include
wget https://github.com/nlohmann/json/releases/download/v3.10.0/json.hpp
cd ..

git clone https://github.com/msgpack/msgpack-c.git
cd msgpack-c
git checkout cpp_master
cmake "-DMSGPACK_CXX[11|17]=ON" "-DBOOST_ROOT=$1"  -DCMAKE_INSTALL_PREFIX:PATH=$PWD/../  .
make
make install
