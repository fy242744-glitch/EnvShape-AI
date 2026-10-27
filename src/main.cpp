#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdio>
#include <windows.h>   // <--- 新增这行
#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

int main() {
    SetConsoleOutputCP(CP_UTF8);   // <--- 新增这行，强制控制台使用 UTF-8
    fs::path root = fs::current_path();;
    fs::path tasks_path = root / "tasks" / "tasks.json";

    // 1. 检查题库
    if (!fs::exists(tasks_path)) {
        std::cerr << "找不到 tasks.json，请在项目根目录运行\n";
        return 1;
    }

    std::ifstream in(tasks_path);
    json tasks;
    in >> tasks;

    // 准备输出目录
    fs::create_directories(root / "answers");
    fs::create_directories(root / "logs");

    // 打开一个新的 CSV 专门存 C++ 跑出的结果，避免和 Python 版的混淆
    std::ofstream csv(root / "logs" / "results_cpp.csv");
    csv << "task_id,group,correct,reason\n";

    std::cout << "开始运行 C++ 调度器...\n\n";

    // 2. 遍历所有题目
    for (const auto& task : tasks) {
        std::string id = task["id"];
        std::string answer_file = "answers/answer_A_" + id + ".json";

        // 3. 模拟模型回答（故意篡改几道题，测试验证器
        json answer = task["expected"];

        if (id == "func_004") {
            answer["intercept"] = -2;  // 故意写错截距（正确是 2）
        }
        if (id == "func_005") {
            answer["intersection"] = {3, 4}; // 故意写错交点（正确是 [3, 5]）
        }
        if (id == "func_012") {
            answer["quadrants"] = {1, 2, 4}; // 故意写错象限（正确是 [1, 3, 4]）
        }
        if (id == "func_012") {
            answer["quadrants"] = {1, 2, 4}; // 故意写错象限（正确是 [1, 3, 4]）
        }

        // 👇 把下面这三行补回来，把篡改后的答案写入文件！
        std::ofstream ans_out(answer_file);
        ans_out << answer.dump(2);
        ans_out.close();
        
        // 4. 拼装调用 Python 验证器的命令
        // 等同于在终端输入: python python/validator.py tasks/tasks.json func_001 answers/answer_A_func_001.json
        std::string cmd = "python python/validator.py tasks/tasks.json " + id + " " + answer_file;

        // 5. 启动子进程并读取返回结果
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "执行 Python 失败: " << id << "\n";
            continue;
        }

        char buffer[256];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);

        // 6. 解析 Python 返回的 JSON (比如 {"correct": true, "reason": "ok"})
        try {
            json res = json::parse(result);
            bool correct = res["correct"];
            std::string reason = res["reason"];

            // 写入 CSV
            csv << id << ",A," << (correct ? "True" : "False") << "," << reason << "\n";
            std::cout << id << ": " << (correct ? "PASS" : "FAIL") << "\n";
        } catch (const std::exception& e) {
            std::cerr << id << " 解析 Python 输出失败: " << result << "\n";
        }
    }

    csv.close();
    std::cout << "\n全部完成！结果已写入 logs/results_cpp.csv\n";
    return 0;
}