#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stdexcept>
#include <cstdlib>

using namespace std;

// =============================
// 型別定義
// =============================
struct PairHash {
    size_t operator()(const pair<int, int>& p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
    }
};

// 調整 Graph 結構，使其包含方向 (char: 'N','S','W','E')
using Graph = unordered_map<int, vector<pair<char, int>>>;

using M1Value = tuple<int, vector<char>, pair<int, int>>; // dist, path, (dx, dy)
using M2Value = tuple<int, vector<char>, int>;           // dist, path, manhattan

using M1 = unordered_map<pair<int, int>, M1Value, PairHash>;
using M2 = unordered_map<pair<int, int>, M2Value, PairHash>;

// =============================
// 工具函式
// =============================
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

vector<string> splitCSVLine(const string& line) {
    vector<string> fields;
    string field;
    stringstream ss(line);
    while (getline(ss, field, ',')) {
        fields.push_back(trim(field));
    }
    if (!line.empty() && line.back() == ',') fields.push_back("");
    return fields;
}

bool tryParseInt(const string& s, int& value) {
    string t = trim(s);
    if (t.empty()) return false;
    try {
        value = stoi(t);
        return true;
    } catch (...) { return false; }
}

string pathToString(const vector<char>& path) {
    return string(path.begin(), path.end());
}

// =============================
// 第 1 部分：讀 CSV 並建立 Graph (包含方向)
// =============================
Graph readMazeCSV(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) throw runtime_error("無法開啟檔案: " + filename);
    
    Graph graph;
    string line;
    getline(fin, line); // 跳過表頭

    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        vector<string> fields = splitCSVLine(line);
        if (fields.size() < 5) continue;

        int idx;
        if (!tryParseInt(fields[0], idx)) continue;

        int neighbor;
        if (tryParseInt(fields[1], neighbor)) graph[idx].push_back({'N', neighbor});
        if (tryParseInt(fields[2], neighbor)) graph[idx].push_back({'S', neighbor});
        if (tryParseInt(fields[3], neighbor)) graph[idx].push_back({'W', neighbor});
        if (tryParseInt(fields[4], neighbor)) graph[idx].push_back({'E', neighbor});
    }
    return graph;
}

// =============================
// 第 2 部分：BFS 核心邏輯 (填充 M1)
// =============================
void runBFS(int startNode, const Graph& graph, M1& m1) {
    struct State {
        int node;
        vector<char> path;
        int x, y;
    };

    queue<State> q;
    unordered_map<int, int> visited_dist; // 紀錄造訪過的節點與其最短距離

    q.push({startNode, {}, 0, 0});
    visited_dist[startNode] = 0;

    while (!q.empty()) {
        State curr = q.front();
        q.pop();

        // 存入 M1
        m1[{startNode, curr.node}] = make_tuple((int)curr.path.size(), curr.path, make_pair(curr.x, curr.y));

        // 探索鄰居
        if (graph.count(curr.node)) {
            for (auto const& edge : graph.at(curr.node)) {
                char dir = edge.first;
                int nextNode = edge.second;

                // BFS 保證第一次造訪就是最短路徑
                if (visited_dist.find(nextNode) == visited_dist.end()) {
                    visited_dist[nextNode] = curr.path.size() + 1;
                    
                    vector<char> nextPath = curr.path;
                    nextPath.push_back(dir);

                    int nx = curr.x, ny = curr.y;
                    if (dir == 'N') ny++;
                    else if (dir == 'S') ny--;
                    else if (dir == 'W') nx--;
                    else if (dir == 'E') nx++;

                    q.push({nextNode, nextPath, nx, ny});
                }
            }
        }
    }
}

// =============================
// 第 3 部分：建立 M2 (篩選死路與計算得分)
// =============================
M2 buildM2(const M1& m1, const Graph& graph, int startNode) {
    M2 m2;
    for (const auto& entry : m1) {
        int u = entry.first.first;
        int v = entry.first.second;

        // 判定死路：鄰居數量為 1 且不是起點
        if (graph.at(v).size() == 1 && v != startNode) {
            const auto& val = entry.second;
            int dist = get<0>(val);
            const vector<char>& path = get<1>(val);
            pair<int, int> coord = get<2>(val);

            int manhattan = abs(coord.first) + abs(coord.second);
            m2[{u, v}] = make_tuple(dist, path, manhattan);
        }
    }
    return m2;
}

// 定義方向與旋轉邏輯
enum class Heading { NORTH, EAST, SOUTH, WEST };

string getTurnCommand(Heading& current, char nextMove) {
    Heading next;
    if (nextMove == 'N') next = Heading::NORTH;
    else if (nextMove == 'E') next = Heading::EAST;
    else if (nextMove == 'S') next = Heading::SOUTH;
    else next = Heading::WEST;

    // 計算相對旋轉次數 (每個 90 度為 1 單位)
    int diff = (static_cast<int>(next) - static_cast<int>(current) + 4) % 4;
    
    string cmd;
    if (diff == 0) cmd = "f";
    else if (diff == 1) cmd = "r";
    else if (diff == 2) cmd = "b";
    else if (diff == 3) cmd = "l";

    current = next; // 更新目前車頭朝向
    return cmd;
}

int main() {
    try {
        string csvFilename = "maze.csv";
        Graph graph = readMazeCSV(csvFilename);
        M1 m1;
        runBFS(1, graph, m1);
        M2 m2 = buildM2(m1, graph, 1);

        if (m2.empty()) {
            cout << "找不到任何得分死路！" << endl;
            return 0;
        }

        // 1. 找出 Score 最大的路徑
        pair<int, int> bestTargetKey;
        int maxScore = -1;

        for (auto const& entry : m2) {
            int manhattan = get<2>(entry.second);
            int currentScore = manhattan * 30;
            if (currentScore > maxScore) {
                maxScore = currentScore;
                bestTargetKey = entry.first;
            }
        }

        int targetNode = bestTargetKey.second;
        vector<char> rawPath = get<1>(m2[bestTargetKey]);

        cout << "=== 最佳路徑規劃 ===" << endl;
        cout << "目標節點: " << targetNode << " (最高分: " << maxScore << ")" << endl;
        cout << "原始方位路徑: " << pathToString(rawPath) << endl << endl;

        // 2. 轉換為車體轉向指令
        // 假設車子起點是面向北方 (Heading::NORTH)
        Heading currentHeading = Heading::WEST;
        
        cout << "--- 導航指令清單 ---" << endl;
        for (size_t i = 0; i < rawPath.size(); ++i) {
            char move = rawPath[i];
            string turn = getTurnCommand(currentHeading, move);
            
            // 輸出：移動 1 單位後的操作
            //cout << "第 " << i + 1 << " 段路: 先執行 [" << turn << "] 然後直行至下一個節點" << endl;
            cout << turn;
        }
        cout << endl;
        cout << "抵達死路節點 " << targetNode << "，任務完成！" << endl;

    } catch (const exception& e) {
        cerr << "錯誤: " << e.what() << endl;
    }
    return 0;
}

/*
///////////////////////////
請注意：
1. 車子初始方為設定成向北
2. 此為有設定終點的結果
3. 不考慮時間（不限制步數）
///////////////////////////
*/