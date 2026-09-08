// c++ bundle.cpp -o bundle.out && ./bundle.out a/a.js a/y.js

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

string base64 = "$0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

string resolve (string f, string file) {
    if (f.rfind("./", 0) == 0) {
        f = f.substr(2);
    }
    char i = f.front();
    if (i != '.' && i != '/') {
        f = file.substr(0, file.find_last_of('/')) + '/' + f;
    } else if (f.rfind("../", 0) == 0) {
        while (f.rfind("../", 0) == 0) {
            f = f.substr(3);
            file = file.substr(0, file.find_last_of('/'));
        }
        f = file.substr(0, file.find_last_of('/')) + '/' + f;
    }
    if (f.substr(f.length() - 3) != ".js") {
        f += ".js";
    }
    return f;
}

string substitute (string & text, string & past, string next) {
    int a = 0;
    int i = past.length();
    int j = next.length();
    while ((a = text.find(past, a)) != -1) {
        if (text.length() < a + i + 1) {
            return text;
        }
        if (base64.find(text[a + i]) != -1 || (a != 0 && (base64 + "\"'.").find(text[a - 1]) != -1)) {
            a += i;
            continue;
        }
        text = text.substr(0, a) + next + text.substr(a + i);
        a += j;
    }
    return text;
}

void parse (string & file, map<string, vector<string>> & modules, map<string, string> & texts) {
    string text;
    ifstream f(file);
    if (f.is_open()) {
        stringstream t;
        t << f.rdbuf();
        text = t.str();
        f.close();
    } else {
        texts[file] = "";
        return;
    }
    int i, j;
    while ((i = text.find(" \n")) != -1) {
        text.replace(i, 2, "\n");
    }
    while ((i = text.find("\n\n")) != -1) {
        text.replace(i, 2, "\n");
    }
    vector<string> lines = {};
    stringstream s(text);
    for (string line; getline(s, line, '\n');) {
        lines.push_back(line);
    }
    bool remove = false;
    text = "";
    for (string line : lines) {
        if (line.substr(line.find_first_not_of("\t "), 2) == "//") {
            continue;
        }
        if (!remove && (i = line.find("/*")) != -1 && line.find("//*") == -1) {
            if ((j = line.find("*/")) != -1) {
                line = line.substr(0, i) + ' ' + line.substr(j + 2);
            } else {
                line = line.substr(0, i);
                remove = true;
            }
        }
        if (remove) {
            if ((i = line.find("*/")) != -1) {
                line = line.substr(i + 2);
                remove = false;
            } else {
                continue;
            }
        }
        line = line.substr(0, line.find_last_not_of("\t ") + 1);
        if (!line.empty()) {
            text += line + "\n";
        }
    }
    map<string, vector<string>> files = {{file, {}}};
    vector<string> order = {};
    string texta = text;
    while ((i = text.find("import ")) != -1) {
        if (i != 0) {
            char j = text[i - 1];
            if (j != '\t' && j != '\n' && j != ' ') {
                text = text.substr(i + 6);
                continue;
            }
        }
        i += 6;
        while (text[i] == ' ') {
            i++;
        }
        text = text.substr(i);
        i = text.find("from");
        j = text.find('"');
        int k = text.find("'");
        vector<string> names = {};
        if (i != -1 && (i < j || j == -1) && (i < k || k == -1)) {
            while (i < text.length()) {
                char j = text[i - 1];
                char k = text[i + 4];
                if ((j == ' ' || j == '}') && (k == ' ' || k == '"' || k == '\'')) {
                    break;
                }
                i += 4;
                i += text.substr(i).find("from");
            }
            j = 0;
            string t = text.substr(0, i);
            while ((k = t.find_first_of(" ,{}", j)) != -1) {
                if (j < k) {
                    names.push_back(t.substr(j, k - j));
                }
                j = k + 1;
            }
            if (j < t.length()) {
                names.push_back(t.substr(j));
            }
            i += 5;
            while (text[i] == ' ') {
                i++;
            }
        } else {
            i = 0;
        }
        string f = string(1, text[i]);
        if (f == "\"" || f == "'") {
            text = text.substr(i + 1);
            i = text.find(f);
            f = resolve(text.substr(0, i), file);
            if (files.find(f) == files.end()) {
                files[f] = {};
                order.push_back(f);
            }
            files[f].insert(files[f].end(), names.begin(), names.end());
        }
    }
    vector<string> mods = {};
    modules[file] = {};
    for (string & f : order) {
        if (texts.find(f) == texts.end()) {
            mods.push_back(f);
            if (modules.find(f) == modules.end()) {
                modules[file] = mods;
                return;
            }
        }
    }
    vector<string> declares = {"async", "class", "const", "default", "function", "let", "var"};
    vector<char> defines = {'\n', ' ', '(', ',', '.', '['};
    text = texta;
    while ((i = text.find("export ")) != -1) {
        text = text.substr(i + 7);
        for (string & name : declares) {
            i = text.find(name);
            if (i != -1 && i < 3) {
                text = text.substr(i + name.length());
            }
        }
        string names = "";
        if ((i = text.find('\n')) != -1) {
            names = text.substr(0, i);
        }
        i = 0;
        while (i < names.length() && names[i] == ' ') {
            i++;
        }
        vector<string> split = {};
        if (i < names.length() && names[i] == '{') {
            names = names.substr(i + 1);
            stringstream t(names.substr(0, names.find('}')));
            for (string name; getline(t, name, ',');) {
                split.push_back(name);
            }
        } else {
            i = names.find('(');
            j = names.find('=');
            if (j == -1 || (i < j && i != -1)) {
                split.push_back(names);
            } else {
                while (j != -1 && names[j + 1] != '>') {
                    split.push_back(names.substr(0, j));
                    names = names.substr(j);
                    if ((j = names.find(',')) == -1) {
                        break;
                    }
                    names = names.substr(j);
                    j = names.find('=');
                }
            }
        }
        for (string & name : split) {
            while (find(defines.begin(), defines.end(), name.front()) != defines.end()) {
                name = name.substr(1);
            }
            for (char & i : defines) {
                if ((j = name.find(i)) != -1) {
                    name = name.substr(0, j);
                }
            }
            files[file].push_back(name);
        }
        i = text.find("export ");
    }
    text = texta;
    for (map<string, vector<string>>::iterator pair = files.begin(); pair != files.end(); pair++) {
        string f = pair->first;
        string path = f.substr(0, f.length() - 3);
        for (char & i : path) {
            if (base64.find(i) == -1) {
                i = '_';
            }
        }
        for (string & name : pair->second) {
            text = substitute(text, name, name + '_' + path);
        }
    }
    lines = {};
    stringstream t(text);
    for (string line; getline(t, line, '\n');) {
        lines.push_back(line);
    }
    text = "";
    for (string & line : lines) {
        string a = line.substr(line.find_first_not_of("\t "));
        if (a.rfind("export default ", 0) == 0) {
            line = a.substr(15);
        } else if (a.rfind("export ", 0) == 0) {
            line = a.substr(7);
            a = line.substr(line.find_first_not_of("\t "));
            if (a.front() == '{') {
                continue;
            }
        }
        if (!line.empty() && a.rfind("import ", 0) != 0) {
            text += line + "\n";
        }
    }
    texts[file] = text;
}

void build (string file, string output) {
    vector<string> imported = {};
    vector<string> imports = {file};
    map<string, vector<string>> modules = {};
    map<string, string> texts = {};
    while (!imports.empty()) {
        file = imports[0];
        if (find(imported.begin(), imported.end(), file) != imported.end()) {
            imports.erase(imports.begin());
        } else {
            parse(file, modules, texts);
            if (modules.find(file) != modules.end()) {
                vector<string> & mods = modules[file];
                imports.insert(imports.begin(), mods.begin(), mods.end());
            }
            if (texts.find(file) != texts.end()) {
                imported.push_back(file);
            }
        }
    }
    string text = "";
    for (string & file : imported) {
        text += texts[file];
    }
    ofstream f(output);
    if (f.is_open()) {
        f << text;
        f.close();
    }
}

int main (int argc, char * argv[]) {
    build(argc > 1 ? argv[1] : "a/a.js", argc > 2 ? argv[2] : "a/y.js");
    return 0;
}