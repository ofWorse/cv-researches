Build:

g++ -std=c++17 motion.cpp -o motion $(pkg-config --cflags --libs opencv4)

Run:

./motion             - opens your camera as source stream
./motion video.mp4   - opens a video file as source stream
./motion video.mp4 N - opens a video file and applies N as an threshold
