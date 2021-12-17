#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <list>

using namespace std;

class InputFacade
{
	ifstream* input;
	int lines_read;
	int chars_since_line_start;

public:
	InputFacade(ifstream* _input)
	{
		input = _input;
	}

	int get()
	{
		int i = input->get();

		if (i == '\n') {
			lines_read++;
			chars_since_line_start = 0;
		}
		else {
			chars_since_line_start++;
		}

		return i;
	}

	string get(char delimiter)
	{
		string result;

		char c;
		do {
			c = get();
			result.push_back(c);
		}
		while (c != delimiter);

		return result;
	}

	void get_pos(int& line, int& chars)
	{
		line = lines_read;
		chars = chars_since_line_start;
	}
};

class CompilingError : std::exception
{
public:
	string reason;
	int line_number;
	int char_number;

	CompilingError(string e, InputFacade input)
	{
		reason = e;
		input.get_pos(line_number, char_number);
	}

	string what()
	{
		return reason + " at (" + to_string(line_number) + ", " + to_string(char_number) + ")";
	}
};

class ValidationError : std::exception
{
public:
	string reason;

	ValidationError(string e)
	{
		reason = e;
	}

	string what()
	{
		return reason;
	}
};

class TreeNode
{
	list<TreeNode*> childrens;
	TreeNode* parent = nullptr;

public:
	~TreeNode()
	{
		for (TreeNode* child : childrens) {
			delete child;
		}
	}

	TreeNode* get_parent()
	{
		return parent;
	}

	void encode_to(ofstream& output)
	{
		encode_childrens(output);
	}

	void encode_childrens(ofstream& output)
	{
		for (TreeNode* child : childrens) {
			child->encode_to(output);
		}
	}

	void validate()
	{
		validate_childrens();
	}

	void validate_childrens()
	{
		for (TreeNode* child : childrens) {
			child->validate();
		}
	}

	void push_back_child(TreeNode* child)
	{
		child->parent = this;
		childrens.push_back(child);
	}

	void push_front_child(TreeNode* child)
	{
		child->parent = this;
		childrens.push_front(child);
	}
};

class Section : public TreeNode
{
	string name;
	list<int> id;

public:
	Section(string _name, Section* parent = nullptr, bool parent_is_brother = false)
	{
		name = _name;

		if (parent == nullptr) {
			// Init the list to [1]
			id.push_back(1);
		}
		else {
			id = list<int>(parent->id);
			if (parent_is_brother) {
				parent->get_parent()->push_back_child(this);

				// Indent the last id number (1.1 => 1.2)
				id.back()++;
			}
			else {
				parent->push_back_child(this);

				// Add a new number (2.1 => 2.1.1)
				id.push_back(1);
			}
		}
	}

	// TODO 3 next functions
	void create_indentation(ofstream& output, string opening, string closing)
	{
		if (level > last_level) {
			output << opening;
		}
		else if (level < last_level) {
			output << closing;
		}
		last_level = level;
	}

	void encode_to(ofstream& output)
	{
		static int last_level = 0;
		string level_str = to_string(level + 2);
		string opening_tag = "<h" + level_str + " id='" + to_string(id) + "'>";
		string closing_tag = "</h" + level_str + ">";

		create_indentation(output, "<div class='section'>\n", "</div>\n");
		output << opening_tag << name << closing_tag << "\n";
	}

	void create_summary_entry(ofstream& output)
	{
		create_indentation(output, "<ul>\n", "</ul>\n");
		output << "<li><a href='#" + to_string(id) + "'>" << name << "</a></li>\n";
	}
};

class Paragraph : public TreeNode
{
	string text;

public:
	Paragraph(string _text)
	{
		text = _text;
	}

	void encode_to(ofstream& output)
	{
		output << "<p>" << text << "</p>\n";
	}
};

class PageAuthor : public TreeNode
{
	string name;

public:
	PageAuthor(string _name)
	{
		name = _name;
	}

	void encode_to(ofstream& output)
	{
		output << "<h3 id='author'>" << name << "</h3>\n";
	}
};

class PageTitle : public TreeNode
{
	string name;

public:
	PageTitle(string _name)
	{
		name = _name;
	}

	string get_name()
	{
		return name;
	}

	void encode_to(ofstream& output)
	{
		output << "<h1 id='title'>" << name << "</h1>\n";
	}
};

class TabTitle : public TreeNode
{
	PageTitle* title;

public:
	TabTitle(PageTitle* _title)
	{
		title = _title;
	}

