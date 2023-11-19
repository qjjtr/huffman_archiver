
#include <fstream>
#include <vector>

void HuffmanArchive(std::ofstream& archive, std::vector<std::pair<std::string, std::ifstream>>& files);
void HuffmanUnarchive(std::ifstream& archive);
