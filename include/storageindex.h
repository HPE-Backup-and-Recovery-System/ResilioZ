#ifndef STORAGEINDEX_H
#define STORAGEINDEX_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>

class StorageIndex {
public:
    void insert(const std::string& key, const std::string& value);  
    std::vector<std::string> search(const std::string& key);         
    void scanForStorages();                                         

private:
    struct Node {
        bool isLeaf;
        std::map<std::string, std::vector<std::string>> data;
        std::vector<Node*> children;
        Node(bool leaf) : isLeaf(leaf) {}
    };

    Node* root = nullptr;  
    void insertInternal(Node* node, const std::string& key, const std::string& value);
};

#endif