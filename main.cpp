#include <iostream>
#include "./huffman/HuffmanIndex.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> readColumnFromCSV(const std::string& filename, int columnIndex) {
  std::vector<std::string> columnData;
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Could not open the file: " << filename << std::endl;
    return columnData; // Return empty vector if file couldn't be opened
  }

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string cell;
    int currentIndex = 0;

    // Split the line into cells
    while (std::getline(ss, cell, '|')) {
      if (currentIndex == columnIndex) {
        columnData.push_back(cell); // Add the cell to the vector if it matches the columnIndex
        break; // Exit the loop once we've processed the desired column
      }
      currentIndex++;
    }
  }

  file.close();
  return columnData;
}

int main() {
  std::string filename = "Euro2016_1.csv"; // Specify your CSV file name
  int columnIndex = 9; // Specify the column index you want to read (0-based index)

  std::vector<std::string> columnStrings = readColumnFromCSV(filename, columnIndex);
  std::vector<std::string> test = std::vector<std::string>(columnStrings.begin(), columnStrings.begin() + 50);
  // std::vector<std::string> test{"150 Russians behind much of violence in Marseille ahead of Euro 2016 England-Russia match, say... https://t.co/XHLtAmCspy via @BBCWorld"};

  auto t = HuffmanEncoder(test);

  auto res = t.encode(test);
  auto res_de = t.decode(res);

  uint32_t prev_size = 0;
  for (auto& st : test) {
    prev_size += st.size();
  }

  std::cout << "previous size: " << prev_size << "byte\n" << "new size: " << res.encodedContent.size() * 8 << "byte\n" << std::endl;

  for (size_t i = 0; i < test.size(); i++) {
    if (test[i] != res_de[i]) {
      std::cout << test[i] << "\n" << res_de[i] << "\n\n" << std::endl;
    }
  }

  return 0;
}