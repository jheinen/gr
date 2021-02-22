set prefix=S:\opt\msvc2019_64
rem cmake -S .. -B . -D CMAKE_INSTALL_PREFIX=%prefix%\gr -D GR_USE_BUNDLED_LIBRARIES=OFF
cmake -D CMAKE_INSTALL_PREFIX=%prefix%\gr --config Release ..
cmake --build . --config Release
cmake --install . --config Release
