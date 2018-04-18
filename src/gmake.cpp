#include <iostream>
#include <fstream>
#include <sstream>
#include <experimental/filesystem>

#include "../Graph/include/graph.h"
#include "../include/parser.h"

namespace fs = std::experimental::filesystem;

static const std::vector<std::string> EXTS_SRC = {".cpp", ".c++", ".cxx", ".cp", ".cc"};
static const std::string INCLUDE_STMT = "#include";
static const std::string INCLUDE_L_DLMTR = "\"";
static const std::string INCLUDE_R_DLMTR = "\"";

void eraseAll(std::string &str, const std::string& sub){
    int pos = 0;
    int length = sub.size();
    while((pos = str.find(sub))!=-1){
        str.replace(pos, length, "");
    }
}

bool startWith(const std::string& str, const std::string& sub){
    return str.find(sub)==0;
}

bool endWith(const std::string& str, const std::string& sub){
    return (str.size() >= sub.size() && str.compare(str.size() - sub.size(), sub.size(), sub) == 0);
}

template<typename T>
bool contains(const std::vector<T> &v, const T& value){
    return (std::find(v.begin(), v.end(), value) != v.end());
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

bool isSrc(GraphEntry const* entry){
    for(const auto& ext : EXTS_SRC){
        if(endWith(entry->mNode->getUid(), ext)){
            return true;
        }
    }
    return false;
}

bool readFileDependencies(const char* filename, std::vector<std::string> &deps){
    std::ifstream file(filename);
    if(file){
        std::string line;
        while(!file.eof()){
            getline(file, line);
            eraseAll(line, " ");
            if(startWith(line, INCLUDE_STMT)){
                int ld = line.find(INCLUDE_L_DLMTR);
                int rd = line.find(INCLUDE_R_DLMTR, ld+1);
                if(ld!=-1 && rd!=-1 && ld!=rd){
                    std::string dependency = line.substr(ld+1, rd-ld-1);
                    deps.push_back(dependency);
                }
            }
        }
        file.close();
        return true;
    }
    return false;
}

int findPath(const std::vector<fs::path> &paths, const std::string &dep){
    for(int i=0 ; i<paths.size() ; i++){
        if(paths[i].filename().string()==dep){
            return i;
        }
    }
    return -1;
}

std::vector<GraphEntry> filterGraph(const Graph &graph, bool (*filter)(GraphEntry const*)){
    std::map<std::string, GraphEntry> all = graph.getAllData();
    std::vector<GraphEntry> filtered;
    for(const auto& pair : all){
        if(filter(&pair.second)){
            filtered.push_back(pair.second);
        }
    }
    return filtered;
}

Graph getDependencyGraph(const std::string& directory){
    Graph graph;
    std::vector<fs::path> paths = listdir(directory);
    for(const auto& file : paths){
        std::vector<std::string> dependencies;
        std::string filepath = file.string();
        std::string extension = file.extension().string();
        if(contains(EXTS_SRC, extension) || contains(EXTS_HDR, extension)){
            graph.addNode(filepath);
            if(readFileDependencies(filepath.c_str(), dependencies)){
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

std::vector<GraphEntry> getDeepDependencies(const std::map<std::string, GraphEntry>& graph, const GraphEntry& entry){
    std::vector<GraphEntry> deps, tmp;
    std::vector<Edge*> out = entry.mOut;
    for(const auto& o : out){
        deps.push_back(graph.at(o->getEnd()->getUid()));
        tmp = getDeepDependencies(graph, graph.at(o->getEnd()->getUid()));
        deps.insert(deps.end(), tmp.begin(), tmp.end());
    }
    return deps;
}

std::stringstream generateMakefile(const Graph& graph, const std::string& exec){
    std::map<std::string, GraphEntry> all = graph.getAllData();
    std::vector<GraphEntry> sources = filterGraph(graph, isSrc);
    std::stringstream ss;

    // variables
    ss << "CC = g++" << std::endl;
    ss << "ODIR = obj" << std::endl;
    ss << "PROG = " << exec << std::endl;
    ss << "CXXFLAGS = -std=c++11" << std::endl;
    ss << std::endl;

    // executable
    ss << "OBJS = ";
    for(const auto& src : sources){
        ss << "$(ODIR)/" << fs::path(src.mNode->getUid()).stem().string() << ".o ";
    }
    ss << std::endl;
    ss << "$(PROG) : $(ODIR) $(OBJS)" << std::endl;
    ss << "\t$(CC) -o $@ $(OBJS) $(CXXFLAGS)";
    ss << std::endl << std::endl;

    // dependencies
    for(const auto& src : sources){
        std::string srcFilename = fs::path(src.mNode->getUid()).stem().string();
        ss << "$(ODIR)/" << srcFilename << ".o : " << src.mNode->getUid() << " ";

        // src deps
        std::vector<GraphEntry> deps = getDeepDependencies(all, src);
        for(const auto& d : deps){
            ss << d.mNode->getUid() << " ";
        }

        ss << std::endl;
        ss << "\t$(CC) -c $< -o $@ $(CXXFLAGS)" << std::endl << std::endl;
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