	void encode_to(ofstream& output)
	{
		output << "<title>" << title->get_name() << "</title>\n";
	}
};

class Summary : public TreeNode
{
	list<Section*> sections;

public:
	Summary(list<Section*> _sections)
	{
		sections = _sections;
	}

	void encode_to(ofstream& output)
	{
		output << "<div id='summary'>\n<h3>Table des matieres :</h3>\n";
		for (Section* s : sections) {
			s->create_summary_entry(output);
		}
		output << "</div>\n";
	}
};

class HTML_Body : public TreeNode
{
public:
	void encode_to(ofstream& output)
	{
		output << "<body>\n";
		TreeNode::encode_childrens(output);
		output << "</body>\n";
	}
};

class HTML_Header : public TreeNode
{
public:
	void encode_to(ofstream& output)
	{
		output << "<head>\n";
		output << "<meta charset='UTF-8'>\n";
		output << "<link rel='stylesheet' href='style.css'>\n";
		TreeNode::encode_childrens(output);
		output << "</head>\n";
	}
};

class HTML : public TreeNode
{
	HTML_Header* h;
	HTML_Body* b;

	PageTitle* t = nullptr;
	PageAuthor* a = nullptr;

public:
	HTML()
	{
		h = new HTML_Header();
		b = new HTML_Body();
		TreeNode::push_back_child(h);
		TreeNode::push_back_child(b);
	}

	void encode_to(ofstream& output)
	{
		output << "<!DOCTYPE html>\n";
		output << "<html lang='en'>\n";
		TreeNode::encode_childrens(output);
		output << "</html>\n";
	}

	// Need a function to do it in differed because the title can be anywhere
	void link_page_header(list<Section*> sections, PageTitle* title, PageAuthor* author)
	{
		t = title;
		a = author;

		h->push_back_child(new TabTitle(title));

		// In reversed order of appearance
		b->push_front_child(author);
		b->push_front_child(title);
		b->push_front_child(new Summary(sections));
	}

	void add_to_body(TreeNode* n)
	{
		b->push_back_child(n);
	}

	void validate()
	{
		if (t == nullptr) {
			throw ValidationError("Title not found");
		}
		if (a == nullptr) {
			throw ValidationError("Author not found");
		}

		TreeNode::validate_childrens();
	}
};

void compile(ifstream& input, ofstream& output)
{
	// Globals values
	PageAuthor* author = nullptr;
	PageTitle* title = nullptr;
	list<Section*> sections;
	HTML root;

	// Creation of the facade
	InputFacade facade(&input);

	// TODO: add paragraphes to sections (don't forget first p doesn't have a section)
	// Searching tokens
	string buffer;
	char c;
	int result;
	while ((result = facade.get()) != EOF) {
		c = (char)result;
		buffer = "";

		// Ignore newlines
		if (c == '\n') {
			continue;
		}

		if (c == '>') {
			buffer = facade.get(' ');

			if (buffer == "Titre") {
				title = new PageTitle(facade.get('\n'));
			}
			else if (buffer == "Auteur") {
				author = new PageAuthor(facade.get('\n'));
			}
			else {
				throw CompilingError("Unknown >command '" + buffer + "'", facade);
			}
		}
		else if (c == '=') {
			int level;
			string name;

			// Compter le niveaux de la section
			buffer = facade.get(' ');
			level = 0;
			while (level < buffer.length()) {
				// Si les niveaux et le nom ne sont pas séparer par un espace:
				if (buffer[level] != '=') {
					throw CompilingError("Bad section level", facade);
				}
				level++;
			}

			// Récuperer le nom
			name = facade.get('\n');

			// Section(name, parent, parent_is_brother)
			Section* s = new Section(name, parent, parent_is_brother);
			root.add_to_body(s);
			sections.push_back(s);
		}
		else {
			string text;
			text.push_back(c);

			do {
				buffer = facade.get('\n');
				text += buffer;
			}
			while (!buffer.empty());

			Paragraph* p = new Paragraph(text);
			root.add_to_body(p);
		}
	}

	// Validating tokens
	root.validate();

	// Creating html
	root.encode_to(output);
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
	catch (CompilingError e) {
		cerr << "Error at compile: " << e.what() << endl;
	}
	catch (ValidationError e) {
		cerr << "Error at validation: " << e.what() << endl;
	}

	input.close();
	output.close();

	return 0;
}
