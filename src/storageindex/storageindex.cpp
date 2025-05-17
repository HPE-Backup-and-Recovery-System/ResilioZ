#include "storageindex.h"
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void StorageIndex::insert(const std::string& key, const std::string& value) {
    if (!root) {
        root = new Node(true);
    }
    insertInternal(root, key, value);
}

void StorageIndex::insertInternal(Node* node, const std::string& key, const std::string& value) {
    if (node->isLeaf) {
        node->data[key].push_back(value);
    } else {
        for (Node* child : node->children) {
            insertInternal(child, key, value);
        }
    }
}

std::vector<std::string> StorageIndex::search(const std::string& key) {
    if (!root) return {};
    Node* current = root;
    while (!current->isLeaf) {
        for (Node* child : current->children) {
            current = child;
        }
    }
    return current->data[key];
}

void StorageIndex::scanForStorages() {
    std::cout << "Scanning for storage devices..." << std::endl;

    for (const auto& entry : fs::directory_iterator("/mnt")) {
        if (fs::is_directory(entry.path())) {
            std::string path = entry.path().string();
            std::cout << "Storage detected: " << path << std::endl;
            insert("storage", path);
        }
    }

    for (const auto& entry : fs::directory_iterator("/media")) {
        if (fs::is_directory(entry.path())) {
            std::string path = entry.path().string();
            std::cout << "Storage detected: " << path << std::endl;
            insert("storage", path);
        }
    }
}