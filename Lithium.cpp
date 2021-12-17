#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <list>

using namespace std;

class Error : std::exception
{
public:
	string error;
	int c_nb;

	Error(string e, int _c_nb = 0)
	{
		error = e;
		c_nb = _c_nb;
	}

	string what()
	{
		return error; // + " at character " + to_string(c_nb);
	}
};

class Token
{
public:
	virtual void create_code(ofstream& output) = 0;
};

int nb_sections = 0;

class Section : public Token
{
	int id;
	string name;

public:
	int level;

	Section(string _name, int _level)
	{
		nb_sections++;
		id = nb_sections;

		level = _level;
		name = _name;
	}

	void create_code(ofstream& output)
	{
		string level_str = to_string(level + 2);
		string opening_tag = "<h" + level_str + " id=\"" + to_string(id) + "\">";
		string closing_tag = "</h" + level_str + ">";

		output << opening_tag << name << closing_tag << "\n";
	}

	void create_summary_entry(ofstream& output)
	{
		output << "<li><a href=\"#" + to_string(id) + "\">" << name << "</a></li>\n";
	}
};

class Paragraph : public Token
{
	string text;

public:
	Paragraph(string _text)
	{
		text = _text;
	}

	void create_code(ofstream& output)
	{
		output << "<p>" << text << "</p>\n";
	}
};

void compile(ifstream& input, ofstream& output)
{
	// Globals values
	string author, title;
	list<Section*> sections;
	// Tokens
	list<Token*> tokens;

	// Searching tokens
	string buffer;
	char c;
	int result;
	while ((result = input.get()) != EOF) {
		c = (char)result;
		buffer = "";

		// Ignore newlines
		if (c == '\n') {
			continue;
		}

		if (c == '>') {
			getline(input, buffer, ' ');

			if (buffer == "Titre") {
				getline(input, title, '\n');
			}
			else if (buffer == "Auteur") {
				getline(input, author, '\n');
			}
			else {
				throw Error("Unknown chevron global data");
			}
		}
		else if (c == '=') {
			int level;
			string name;

			// Compter le niveaux de la section
			getline(input, buffer, ' ');
			level = 0;
			while (level < buffer.length()) {
				// Si les niveaux et le nom ne sont pas séparer par un espace:
				if (buffer[level] != '=') {
					throw Error("Bad section level");
				}
				level++;
			}

			// Récuperer le nom
			getline(input, name, '\n');

			Section* s = new Section(name, level);
			tokens.push_back(s);
			sections.push_back(s);
		}
		else {
			string text;
			text.push_back(c);

			do {
				getline(input, buffer, '\n');
				text += buffer;
			}
			while (!buffer.empty());

			Paragraph* p = new Paragraph(text);
			tokens.push_back(p);
		}
	}

	// Validating tokens
	if (title.empty()) {
		throw Error("Title empty");
	}
	if (author.empty()) {
		throw Error("Author empty");
	}

	// Creating html
	// Head
	output << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n";
	output << "<link rel=\"stylesheet\" href=\"style.css\">\n";
	// Title of the tab
	output << "<title>" << title << "</title>\n";

	// Body
	output << "</head>\n<body>\n";

	// Create summary
	output << "<div id=\"summary\">\n<h3>Table des matieres :</h3>\n";
	int last_level = -1;
	for (Section* s : sections) {
		// Create indentation
		if (s->level > last_level) {
			output << "<ul>\n";
		}
		else if (s->level < last_level) {
			output << "</ul>\n";
		}
		last_level = s->level;

		s->create_summary_entry(output);
	}
	output << "</ul>\n</div>\n";

	output << "<div id=\"header\">\n<h1 id=\"title\">" << title << "</h1>\n";
	output << "<h3 id=\"author\">" << author << "</h3>\n</div>\n";

	// Filling body
	for (Token* t : tokens) {
		t->create_code(output);
	}

	// End of html
	output << "</body>\n</html>\n";

	// Freeing tokens
	for (Token* t : tokens) {
		delete t;
	}
}

int main(int argc, char const* argv[])
{
	ifstream input;
	ofstream output;

	if (argc < 3) {
		cout << "Error: not enough arguments\nUsage: ./Lithium input output" << endl;
		return -1;
	}

	input.open(argv[1]);
	if (input.fail()) {
		cerr << "Error '" << argv[1] << "': " << strerror(errno) << endl;
		return -2;
	}

	output.open(argv[2]);
	if (output.fail()) {
		cerr << "Error '" << argv[2] << "': " << strerror(errno) << endl;
		return -2;
	}

	try {
		compile(input, output);
	}
	catch (Error e) {
		cerr << "Error at compile: " << e.what() << endl;
	}

	input.close();
	output.close();

	return 0;
}
