#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <list>

using namespace std;

string c_to_string(char c)
{
	switch (c) {
	case '\n':
		return string("\\n");

	case '\r':
		return string("\\r");

	case '\t':
		return string("\\t");

	default:
		return string(1, c);
	}
}

class InputFacade
{
	ifstream* input;
	int lines_read = 0;
	int chars_since_line_start = 0;
	int last_line_chars = 0;

public:
	InputFacade(ifstream* _input)
	{
		input = _input;
	}

	void debug_pos(int i)
	{
		int lines, chars;
		get_pos(lines, chars);
		cout << "[" << lines << ", " << chars << "]: ";

		if (i == EOF) {
			cout << "EOF" << endl;
		}
		else {
			cout << c_to_string(i) << " (" << i << ")" << endl;
		}
	}

	int get()
	{
		int i = input->get();

		// debug_pos(i);

		if (i == EOF) {
			return i;
		}

		if (i == '\n') {
			lines_read++;
			last_line_chars = chars_since_line_start + 1;
			chars_since_line_start = 0;
		}
		else {
			chars_since_line_start++;
		}

		return i;
	}

	string get(char delimiter)
	{
		string buffer;

		int result;
		while ((result = get()) != EOF && result != delimiter) {
			buffer.push_back((char)result);
		}

		return buffer;
	}

	void get_pos(int& line, int& chars)
	{
		line = lines_read + 1;
		chars = chars_since_line_start;

		if (chars == 0) {
			line--;
			chars = last_line_chars;
		}
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
protected:
	list<TreeNode*> childrens;
	TreeNode* parent = nullptr;

public:
	string type;
	TreeNode()
	{
		type = "TreeNode";
	}

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

	virtual void encode_to(ofstream& output)
	{
		encode_childrens(output);
	}

	void encode_childrens(ofstream& output)
	{
		for (TreeNode* child : childrens) {
			child->encode_to(output);
		}
	}

	virtual void validate()
	{
		// cout << "Type: " + type << endl;
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

class BodySection : public TreeNode
{
public:
	string name;
	list<int> id;

	BodySection(string _name)
	{
		name = _name;
		type = "BodySection: " + name;
	}

	BodySection* get_parent()
	{
		return (BodySection*)parent;
	}

	void set_id(list<int> _id)
	{
		id = list<int>(_id);
	}

	string get_id_str()
	{
		string s;

		for (int i : id) {
			s += to_string(i) + '.';
		}
		s.pop_back();

		return s;
	}

	string get_h()
	{
		int level = id.size();
		return "h" + to_string(level + 2);
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<div class='section'>\n";

		string id_str = get_id_str();
		output << "<" + get_h() + " id='" + id_str + "'>" << id_str + " " + name
			<< "</" + get_h() + ">"
			<< "\n";

		TreeNode::encode_childrens(output);

		output << "</div>\n";
	}
};

class SummarySection : public BodySection
{
public:
	SummarySection(BodySection& s)
		: BodySection(s)
	{}

	SummarySection(string name)
		: BodySection(name)
	{}

	void encode_to(ofstream& output)
	{
		string id_str = get_id_str();
		output << "<li><a href='#" + id_str + "'>" << id_str + " " + name << "</a></li>\n";

		if (childrens.size() != 0) {
			output << "<ul>\n";
			TreeNode::encode_childrens(output);
			output << "</ul>\n";
		}
	}
};

class MainBodySection : public BodySection
{
	TreeNode* parent;

public:
	MainBodySection()
		: BodySection("main")
	{
		type = "MainBodySection";
	}

	TreeNode* get_parent()
	{
		return parent;
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<div class='section'>\n";
		TreeNode::encode_childrens(output);
		output << "</div>\n";
	}
};

class MainSummarySection : public SummarySection
{
public:
	MainSummarySection()
		: SummarySection("MainSummarySection")
	{
		type = "MainSummarySection";
	}

	TreeNode* get_parent()
	{
		return parent;
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<div id='summary'>\n<h3>Table des matieres :</h3>\n<ul>\n";
		TreeNode::encode_childrens(output);
		output << "</ul>\n</div>\n";
	}
};

class Paragraph : public TreeNode
{
	string text;

public:
	Paragraph(string _text)
	{
		text = _text;
		type = "Paragraph " + text;
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<p>\n" << text << "</p>\n";
	}
};

class PageAuthor : public TreeNode
{
	string name;

public:
	PageAuthor(string _name)
	{
		name = _name;
		type = "PageAuthor " + name;
	}

	virtual void encode_to(ofstream& output)
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
		type = "PageTitle " + name;
	}

	string get_name()
	{
		return name;
	}

	virtual void encode_to(ofstream& output)
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
		type = "TabTitle";
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<title>" << title->get_name() << "</title>\n";
	}
};

class HTML_Body : public TreeNode
{
public:
	HTML_Body()
	{
		type = "HTML_Body";
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<body>\n";
		TreeNode::encode_childrens(output);
		output << "</body>\n";
	}
};

class HTML_Header : public TreeNode
{
public:
	HTML_Header()
	{
		type = "HTML_Header";
	}

