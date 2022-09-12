src = src
$(shell if [ ! -d "dist/" ]; then mkdir dist; fi)
dist = dist
module_names = util console esm ev
common_includes = -I ./include -I ./include/v8
all_includes = $(common_includes) $(foreach n, $(module_names), -I $(src)/$(n))
objs = $(foreach n, $(module_names), $(dist)/$(n).o)
libs = -L ./lib -lv8_monolith
flags = -m64 -DV8_COMPRESS_POINTERS

$(dist)/main: $(objs) $(src)/main.cc
	g++ -o $@ $^ $(all_includes) $(libs) $(flags)

$(dist)/util.o: $(src)/util/util.c
	gcc -o $@ -c $^

$(dist)/console.o: $(src)/console/console.cc
	g++ -o $@ $(common_includes) -c $^

$(dist)/esm.o: $(src)/esm/esm.cc
	g++ -o $@ $(common_includes) -I $(src)/util -c $^

$(dist)/ev.o: $(src)/ev/ev.c
	gcc -o $@ $(common_includes) -c $^

.PHONY: clean
clean:
	rm -rf $(dist)/*
