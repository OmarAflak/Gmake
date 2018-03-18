#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>

#include <experimental/filesystem>
#include "../Graph/include/Graph.h"
#include "../include/parser.h"

namespace fs = std::experimental::filesystem;

void eraseAll(std::string &str, const std::string& sub){
    int pos = 0;
    int length = sub.size();
    while((pos = str.find(sub))!=-1){
        str.replace(pos, length, "");
    }
}

bool startsWith(const std::string& str, const std::string& sub){
    return str.find(sub)==0;
}

bool endsWith(const std::string& str, const std::string& sub){
    return (str.size() >= sub.size() && str.compare(str.size() - sub.size(), sub.size(), sub) == 0);
}

bool is_cpp(Node const* node){
    return endsWith(node->getName(), ".cpp");
}

bool readDeps(const char* filename, std::vector<std::string> &deps){
    std::ifstream file(filename);
    if(file){
        std::string include = "#include\"";
        int startPos = include.size();

        std::string line;
        while(!file.eof()){
            getline(file, line);
            eraseAll(line, " ");
            if(startsWith(line, include)){
                std::string dependency = line.substr(startPos, line.find("\"", startPos)-startPos);
                deps.push_back(dependency);
            }
        }
        file.close();
        return true;
    }
    return false;
}

std::vector<fs::path> listdir(const std::string& dir){
    fs::path p(dir);
    fs::directory_iterator end_itr;
    std::vector<fs::path> list;

    for (fs::directory_iterator itr(p); itr != end_itr; ++itr){
        if (is_regular_file(itr->path())) {
            list.push_back(itr->path());
        }
        else{
            std::vector<fs::path> subdir = listdir(itr->path().string());
            list.insert(list.end(), subdir.begin(), subdir.end());
        }
    }

    return list;
}

int findPath(const std::vector<fs::path> &paths, const std::string &dep){
    for(int i=0 ; i<paths.size() ; i++){
        if(paths[i].filename().string()==dep){
            return i;
        }
    }
    return -1;
}

Graph getDependencyGraph(const std::string& directory){
    Graph graph;
    std::vector<fs::path> paths = listdir(directory);
    for(const auto& file : paths){
        std::vector<std::string> dependencies;
        std::string filepath = file.string();
        std::string extension = file.extension().string();
        if(extension==".cpp" || extension==".h" || extension==".hpp"){
            graph.addNode(filepath);
            if(readDeps(filepath.c_str(), dependencies)){
                for(const auto& dep : dependencies){
                    fs::path p(dep);
                    int index = findPath(paths, p.filename().string());
                    if(index!=-1){
                        graph.connect(filepath, paths[index].string());
                    }
                }
            }
        }
    }
    return graph;
}

std::vector<Node*> filterGraph(const Graph &graph, bool (*filter)(Node const*)){
    std::vector<Node*> nodes = graph.getNodes();
    std::vector<Node*> array;
    for(const auto& node : nodes){
        if(filter(node)){
            array.push_back(node);
        }
    }
    return array;
}

Node* getHeader(const std::vector<Node*> &dependencies, const Node& cppNode){
    std::string name = fs::path(cppNode.getName()).stem().string();
    std::string nameH = name+".h";
    std::string nameHPP = name+".hpp";
    for(const auto& node : dependencies){
        std::string str = fs::path(node->getName()).filename().string();
        if(str==nameH || str==nameHPP){
            return node;
        }
    }
    return nullptr;
}

std::vector<Node*> getHeaderDependencies(const Graph& graph, const Node& header){
    std::vector<Node*> deps, hDeps, tmp;
    hDeps = graph.getOutConnections(header.getName());
    for(const auto& d : hDeps){
        deps.push_back(d);
        tmp = getHeaderDependencies(graph, *d);
        deps.insert(deps.end(), tmp.begin(), tmp.end());
    }
    return deps;
}

std::stringstream generateMakefile(const Graph& graph, const std::string& exec){
    std::vector<Node*> cpps = filterGraph(graph, is_cpp);
    std::vector<Node*> nodes = graph.getNodes();
    std::stringstream ss;

    // variables
    ss << "CC = g++" << std::endl;
    ss << "ODIR = obj" << std::endl;
    ss << "PROG = " << exec << std::endl;
    ss << "CXXFLAG = -std=c++11" << std::endl;
    ss << std::endl;

    // executable
    std::stringstream objs;
    for(const auto& cpp : cpps){
        objs << "$(ODIR)/" << fs::path(cpp->getName()).stem().string() << ".o ";
    }
    ss << "$(PROG) : $(ODIR) " << objs.str() << std::endl;
    ss << "\t$(CC) -o $@ " << objs.str() << "$(CXXFLAG)";
    ss << std::endl << std::endl;

    // dependencies
    for(const auto& cpp : cpps){
        std::vector<Node*> dependencies;
        std::string cpp_filename = fs::path(cpp->getName()).stem().string();
        ss << "$(ODIR)/" << cpp_filename << ".o : " << cpp->getName() << " ";

        // cpp deps
        dependencies = graph.getOutConnections(cpp->getName());
        for(const auto& dep : dependencies){
            ss << dep->getName() << " ";
        }

        // h deps
        Node* header = getHeader(dependencies, *cpp);
        if(header!=nullptr){
            std::vector<Node*> deps = getHeaderDependencies(graph, *header);
            for(const auto& d : deps){
                ss << d->getName() << " ";
            }
        }

        ss << std::endl;
        ss << "\t$(CC) -c $< -o $@ $(CXXFLAG)" << std::endl << std::endl;
    }

    // mkdir
    ss << "$(ODIR) :" << std::endl;
    ss << "\tif [ ! -d $(ODIR) ]; then mkdir $(ODIR); fi" << std::endl << std::endl;

    // clean
    ss << ".PHONY : clean" << std::endl;
    ss << "clean :" << std::endl;
    ss << "\tif [ -d $(ODIR) ]; then rm $(ODIR) -r; fi" << std::endl;
    ss << "\tif [ -f $(PROG) ]; then rm $(PROG); fi" << std::endl;

    return ss;
}

bool saveMakefile(const char* filepath, const std::stringstream& makefile){
    std::ofstream out(filepath);
    if(out){
        out << makefile.str() << std::endl;
        out.close();
        return true;
    }
    return false;
}

void help(){
    std::cout << "Usage: gmake [options]" << std::endl;
    std::cout << "  -x\texecutable name" << std::endl;
}

int main(int argc, char const *argv[]) {
    if(hasArg("-x", argc, argv)){
        std::string executable = getArg("-x", argc, argv);
        Graph graph = getDependencyGraph(".");
        std::stringstream makefile = generateMakefile(graph, executable);
        if(!saveMakefile("Makefile", makefile)){
            std::cerr << "Could not write Makefile." << std::endl;
            return 1;
        }
    }
    else{
        help();
    }

    return 0;
}