	virtual void encode_to(ofstream& output)
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
	HTML_Header* header;
	HTML_Body* body;

	PageTitle* title = nullptr;
	PageAuthor* author = nullptr;

public:
	HTML()
	{
		type = "HTML";

		header = new HTML_Header();
		body = new HTML_Body();

		TreeNode::push_back_child(header);
		TreeNode::push_back_child(body);
	}

	virtual void encode_to(ofstream& output)
	{
		output << "<!DOCTYPE html>\n";
		output << "<html lang='en'>\n";
		TreeNode::encode_childrens(output);
		output << "</html>\n";
	}

	// Need a function to do it in differed because the title can be anywhere
	void link_page_header(MainSummarySection* _summary, PageTitle* _title, PageAuthor* _author)
	{
		title = _title;
		author = _author;

		header->push_back_child(new TabTitle(title));

		// In reversed order of appearance
		body->push_front_child(author);
		body->push_front_child(title);
		body->push_front_child(_summary);
	}

	void add_to_body(TreeNode* n)
	{
		body->push_back_child(n);
	}

	virtual void validate()
	{
		if (title == nullptr) {
			throw ValidationError("Title not found");
		}
		if (author == nullptr) {
			throw ValidationError("Author not found");
		}

		TreeNode::validate();
	}
};

void compile(ifstream& input, ofstream& output)
{
	// Creation of the facade
	InputFacade facade(&input);

	// Root
	HTML root;

	// HTML Header
	PageAuthor* author = nullptr;
	PageTitle* title = nullptr;
	MainSummarySection* summary = new MainSummarySection();

	// Global vars
	BodySection* current_body_s = new MainBodySection();
	BodySection* current_sum_s = summary;
	int current_s_lvl = 0;

	// Init
	root.add_to_body(current_body_s);

	cout << "Tokenizing..." << endl;

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
			// Compter le niveaux de la section
			int level = 1;
			while ((result = facade.get()) != EOF && (char)result == '=') {
				level++;
			}

			if ((char)result != ' ') {
				throw CompilingError("Section equals and name needs to be separated by a space character", facade);
			}

			// Make it allowable with a while
			if (abs(level - current_s_lvl) > 1) {
				throw CompilingError("More than one identation level change is not allowed", facade);
			}

			// Récuperer le nom
			string name = facade.get('\n');

			// Creer les sections
			BodySection* body_s = new BodySection(name);
			SummarySection* sum_s = new SummarySection(name);

			// Determiner leur indexation
			list<int> id = list<int>(current_body_s->id); // copy current id

			if (level > current_s_lvl) {
				current_body_s->push_back_child(body_s);
				current_sum_s->push_back_child(sum_s);

				// Add a new number (2.1 => 2.1.1)
				id.push_back(1);
			}
			else if (level < current_s_lvl) {
				current_body_s->get_parent()->get_parent()->push_back_child(body_s);
				current_sum_s->get_parent()->get_parent()->push_back_child(sum_s);

				id.pop_back();
				id.back()++;
			}
			else {
				current_body_s->get_parent()->push_back_child(body_s);
				current_sum_s->get_parent()->push_back_child(sum_s);

				id.back()++;
			}

			body_s->set_id(id);
			sum_s->set_id(id);

			current_sum_s = sum_s;
			current_body_s = body_s;
			current_s_lvl = level;
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
			current_body_s->push_back_child(p);
		}
	}

	root.link_page_header(summary, title, author);

	// Validating tree
	cout << "Validating..." << endl;
	root.validate();

	// Encoding html
	cout << "Encoding..." << endl;
	root.encode_to(output);

	cout << "Done !" << endl;
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
