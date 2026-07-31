git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git checkout dfb9d1a46c3bb8f52e1e6324be23123b9d73c190

./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

cd ..

emsdk/upstream/emscripten/emcmake cmake -S . -B build-em
cd build-em
../emsdk/upstream/emscripten/emmake make -j
