default: Lithium

.PHONY: Lithium test

Lithium:
	g++ Lithium.cpp -o Lithium
	
test: Lithium
	./Lithium input.txt output.html
	