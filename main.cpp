#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <boost/filesystem.hpp>

#include "Graph/include/Graph.h"

using namespace std;
using namespace boost::filesystem;

void rmSpaces(string &str){
    string result = "";
    for(int i=0 ; i<str.size() ; i++){
        if(str[i]!=' '){
            result += str[i];
        }
    }
    str = result;
}

bool startsWith(string str, string sub){
    if(str.size()<sub.size())
        return false;
    return str.substr(0, sub.size())==sub;
}

bool endsWith(string str, string sub){
    if(str.size()<sub.size())
        return false;
    return str.substr(str.size()-sub.size())==sub;
}

bool readDeps(const char* filename, vector<string> &deps){
    std::ifstream file(filename);
    if(file){
        string line;
        while(!file.eof()){
            getline(file, line);
            rmSpaces(line);
            if(startsWith(line, "#include\"")){
                string dependency = line.substr(line.find("#include\"")+9);
                dependency = dependency.substr(0, dependency.size()-1);
                deps.push_back(dependency);
            }
        }
        file.close();
        return true;
    }
    return false;
}

vector<path> listdir(string dir){
    path p(dir);
    directory_iterator end_itr;
    vector<path> list;

    for (directory_iterator itr(p); itr != end_itr; ++itr){
        if (is_regular_file(itr->path())) {
            list.push_back(itr->path());
        }
        else{
            vector<path> subdir = listdir(itr->path().string());
            list.insert(list.end(), subdir.begin(), subdir.end());
        }
    }

    return list;
}

int findPath(const vector<path> &files, string dep){
    for(int i=0 ; i<files.size() ; i++){
        if(files[i].filename().string()==dep){
            return i;
        }
    }
    return -1;
}

Graph getDependencyGraph(string directory){
    Graph graph;
    vector<path> files = listdir(directory);
    for(int i=0 ; i<files.size() ; i++){
        vector<string> dependencies;
        string filepath = files[i].string();
        string extension = files[i].extension().string();
        if(extension==".cpp" || extension==".h"){
            if(readDeps(filepath.c_str(), dependencies)){
                for(int i=0 ; i<dependencies.size() ; i++){
                    path p(dependencies[i]);
                    int index = findPath(files, p.filename().string());
                    if(index!=-1){
                        graph.connect(filepath, files[index].string());
                    }
                }
            }
        }
    }
    return graph;
}

vector<Node*> getOrderedDependencies(const Graph &graph){
    vector<Node*> nodes = graph.getNodes();
    vector<Node*> ordered;

    for(int i=0 ; i<nodes.size() ; i++){
        if(endsWith(nodes[i]->getName(), ".h")){
            ordered.push_back(nodes[i]);
        }
    }

    for(int i=0 ; i<ordered.size() ; i++){
        for(int j=i ; j<ordered.size() ; j++){
            if(ordered[i]->getOutConnections()>ordered[j]->getOutConnections()){
                Node* tmp = ordered[i];
                ordered[i] = ordered[j];
                ordered[j] = tmp;
            }
        }
    }

    return ordered;
}

stringstream generateMakefile(const Graph& graph, string entryPoint){
    path entryFilename = path(entryPoint).filename();
    vector<Node*> headers = getOrderedDependencies(graph);
    stringstream ss;

    // variables
    ss << "CC = g++" << endl;
    ss << "ODIR = obj" << endl;
    ss << "PROG = " << entryFilename.stem().string() << endl;
    ss << "CXXFLAG = " << endl;
    ss << endl;

    // executable
    ss << "$(PROG) : $(ODIR) $(ODIR)/$(PROG).o" << endl;
    ss << "\t$(CC) -o $@ $(ODIR)/$(PROG).o ";
    for(int i=0 ; i<headers.size() ; i++){
        ss << "$(ODIR)/" << path(headers[i]->getName()).stem().string() << ".o ";
    }
    ss << "$(CXXFLAG)" << endl << endl;

    // dependencies
    for(int i=0 ; i<headers.size() ; i++){
        Node* header = headers[i];
        std::string file = path(header->getName()).stem().string();
        string cppName = file + ".cpp";

        vector<Node*> all = graph.getNodes();
        for(int j=0 ; j<all.size() ; j++){
            if(path(all[j]->getName()).filename().string()==cppName){
                vector<Node*> deps = graph.getOutConnections(header->getName());
                ss << "$(ODIR)/" << file << ".o" << " : ";
                ss << all[j]->getName() << " " << header->getName() << " ";
                for(int k=0 ; k<deps.size() ; k++){
                    ss << "$(ODIR)/" << path(deps[k]->getName()).stem().string() << ".o ";
                }
                ss << endl;
                ss << "\t$(CC) -c $< -o $@" << endl << endl;
                break;
            }
        }
    }

    // entry point
    Node* entryNode = NULL;
    vector<Node*> allNodes = graph.getNodes();
    for(int i=0 ; i<allNodes.size() ; i++){
        if(path(allNodes[i]->getName()).filename()==entryFilename){
            entryNode = allNodes[i];
            break;
        }
    }
    vector<Node*> entryDeps = graph.getOutConnections(entryNode->getName());
    ss << "$(ODIR)/$(PROG).o : " << entryNode->getName() << " ";
    for(int i=0 ; i<entryDeps.size() ; i++){
        ss << "$(ODIR)/" << path(entryDeps[i]->getName()).stem().string() << ".o ";
    }
    ss << endl;
    ss << "\t$(CC) -c $< -o $@" << endl << endl;

    // mkdir
    ss << "$(ODIR) :" << endl;
    ss << "\tif [ ! -d $(ODIR) ]; then mkdir $(ODIR); fi" << endl << endl;

    // clean
    ss << ".PHONY: clean" << endl;
    ss << "clean :" << endl;
    ss << "\tif [ -d $(ODIR) ]; then rm $(ODIR) -r; fi" << endl;
    ss << "\tif [ -f $(PROG) ]; then rm $(PROG); fi" << endl;

    return ss;
}

int main(int argc, char const *argv[]) {
    string entry = argv[1];
    Graph graph = getDependencyGraph(".");
    stringstream makefile = generateMakefile(graph, entry);

    std::ofstream out("Makefile");
    out << makefile.str() << endl;
    out.close();

    return 0;
}
