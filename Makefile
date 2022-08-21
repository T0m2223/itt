build: image_to_ascii.cc
	g++ -o ~/.trg/img2txt/img2txt *.cc -lX11 -pthread

test: ~/.trg/img2txt/img2txt
	~/.trg/img2txt/img2txt ~/ttt.psm ~/test.png 30

clean:
	rm ~/.trg/img2txt/*
