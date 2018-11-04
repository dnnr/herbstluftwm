#include "rootcommands.h"

#include "ipc-protocol.h"
#include "root.h"
#include "command.h"
#include "attribute_.h"

#include <algorithm>
#include <map>
#include <functional>
#include <sstream>
#include <cstring>

using namespace std;

RootCommands::RootCommands(Root* root_) : root(root_) {
}

int RootCommands::get_attr_cmd(Input in, Output output) {
    in.shift();
    if (in.empty()) return HERBST_NEED_MORE_ARGS;

    Attribute* a = getAttribute(in.front(), output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    output << a->str();
    return 0;
}

int RootCommands::set_attr_cmd(Input in, Output output) {
    in.shift();
    if (in.empty()) return HERBST_NEED_MORE_ARGS;
    auto path = in.front();
    in.shift();
    if (in.empty()) return HERBST_NEED_MORE_ARGS;
    auto new_value = in.front();

    Attribute* a = getAttribute(path, output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    std::string error_message = a->change(new_value);
    if (error_message == "") {
        return 0;
    } else {
        output << in.command() << ": \""
            << in.front() << "\" is not a valid value for "
            << a->name() << ": "
            << error_message << std::endl;
        return HERBST_INVALID_ARGUMENT;
    }
}

int RootCommands::attr_cmd(Input in, Output output) {
    in.shift();

    auto path = in.empty() ? std::string("") : in.front();
    in.shift();
    std::ostringstream dummy_output;
    Object* o = root;
    auto p = Path::split(path);
    if (!p.empty()) {
        while (p.back().empty()) p.pop_back();
        o = o->child(p);
    }
    if (o && in.empty()) {
        o->ls(output);
        return 0;
    }

    Attribute* a = getAttribute(path, output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    if (in.empty()) {
        // no more arguments -> return the value
        output << a->str();
        return 0;
    } else {
        // another argument -> set the value
        std::string error_message = a->change(in.front());
        if (error_message == "") {
            return 0;
        } else {
            output << in.command() << ": \""
                << in.front() << "\" is not a valid value for "
                << a->name() << ": "
                << error_message << std::endl;
            return HERBST_INVALID_ARGUMENT;
        }
    }
}

Attribute* RootCommands::getAttribute(std::string path, Output output) {
    auto attr_path = Object::splitPath(path);
    auto child = root->child(attr_path.first);
    if (!child) {
        output << "No such object " << attr_path.first.join('.') << std::endl;
        return nullptr;
    }
    Attribute* a = child->attribute(attr_path.second);
    if (!a) {
        output << "Object " << attr_path.first.join('.')
               << " has no attribute \"" << attr_path.second << "\""
               << std::endl;
        return nullptr;
    }
    return a;
}

int RootCommands::print_object_tree_command(ArgList in, Output output) {
    in.shift();
    auto path = Path(in.empty() ? std::string("") : in.front()).toVector();
    while (!path.empty() && path.back() == "") {
        path.pop_back();
    }
    auto child = root->child(path);
    if (!child) {
        output << "No such object " << Path(path).join('.') << std::endl;
        return HERBST_INVALID_ARGUMENT;
    }
    child->printTree(output, Path(path).join('.'));
    return 0;
}


int RootCommands::substitute_cmd(Input input, Output output)
{
    string cmd, ident, path;
    if (!input.read({ &cmd, &ident, &path })) {
        return HERBST_NEED_MORE_ARGS;
    }
    if (input.empty()) return HERBST_NEED_MORE_ARGS;
    Attribute* a = getAttribute(path, output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    return Commands::call(input.replace(ident, a->str()), output);
}

int RootCommands::sprintf_cmd(Input input, Output output)
{
    string cmd, ident, format;
    if (!input.read({ &cmd, &ident, &format })) return HERBST_NEED_MORE_ARGS;
    string blobs;
    size_t lastpos = 0; // the position where the last plaintext blob started
    for (size_t i = 0; i < format.size(); i++) if (format[i] == '%') {
        if (i + 1 >= format.size()) {
            output
                << cmd << ": dangling % at the end of format \""
                << format << "\"" << endl;
            return HERBST_INVALID_ARGUMENT;
        } else {
            if (i > lastpos) {
                blobs += format.substr(lastpos, i - lastpos);
            }
            char format_type = format[i+1];
            lastpos = i + 2;
            i++; // also consume the format_type
            if (format_type == '%') {
                blobs += "%";
            } else if (format_type == 's') {
                string path;
                if (!input.read({ &path })) {
                    return HERBST_NEED_MORE_ARGS;
                }
                Attribute* a = getAttribute(path, output);
                if (!a) return HERBST_INVALID_ARGUMENT;
                blobs += a->str();
            } else {
                output
                    << cmd << ": invalid format type %"
                    << format_type << " at position "
                    << i << " in format string \""
                    << format << "\"" << endl;
                return HERBST_INVALID_ARGUMENT;
            }
        }
    }
    if (lastpos < format.size()) {
        blobs += format.substr(lastpos, format.size()-lastpos);
    }
    return Commands::call(input.replace(ident, blobs), output);
}


Attribute* RootCommands::newAttributeWithType(std::string typestr, std::string attr_name, Output output) {
    std::map<string, function<Attribute*(string)>> name2constructor {
    { "bool",  [](string n) { return new Attribute_<bool>(n, false); }},
    { "color", [](string n) { return new Attribute_<Color>(n, {"black"}); }},
    { "int",   [](string n) { return new Attribute_<int>(n, 0); }},
    { "string",[](string n) { return new Attribute_<string>(n, ""); }},
    { "uint",  [](string n) { return new Attribute_<unsigned long>(n, 0); }},
    };
    auto it = name2constructor.find(typestr);
    if (it == name2constructor.end()) {
        output << "error: unknown type \"" << typestr << "\"";
        return nullptr;
    }
    auto attr = it->second(attr_name);
    attr->setWriteable(true);
    return attr;
}

int RootCommands::new_attr_cmd(Input input, Output output)
{
    string cmd, type, path;
    if (!input.read({ &cmd, &type, &path })) {
        return HERBST_NEED_MORE_ARGS;
    }
    auto obj_path_and_attr = Object::splitPath(path);
    string attr_name = obj_path_and_attr.second;
    Object* obj = root->child(obj_path_and_attr.first, output);
    if (!obj) return HERBST_INVALID_ARGUMENT;
    if (attr_name.substr(0,strlen(USER_ATTRIBUTE_PREFIX)) != USER_ATTRIBUTE_PREFIX) {
        output
            << cmd << ": attribute name must start with \""
            << USER_ATTRIBUTE_PREFIX << "\""
            << " but is actually \"" << attr_name << "\"" << endl;
        return HERBST_INVALID_ARGUMENT;
    }
    if (obj->attribute(attr_name)) {
        output
            << cmd << ": object \"" << obj_path_and_attr.first.join()
            << "\" already has an attribute named \"" << attr_name
            <<  "\"" << endl;
        return HERBST_INVALID_ARGUMENT;
    }
    // create the new attribute and add it
    Attribute* a = newAttributeWithType(type, attr_name, output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    obj->addAttribute(a);
    return 0;
}

int RootCommands::remove_attr_cmd(Input input, Output output)
{
    string cmd, path;
    if (!input.read({ &cmd, &path })) {
        return HERBST_NEED_MORE_ARGS;
    }
    Attribute* a = root->deepAttribute(path, output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    if (a->name().substr(0,strlen(USER_ATTRIBUTE_PREFIX)) != USER_ATTRIBUTE_PREFIX) {
        output << cmd << ": \"" << path
               << "\" is not a user defined attribute. can not remove it." << endl;
        return HERBST_INVALID_ARGUMENT;
    }
    a->detachFromOwner();
    delete a;
    return 0;
}

template <typename T> int do_comparison(const T& a, const T& b) {
    return (a == b) ? 0 : 1;
}

template <> int do_comparison<int>(const int& a, const int& b) {
    if (a < b) return -1;
    else if (a > b) return 1;
    else return 0;
}
template <> int do_comparison<unsigned long>(const unsigned long& a, const unsigned long& b) {
    if (a < b) return -1;
    else if (a > b) return 1;
    else return 0;
}

template <typename T> int parse_and_compare(string a, string b, Output o) {
    vector<string> strings = { a, b };
    vector<T> vals;
    for (auto & x : strings) {
        try {
            vals.push_back(Attribute_<T>::parse(x, nullptr));
        } catch(std::exception& e) {
            o << "can not parse \"" << x << "\" to "
              << typeid(T).name() << ": " << e.what() << endl;
            return (int) HERBST_INVALID_ARGUMENT;
        }
    }
    return do_comparison<T>(vals[0], vals[1]);
}

int RootCommands::compare_cmd(Input input, Output output)
{
    string cmd, path, oper, value;
    if (!input.read({ &cmd, &path, &oper, &value })) {
        return HERBST_NEED_MORE_ARGS;
    }
    Attribute* a = root->deepAttribute(path, output);
    if (!a) return HERBST_INVALID_ARGUMENT;
    // the following compare functions returns
    //   -1 if the first value is smaller
    //    1 if the first value is greater
    //    0 if the the values match
    //    HERBST_INVALID_ARGUMENT if there was a parsing error
    map<string, pair<bool, vector<int> > > operators {
        // map operator names to "for numeric types only" and possible return codes
        { "=",  { false, { 0 }, }, },
        { "!=", { false, { -1, 1 } }, },
        { "ge", { true, { 1, 0 } }, },
        { "gt", { true, { 1    } }, },
        { "le", { true, { -1, 0 } }, },
        { "lt", { true, { -1    } }, },
    };
    map<Type, pair<bool, function<int(string,string,Output)>>> type2compare {
        // map a type name to "is it numeric" and a comperator function
        { Type::ATTRIBUTE_INT,      { true,  parse_and_compare<int> }, },
        { Type::ATTRIBUTE_ULONG,    { true,  parse_and_compare<int> }, },
        { Type::ATTRIBUTE_STRING,   { false, parse_and_compare<string> }, },
        { Type::ATTRIBUTE_BOOL,     { false, parse_and_compare<bool> }, },
        { Type::ATTRIBUTE_COLOR,    { false, parse_and_compare<Color> }, },
    };
    auto it = type2compare.find(a->type());
    if (it == type2compare.end()) {
        output << "attribute " << path << " has unknown type" << endl;
        return HERBST_INVALID_ARGUMENT;
    }
    auto op_it = operators.find(oper);
    if (op_it == operators.end()) {
        output << "unknown operator \"" << oper
            << "\". Possible values are:";
        for (auto i : operators) {
            // only list operators suitable for the attribute type
            if (!it->second.first && i.second.first) continue;
            output << " " << i.first;
        }
        output << endl;
    }
    if (op_it->second.first && !it->second.first) {
        output << "operator \"" << oper << "\" "
            << "only allowed for numeric types, but the attribute "
            << path << " is of non-numeric type "
            << Entity::typestr(a->type()) << endl;
        return HERBST_INVALID_ARGUMENT;
    }
    int comparison_result = it->second.second(a->str(), value, output);
    if (comparison_result > 1) return comparison_result;
    vector<int>& possible_values = op_it->second.second;
    auto found = std::find(possible_values.begin(),
                           possible_values.end(),
                           comparison_result);
    return (found == possible_values.end()) ? 1 : 0;
}
