This is a tmp file to keep a track of the commands to build and test
Will be deleted before the final PR

### Activate env for build
``` bash
source /home/shivi/Desktop/spack/share/spack/setup-env.sh
spack env activate -p .
spack install -v
```

### Build
``` bash
mkdir build
cd build
cmake .. -DCHRONOLOG_BUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
```

### After changing the unit test make and test
``` bash
make all
ctest --output-on-failure
ctest -R StoryChunk --output-on-failure
```

### Auto format before push
``` bash
cp CodeStyleFiles/ChronoLog.clang-format .clang-format
clang-format -style=file -i test/unit/StoryChunkTest.cpp
```

### Notes
- First focusing on story chunk tests 
- finished insertevent and mergeevents
- Need to confirm behavior of a few test cases