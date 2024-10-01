//
// Created by kosakseb on 9/30/24.
//
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <cassert>
#include <queue>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>

/*
 * we first need to think about the architecture:
 * - we need a normal minheap for the beginning
 * - but we need a special structure for the huffman tree
 *
 * - for the overall tree it is not of relevance if the node has a substructure or not
 *  - both node types need a weight (for minheap)
 *  - the tree operates on abstract node class
 *  - tree gets a list of nodes for init
 *    - has reheapify operation
 *    - has insert operation on nodes
 *
 * - each node is a huffman minheap node
 *  - we have a huffman tree
 *    - minheap (supporting the basic instructions for minheap)
 *    - special operations for huffman tree
 *       - merge two nodes
 *       - assign the bitmaps
 *
 * - huffman minheap node:
 *  - special insert operation
 *    - the smaller weight gets left the other one right (no re heapify)
 *
 */

#ifndef HUFFMAN_MINHEAP_HPP
#define HUFFMAN_MINHEAP_HPP

#define LOOKUP_TABLE_SIZE 13

class HuffmanEncoder {
  struct Node {
    size_t weight;
    uint8_t content;
    Node* left;
    Node* right;

    static std::unique_ptr<Node> merge(Node* node1, Node* node2) {
        return std::make_unique<Node>(node1->weight + node2->weight, 255, node1, node2);
    };
  };

// Function to generate DOT representation of the tree
  void generateDot(const Node* root, std::ofstream& file, int& idCounter) {
    if (root == nullptr) {
      return;
    }

    // Create a unique ID for the current node
    int currentId = idCounter++;

    // Create a string for the current node
    std::stringstream nodeString;
    nodeString << currentId << " [label=\"" << root->weight << " " << static_cast<char>(root->content) << "\"];\n";
    file << nodeString.str();

    // If node has left child
    if (root->left != nullptr) {
      file << "    " << currentId << " -> " << idCounter << ";\n";
      generateDot(root->left, file, idCounter);
    }

    // If node has right child
    if (root->right != nullptr) {
      file << "    " << currentId << " -> " << idCounter << ";\n";
      generateDot(root->right, file, idCounter);
    }
  }

// Function to create a DOT file
  void createDotFile(const Node* root, const std::string& filename) {
    std::ofstream file(filename);
    file << "digraph G {\n";
    file << "    node [shape=circle];\n"; // Optional: shape for nodes

    int idCounter = 0; // Initialize ID counter
    generateDot(root, file, idCounter);

    file << "}\n";
    file.close();
  }


  struct HuffmanCodeItem {
    uint16_t encoded_bits;
    uint8_t used_bits;
  };

public:
  std::map<char, HuffmanCodeItem> huffmanCodes;

  HuffmanEncoder(std::vector<std::string> contents) {
    auto frequencies = getFrequencies(contents);
    huffmanCodes = getHuffmanCodes(frequencies);
  }

private:
  std::map<char, size_t> getFrequencies(std::vector<std::string> contents) {
    std::map<char, size_t> frequencies;

    for (auto& content: contents) {
      for (char c: content) {
        frequencies[c] = (frequencies.contains(c) ? frequencies.at(c) : 0) + 1;
      }
    }

    frequencies['\0'] = contents.size();

    return std::move(frequencies);
  }

  std::map<char, HuffmanCodeItem> getHuffmanCodes(std::map<char, size_t> frequencies) {
    std::map<char, HuffmanCodeItem> huffman_table;
    std::vector<std::unique_ptr<Node>> nodes;

    auto cmp = [](Node* left, Node* right) { return left->weight > right->weight; };
    std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> huffman_tree(cmp);

    for (auto const& [key, val] : frequencies)
    {
      nodes.push_back(std::make_unique<Node>(val, static_cast<uint8_t>(key), nullptr, nullptr));
      huffman_tree.push(nodes.back().get());
    }

    while (huffman_tree.size() > 1) {
      auto node1 = huffman_tree.top();
      huffman_tree.pop();
      auto node2 = huffman_tree.top();
      huffman_tree.pop();

      auto innerNode = Node::merge(node1, node2);

      nodes.push_back(std::move(innerNode));
      huffman_tree.push(nodes.back().get());
    }

    createDotFile(huffman_tree.top(), "./test.dot");

    // now convert the tree to the correct structure
    struct QueueElement {
      Node* node;
      uint16_t current_code;
      uint8_t level;
    };

    // implement bfs for this
    std::queue<QueueElement> queue;
    queue.push({huffman_tree.top(), 0, 0});
    while (queue.size()) {
      Node* current_node = queue.front().node;
      uint8_t level = queue.front().level;
      uint16_t current_code = queue.front().current_code;

      queue.pop();

      if (current_node->content < 255) {
        // inner node
        huffman_table[static_cast<char>(current_node->content)] = {static_cast<uint16_t>(current_code >> 1), level};
      } else {
        queue.push({current_node->left, static_cast<uint16_t>(current_code << 1), static_cast<uint8_t>(level + 1)});
        queue.push({current_node->right, static_cast<uint16_t>((current_code + 1) << 1), static_cast<uint8_t>(level + 1)});
      }
    }

    return std::move(huffman_table);
  }

