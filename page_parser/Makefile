all: tera_easy_sample ec

tera_sample: tera_sample.cc
	g++ -o tera_sample tera_sample.cc -I./include -L./lib -ltera

tera_easy_sample: tera_easy_sample.cc
	g++ -o tera_easy_sample tera_easy_sample.cc -I./include -L./lib -ltera_easy -lpthread -lz

ec: ec.cc
	g++ -o ec ec.cc -I./include -L./lib -ltera_easy -lpthread -lz -lpk

clean:
	rm -rf tera_sample tera_easy_sample ec
