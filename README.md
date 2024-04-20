# pm
```
rm -rf build
mkdir build
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=mips-linux-uclibc-gnu.cmake
```

```
rm -rf build && mkdir build && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=mips-linux-uclibc-gnu.cmake && cmake --build build
```

```
valgrind -s --leak-check=full ./build/pm
valgrind -s --leak-check=full --show-leak-kinds=all ./build/pm
```

```
ffmpeg -f lavfi -i smptebars=duration=10:size=1280x544:rate=25 smptebars.mp4
ffmpeg -f lavfi -i testsrc=duration=10:size=1280x544:rate=25 testsrc.mpg

ffmpeg -i kungfu.mp4 -threads 3 -acodec copy -vcodec h264 -x264opts aud=1  -f segment -segment_time 05:00 -reset_timestamps 1 cam_out_%02d.mp4

./conv.sh mp4 h264 ./ ./ "-vcodec h264 -x264opts aud=1"

./conv.sh mp4 g711a ./ ./ "-acodec pcm_mulaw -f mulaw -ar 8000 -ac 1 "

ffmpeg -f lavfi -i "sine=frequency=1000:duration=5" -c:a pcm_s16le test.wav

ffmpeg -f lavfi -i "sine=frequency=1000:duration=5" -acodec pcm_mulaw -f mulaw -ar 8000 -ac 1 test.wav
```
