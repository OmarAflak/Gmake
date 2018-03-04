#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>

#include <boost/filesystem.hpp>
#include "../Graph/include/Graph.h"

using namespace boost::filesystem;

struct less_connections{
    inline bool operator() (Node const* node1, Node const* node2){
        return (node1->getOutConnections() < node2->getOutConnections());
    }
};


void eraseAll(std::string &str, const std::string& sub){
    int pos = 0;
    int length = sub.size();
    while((pos = str.find(sub))!=-1){
        str.replace(pos, length, "");
    }
}

bool startsWith(const std::string& str, const std::string& sub){
    if(str.size()<sub.size())
        return false;
    return str.substr(0, sub.size())==sub;
}

bool endsWith(const std::string& str, const std::string& sub){
    if(str.size()<sub.size())
        return false;
    return str.substr(str.size()-sub.size())==sub;
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
                std::string dependency = line.substr(startPos, line.find("\"", startPos));
                dependency = dependency.substr(0, dependency.size()-1);
                deps.push_back(dependency);
            }
        }
        file.close();
        return true;
    }
    return false;
}

std::vector<path> listdir(const std::string& dir){
    path p(dir);
    directory_iterator end_itr;
    std::vector<path> list;

    for (directory_iterator itr(p); itr != end_itr; ++itr){
        if (is_regular_file(itr->path())) {
            list.push_back(itr->path());
        }
        else{
            std::vector<path> subdir = listdir(itr->path().string());
            list.insert(list.end(), subdir.begin(), subdir.end());
        }
    }

    return list;
}

int findPath(const std::vector<path> &paths, const std::string &dep){
    for(int i=0 ; i<paths.size() ; i++){
        if(paths[i].filename().string()==dep){
            return i;
        }
    }
    return -1;
}

Graph getDependencyGraph(const std::string& directory){
    Graph graph;
    std::vector<path> paths = listdir(directory);
    for(const auto& file : paths){
        std::vector<std::string> dependencies;
        std::string filepath = file.string();
        std::string extension = file.extension().string();
        if(extension==".cpp" || extension==".h"){
            if(readDeps(filepath.c_str(), dependencies)){
                for(const auto& dep : dependencies){
                    path p(dep);
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

std::vector<Node*> getOrderedDependencies(const Graph &graph){
    std::vector<Node*> nodes = graph.getNodes();
    std::vector<Node*> headers;

    for(const auto& node : nodes){
        if(endsWith(node->getName(), ".h")){
            headers.push_back(node);
        }
    }

    std::sort(headers.begin(), headers.end(), less_connections());
    return headers;
}

std::stringstream generateMakefile(const Graph& graph, const std::string& entryPoint){
    path entryFilename = path(entryPoint).filename();
    std::vector<Node*> headers = getOrderedDependencies(graph);
    std::vector<Node*> nodes = graph.getNodes();
    std::stringstream ss;

    // variables
    ss << "CC = g++" << std::endl;
    ss << "ODIR = obj" << std::endl;
    ss << "PROG = " << entryFilename.stem().string() << std::endl;
    ss << "CXXFLAG = " << std::endl;
    ss << std::endl;

    // executable
    ss << "$(PROG) : $(ODIR) $(ODIR)/$(PROG).o" << std::endl;
    ss << "\t$(CC) -o $@ $(ODIR)/$(PROG).o ";
    for(const auto& header : headers){
        ss << "$(ODIR)/" << path(header->getName()).stem().string() << ".o ";
    }
    ss << "$(CXXFLAG)" << std::endl << std::endl;

    // dependencies
    for(const auto& header : headers){
        std::string file = path(header->getName()).stem().string();
        std::string cppName = file + ".cpp";

        for(const auto& node : nodes){
            if(path(node->getName()).filename().string()==cppName){
                std::vector<Node*> deps = graph.getOutConnections(header->getName());
                ss << "$(ODIR)/" << file << ".o" << " : ";
                ss << node->getName() << " " << header->getName() << " ";
                for(const auto& dependency : deps){
                    ss << "$(ODIR)/" << path(dependency->getName()).stem().string() << ".o ";
                }
                ss << std::endl;
                ss << "\t$(CC) -c $< -o $@" << std::endl << std::endl;
                break;
            }
        }
    }

    // entry point
    Node* entryNode = NULL;
    for(const auto& node : nodes){
        if(endsWith(node->getName(), entryPoint)){
            entryNode = node;
            break;
        }
    }
    std::vector<Node*> entryDeps = graph.getOutConnections(entryNode->getName());
    ss << "$(ODIR)/$(PROG).o : " << entryNode->getName() << " ";
    for(const auto& dep : entryDeps){
        ss << "$(ODIR)/" << path(dep->getName()).stem().string() << ".o ";
    }
    ss << std::endl;
    ss << "\t$(CC) -c $< -o $@" << std::endl << std::endl;

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

void help(){
    std::cout << "gmake [entry]" << std::endl;
    std::cout << "\t[entry] : file containing the main() function" << std::endl;
}

int main(int argc, char const *argv[]) {
    if(argc==2){
        std::string entry = argv[1];
        Graph graph = getDependencyGraph(".");
        std::stringstream makefile = generateMakefile(graph, entry);

        std::ofstream out("Makefile");
        out << makefile.str() << std::endl;
        out.close();
    }
    else{
        help();
    }

    return 0;
}
