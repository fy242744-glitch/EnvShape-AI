#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

int main() {
    fs::path root = fs::current_path();
    fs::path tasks_path = root / "tasks" / "tasks.json";

    if (!fs::exists(tasks_path)) {
        std::cerr << "tasks.json not found at: " << tasks_path << "\n";
        std::cerr << "run this program from the project root directory\n";
        return 1;
    }

    std::ifstream in(tasks_path);
    if (!in.is_open()) {
        std::cerr << "failed to open: " << tasks_path << "\n";
        return 1;
    }

    json tasks;
    try {
        in >> tasks;
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return 1;
    }

    if (!tasks.is_array()) {
        std::cerr << "tasks.json root is not an array\n";
        return 1;
    }

    std::cout << "loaded " << tasks.size() << " tasks\n\n";

    for (const auto& task : tasks) {
        std::string id = task.value("id", "?");
        std::string type = task.value("type", "?");
        std::string variant = task.value("variant", "?");
        std::cout << id << " | " << type << " | " << variant << "\n";
    }

    return 0;
}