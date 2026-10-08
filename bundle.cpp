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
    char i = f[0];
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

string substitute (string & match, string next, string & text) {
    int a = text.find(match);
    int i = match.length();
    int j = next.length();
    while (a != -1) {
        if (base64.find(text[a + i]) != -1 || (a != 0 && (base64 + "\"'.").find(text[a - 1]) != -1)) {
            a = text.find(match, a + i);
        } else {
            text = text.substr(0, a) + next + text.substr(a + i);
            a = text.find(match, a + j);
        }
    }
    return text;
}

void parse (string & file, map<string, vector<string> > & modules, map<string, string> & texts) {
    string text;
    ifstream f(file.c_str());
    if (f.is_open()) {
        stringstream t;
        t << f.rdbuf();
        text = t.str();
        f.close();
    } else {
        texts[file] = "";
        return;
    }
    stringstream s(text);
    bool remove = false;
    text = "";
    int i, j, k;
    for (string line; getline(s, line, '\n');) {
        if ((i = line.find_first_not_of("\t ")) != -1 && line.substr(i, 2) == "//") {
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
    map<string, string> defaults;
    map<string, map<string, string> > exports;
    exports[file];
    vector<string> order;
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
        string defaulted = "";
        map<string, string> names;
        text = text.substr(i);
        i = text.find("from");
        j = text.find('"');
        k = text.find("'");
        if (i != -1 && (i < j || j == -1) && (i < k || k == -1)) {
            int length = text.length();
            while (i < length) {
                char j = text[i - 1];
                char k = text[i + 4];
                if ((j == ' ' || j == '}') && (k == ' ' || k == '"' || k == '\'')) {
                    break;
                }
                i += 4;
                i += text.substr(i).find("from");
            }
            string variables = text.substr(0, i);
            if ((j = variables.find('{')) != -1) {
                k = variables.find('}');
                istringstream stream(variables.substr(j + 1, k - j - 1));
                variables = variables.substr(0, j);
                string name;
                while (getline(stream, name, ',')) {
                    if ((j = name.find_first_not_of("\t ")) != -1) {
                        name = name.substr(j);
                    }
                    name = name.substr(0, name.find_last_not_of("\t ") + 1);
                    if ((j = name.find(" as ")) != -1) {
                        names[name.substr(0, j)] = name.substr(j + 4);
                    } else if (!name.empty()) {
                        names[name] = name;
                    }
                }
            }
            string name = variables.substr(0, variables.find(','));
            if ((j = name.find_first_not_of("\t ")) != -1) {
                name = name.substr(j);
            }
            name = name.substr(0, name.find_last_not_of("\t ") + 1);
            if (!name.empty()) {
                defaulted = name;
                names[name] = name;
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
            if (exports.find(f) == exports.end()) {
                exports[f];
                order.push_back(f);
            }
            if (!defaulted.empty()) {
                defaults[f] = defaulted;
            }
            for (map<string, string>::iterator pair = names.begin(); pair != names.end(); pair++) {
                exports[f][pair->first] = pair->second;
            }
        }
    }
    bool dependencies = false;
    vector<string> mods;
    for (int i = 0, length = order.size(); i < length; i++) {
        string f = order[i];
        if (texts.find(f) == texts.end()) {
            mods.push_back(f);
            if (modules.find(f) == modules.end()) {
                dependencies = true;
            }
        }
    }
    if (dependencies) {
        modules[file] = mods;
        return;
    }
    string declares[] = {"async", "class", "const", "default", "function", "let", "var"};
    string defines = "\n (,.[";
    text = texta;
    while ((i = text.find("export ")) != -1) {
        text = text.substr(i + 7);
        if (text.find("default ") == 0) {
            continue;
        }
        for (int i = 0, length = sizeof(declares) / sizeof(declares[0]); i < length; i++) {
            string name = declares[i];
            j = text.find(name);
            if (j != -1 && j < 3) {
                text = text.substr(j + name.length());
            }
        }
        if ((i = text.find('\n')) != -1) {
            string variables = text.substr(0, i);
            i = 0;
            int length = variables.length();
            while (i < length && variables[i] == ' ') {
                i++;
            }
            vector<string> split;
            if (i < length && variables[i] == '{') {
                variables = variables.substr(i + 1);
                stringstream t(variables.substr(0, variables.find('}')));
                for (string name; getline(t, name, ',');) {
                    split.push_back(name);
                }
            } else {
                i = variables.find('(');
                j = variables.find('=');
                if (j == -1 || (i < j && i != -1)) {
                    split.push_back(variables);
                } else {
                    while (j != -1 && variables[j + 1] != '>') {
                        split.push_back(variables.substr(0, j));
                        variables = variables.substr(j);
                        if ((j = variables.find(',')) == -1) {
                            break;
                        }
                        variables = variables.substr(j);
                        j = variables.find('=');
                    }
                }
            }
            for (int i = 0, len = split.size(); i < len; i++) {
                string name = split[i];
                while (defines.find(name[0]) != -1) {
                    name = name.substr(1);
                }
                for (int j = 0, length = defines.length(); j < length; j++) {
                    if ((k = name.find(defines[j])) != -1) {
                        name = name.substr(0, k);
                    }
                }
                exports[file][name] = name;
            }
        }
        i = text.find("export ");
    }
    string defaulted = "";
    text = texta;
    for (map<string, map<string, string> >::iterator pair = exports.begin(); pair != exports.end(); pair++) {
        string f = pair->first;
        string path = f.substr(0, f.length() - 3);
        for (int j = 0, length = path.length(); j < length; j++) {
            if (base64.find(path[j]) == -1) {
                path[j] = '_';
            }
        }
        if (f == file) {
            defaulted = path;
        }
        map<string, string> exported = pair->second;
        for (map<string, string>::iterator value = exported.begin(); value != exported.end(); value++) {
            string named = value->first;
            string name = value->second;
            if (defaults.find(f) != defaults.end() && defaults[f] == name) {
                text = substitute(name, '_' + path, text);
            } else {
                text = substitute(name, named + '_' + path, text);
            }
        }
    }
    stringstream t(text);
    text = "";
    for (string line; getline(t, line, '\n');) {
        string a = line.substr(line.find_first_not_of("\t "));
        if (a.rfind("export default ", 0) == 0) {
            line = '_' + defaulted + " = " + a.substr(15);
        } else if (a.rfind("export ", 0) == 0) {
            line = a.substr(7);
            a = line.substr(line.find_first_not_of("\t "));
            if (a[0] == '{') {
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
    vector<string> imported;
    vector<string> imports;
    imports.push_back(file);
    map<string, vector<string> > modules;
    map<string, string> texts;
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
                imports.erase(imports.begin());
            }
        }
    }
    string text = "";
    for (int i = 0, length = imported.size(); i < length; i++) {
        text += texts[imported[i]];
    }
    ofstream f(output.c_str());
    if (f.is_open()) {
        f << text;
        f.close();
    }
}

int main (int argc, char * argv[]) {
    build(argc > 1 ? argv[1] : "a/a.js", argc > 2 ? argv[2] : "a/y.js");
    return 0;
}