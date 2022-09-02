if [ ! -d "dist/" ]; then 
    mkdir dist 
fi
gcc -c src/eio/eio.c -I ./include -I src/eio -o dist/eio.o
gcc -c src/ev/ev.c -I ./include -I src/eio -I src/ev -o dist/ev.o
g++ dist/eio.o dist/ev.o src/fs/fs.cc src/main.cc -I ./include/v8 -I ./include -I src/eio -I src/ev -I src/fs -L ./lib -lv8_monolith -m64 -DV8_COMPRESS_POINTERS -o dist/main
