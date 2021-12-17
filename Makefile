default: Lithium

.PHONY: Lithium

Lithium:
	g++ Lithium.cpp -o Lithium
	./Lithium input.txt output.html
	