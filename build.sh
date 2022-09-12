if [ ! -d "dist/" ]; then
    mkdir dist
fi

common_include='-I ./include -I ./include/v8'
obj_list='dist/util.o dist/console.o dist/esm.o'
module_include='-I src/console -I src/esm'

gcc -c src/util/util.c -I src/util -o dist/util.o
g++ -c src/console/console.cc $common_include -I src/console -o dist/console.o
g++ -c src/esm/esm.cc $common_include -I src/util -I src/esm -o dist/esm.o
g++ $obj_list src/main.cc -I $common_include $module_include -L ./lib -lv8_monolith -m64 -DV8_COMPRESS_POINTERS -o dist/main
