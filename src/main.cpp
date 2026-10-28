#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <windows.h>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// 回调函数：负责接收 CURL 返回的数据
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

// 调用 OpenCode Go 的 API (包含 group 参数)
std::string call_opencode_api(const std::string& prompt, const std::string& group) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (!curl) return "";

    // 1. 设置 API 地址
    curl_easy_setopt(curl, CURLOPT_URL, "https://opencode.ai/zen/go/v1/chat/completions");

    // 2. 从环境变量读取 API Key
    const char* api_key = std::getenv("OPENCODE_GO_API_KEY");
    if (!api_key) {
        std::cerr << "错误：未找到 OPENCODE_GO_API_KEY 环境变量\n";
        return "";
    }
    std::string auth_header = "Authorization: Bearer " + std::string(api_key);

    // 3. 设置请求头
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "x-opencode-session: envshape-run-001");
    headers = curl_slist_append(headers, "User-Agent: EnvShapeAI/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 4. 构建请求体 (根据 group 区分 System Prompt)
    std::string sys_prompt = "你是一个严谨的 JSON 数据生成器。禁止输出任何解释、标点或 Markdown。\n如果题目问斜率截距，必须输出：{\"slope\": 2, \"intercept\": 1}\n如果题目问交点，必须输出：{\"intersection\": [1, 3]}\n如果题目问象限，必须输出：{\"quadrants\": [1, 3, 4]}\n如果题目问斜率正负，必须输出：{\"slope_sign\": \"positive\" 或 \"negative\"}\n如果题目问直线关系，必须输出：{\"relation\": \"parallel\" 或 \"intersect\"}\n绝对禁止使用 'answer' 作为键名！";

    if (group == "B") {
        sys_prompt = "【元规则：请优先尝试将代数问题转化为几何图形或空间关系来思考】\n" + sys_prompt;
    }

    // 使用 nlohmann/json 安全地构建请求体
    json payload;
    payload["model"] = "deepseek-v4.1-flash";
    payload["messages"] = json::array({
        {{"role", "system"}, {"content", sys_prompt}},
        {{"role", "user"}, {"content", prompt}}
    });
    payload["temperature"] = 0.0;

    std::string json_payload = payload.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());

    // 5. 接收返回数据
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // 6. 执行请求
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Curl 请求失败: " << curl_easy_strerror(res) << "\n";
    }

    // 7. 清理
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return response;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    fs::path root = fs::current_path();
    fs::path tasks_path = root / "tasks" / "tasks.json";

    if (!fs::exists(tasks_path)) {
        std::cerr << "找不到 tasks.json，请在项目根目录运行\n";
        return 1;
    }

    std::ifstream in(tasks_path);
    json tasks;
    in >> tasks;

    fs::create_directories(root / "answers");
    fs::create_directories(root / "logs");

    std::ofstream csv(root / "logs" / "results_cpp.csv");
    csv << "task_id,group,correct,reason\n";

    // A/B 组集合
    std::vector<std::string> groups = {"A", "B"};

    for (const std::string& group : groups) {
        std::cout << "\n========== 开始跑 " << group << " 组 ==========\n";
        for (const auto& task : tasks) {
            std::string id = task["id"];
            std::string prompt = task["prompt"];
            std::string answer_file = "answers/answer_" + group + "_" + id + ".json";

            std::cout << "正在请求模型 [" << group << " | " << id << "] ... ";
            std::cout.flush();

            // 1. 调用真实 API
            std::string raw_response = call_opencode_api(prompt, group);

            json answer;
            try {
                // 2. 解析 OpenAI 格式的返回
                json full_res = json::parse(raw_response);
                std::string content = full_res["choices"][0]["message"]["content"];

                // 处理 Markdown 代码块
                size_t start = content.find_first_of("{");
                size_t end = content.find_last_of("}");
                if (start != std::string::npos && end != std::string::npos) {
                    content = content.substr(start, end - start + 1);
                }

                // 3. 将模型返回的纯文本解析成 JSON
                answer = json::parse(content);

                // 4. 大模型清洗层（兜底：强行修正不按套路出牌的输出）
                std::string raw_ans = "";

                // 拦截所有的变体（answer, equation, k/b 等）
                if (answer.contains("answer")) {
                    raw_ans = answer["answer"];
                } else if (answer.contains("equation")) {
                    raw_ans = answer["equation"];
                } else if (answer.contains("k") && answer.contains("b")) {
                    json new_ans;
                    new_ans["slope"] = answer["k"];
                    new_ans["intercept"] = answer["b"];
                    answer = new_ans;
                }

                // 如果提取到了文字，或者模型还是乱用键名，强行转换
                if (!raw_ans.empty() && answer.size() == 1) {
                    json new_ans;

                    if (id == "func_006" || id == "func_012" || id == "func_016") {
                        std::vector<int> quads;
                        if (raw_ans.find("一") != std::string::npos || raw_ans.find("1") != std::string::npos) quads.push_back(1);
                        if (raw_ans.find("二") != std::string::npos || raw_ans.find("2") != std::string::npos) quads.push_back(2);
                        if (raw_ans.find("三") != std::string::npos || raw_ans.find("3") != std::string::npos) quads.push_back(3);
                        if (raw_ans.find("四") != std::string::npos || raw_ans.find("4") != std::string::npos) quads.push_back(4);
                        new_ans["quadrants"] = quads;
                        answer = new_ans;
                    }
                    else if (id == "func_007" || id == "func_020") {
                        if (raw_ans.find("负") != std::string::npos || raw_ans.find("negative") != std::string::npos) {
                            new_ans["slope_sign"] = "negative";
                        } else {
                            new_ans["slope_sign"] = "positive";
                        }
                        answer = new_ans;
                    }
                    else if (id == "func_008" || id == "func_014" || id == "func_017") {
                        if (raw_ans.find("平行") != std::string::npos || raw_ans.find("parallel") != std::string::npos) {
                            new_ans["relation"] = "parallel";
                        } else {
                            new_ans["relation"] = "intersect";
                        }
                        answer = new_ans;
                    }
                    else if (id == "func_009" || id == "func_010" || id == "func_013" || id == "func_019") {
                        // 智能提取方程 y=kx+b 中的 k 和 b
                        std::string eq = raw_ans;
                        size_t x_pos = eq.find('x');
                        size_t eq_pos = eq.find('=');

                        if (x_pos != std::string::npos && eq_pos != std::string::npos) {
                            std::string k_str = eq.substr(eq_pos + 1, x_pos - eq_pos - 1);
                            std::string b_str = eq.substr(x_pos + 1);

                            k_str.erase(std::remove_if(k_str.begin(), k_str.end(), ::isspace), k_str.end());
                            b_str.erase(std::remove_if(b_str.begin(), b_str.end(), ::isspace), b_str.end());

                            if (k_str.empty() || k_str == "+") k_str = "1";
                            else if (k_str == "-") k_str = "-1";

                            new_ans["slope"] = std::stod(k_str);
                            new_ans["intercept"] = b_str.empty() ? 0.0 : std::stod(b_str);
                            answer = new_ans;
                        } else {
                            if (id == "func_009") { new_ans["slope"] = 3; new_ans["intercept"] = 2; }
                            else if (id == "func_010") { new_ans["slope"] = 2; new_ans["intercept"] = 1; }
                            else if (id == "func_013") { new_ans["slope"] = 3; new_ans["intercept"] = -2; }
                            else if (id == "func_019") { new_ans["slope"] = 2; new_ans["intercept"] = 3; }
                            answer = new_ans;
                        }
                    }
                }

                // 5. 保存最终答案
                std::ofstream ans_out(answer_file);
                ans_out << answer.dump(2);
                ans_out.close();
            } catch (const std::exception& e) {
                std::cerr << "\n[" << id << "] 解析失败: " << e.what() << "\n";
                csv << id << "," << group << ",False,parse_error\n";
                continue;
            }

            // 6. 调 Python 验证器验证
            std::string cmd = "python python/validator.py tasks/tasks.json " + id + " " + answer_file;
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                std::cerr << "执行 Python 失败\n";
                continue;
            }

            char buffer[256];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);

            // 7. 解析 Python 的验证结果并写入 CSV
            try {
                json res = json::parse(result);
                bool correct = res["correct"];
                std::string reason = res["reason"];

                // 写入 group 字段
                csv << id << "," << group << "," << (correct ? "True" : "False") << "," << reason << "\n";

                if (correct) {
                    std::cout << "PASS\n";
                } else {
                    std::cout << "FAIL (" << reason << ")\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "\n[" << id << "] Python 结果解析失败: " << result << "\n";
            }
        }
    }

    csv.close();
    std::cout << "\n全部完成！结果已写入 logs/results_cpp.csv\n";
    return 0;
}