  struct EncodedStrings {
    std::vector<uint64_t> encodedContent;
    std::vector<size_t> bounds;
  };

public:
  EncodedStrings encode(std::vector<std::string> content) {
    size_t current_idx = 0;
    std::vector<size_t> bounds;

    std::vector<uint64_t> buffer;

    for (auto& st : content) {
      st += '\0';
      uint64_t current = 0;
      uint8_t leftBits = 64;

      for (char c : st) {
        if (leftBits == 0) {
          buffer.push_back(current);
          leftBits = 64;
          current = 0;
        }

        auto& huff = huffmanCodes[c];
        if (leftBits >= huff.used_bits) {
          current <<= huff.used_bits;
          current += huff.encoded_bits;
          leftBits -=  huff.used_bits;
        } else {
          current <<= leftBits;
          current += huff.encoded_bits >> (huff.used_bits - leftBits);

          buffer.push_back(current);
          current = 0;

          current += huff.encoded_bits;
          current &= (1 << (huff.used_bits - leftBits)) - 1;

          leftBits = 64 - (huff.used_bits - leftBits);
        }
      }

      current <<= leftBits;
      buffer.push_back(current);
      bounds.push_back(buffer.size() - current_idx);
      current_idx = buffer.size();
    }

    return {buffer, bounds};
  }

  struct LookupItem {
    char code;
    uint8_t used_bits;
  };

  static std::array<LookupItem, 1 << LOOKUP_TABLE_SIZE> buildLookup(std::map<char, HuffmanCodeItem> huffmanCodes) {
    // setup a simple huffman lookup table -> pretty easy structure
    // - array of size 2 ^ bits used entries
    //  - entries has two infos -> code + bits to shift back
    uint8_t maxBitLength = 0;
    for (auto const& [key, val] : huffmanCodes)
    {
      maxBitLength = std::max(maxBitLength, val.used_bits);
    }

    std::array<LookupItem, 1 << LOOKUP_TABLE_SIZE> lookupIndex{}; // for larger codes this wont work... here we would need a second level for less frequent/longer codes

    for (auto const& [key, val] : huffmanCodes)
    {
      uint32_t starting_idx = val.encoded_bits << (LOOKUP_TABLE_SIZE - val.used_bits);
      uint32_t block_length = (1 << (LOOKUP_TABLE_SIZE - val.used_bits));
      for (uint32_t i = starting_idx; i < starting_idx + block_length; i++) {
        lookupIndex[i] = LookupItem(key, val.used_bits);
      }
    }

    return lookupIndex;
  }

  [[nodiscard]] std::vector<std::string> decode(EncodedStrings encodedContent) const {
    size_t current_idx = 0;
    std::vector<std::string> res{};

    auto lookupIdx = buildLookup(huffmanCodes);

    for (auto& bound : encodedContent.bounds) {
      std::string s;
      uint64_t leftover_content = 0;
      uint8_t leftover_bits = 0;
      for (size_t i = current_idx; i < current_idx + bound; i++) {
        uint64_t current_content = encodedContent.encodedContent[i];
        uint8_t current_bits = 64;

        const uint64_t lookup_mask = (1 << (LOOKUP_TABLE_SIZE)) - 1;

        uint64_t pre_lookup_index_code = ((leftover_content >> (64 - leftover_bits)) << (LOOKUP_TABLE_SIZE - leftover_bits));

        while (current_bits >= LOOKUP_TABLE_SIZE) {
          uint8_t taken_bits = (64 - (LOOKUP_TABLE_SIZE - leftover_bits));
          uint64_t lookup_index_code = pre_lookup_index_code + ((current_content >> taken_bits) & ((1 << (LOOKUP_TABLE_SIZE - leftover_bits)) - 1));
          lookup_index_code &= lookup_mask;

          // lookup_index_code &= lookup_mask;
          assert(lookup_index_code < (1 << LOOKUP_TABLE_SIZE));
          LookupItem li = lookupIdx[lookup_index_code];

          if (li.code == '\0') {
            break;
          }

          s += li.code;

          current_content <<= std::max(0, li.used_bits - leftover_bits);
          current_bits -= std::max(0, li.used_bits - leftover_bits);

          pre_lookup_index_code <<= li.used_bits;
          pre_lookup_index_code &= lookup_mask;
          leftover_bits = std::max(0, leftover_bits - li.used_bits);
        }

        leftover_bits = current_bits;
        leftover_content = current_content;
      }

      current_idx += bound;
      res.push_back(s);
    }

    return res;
  }
};



#endif //HUFFMAN_MINHEAP_HPP
