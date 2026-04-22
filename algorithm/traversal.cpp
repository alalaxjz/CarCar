#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>

using namespace std;

// =============================
// 1. 資料結構
// =============================
// 新增 weight 欄位以紀錄路段實際距離
struct Edge { char dir; int target; int weight; };
using Graph = unordered_map<int, vector<Edge>>;

struct PathInfo {
    int dist;
    vector<char> dirs;
};

// 全域變數
unordered_map<int, unordered_map<int, PathInfo>> memo_dist;
enum class Heading { NORTH, EAST, SOUTH, WEST };

// =============================
// 2. 工具函式
// =============================
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, s.find_last_not_of(" \t\r\n") - start + 1);
}

// 轉向邏輯維持原樣 
string getTurnCommand(Heading& current, char nextMove) {
    Heading next;
    if (nextMove == 'N') next = Heading::NORTH;
    else if (nextMove == 'E') next = Heading::EAST;
    else if (nextMove == 'S') next = Heading::SOUTH;
    else next = Heading::WEST;
    
    int diff = (static_cast<int>(next) - static_cast<int>(current) + 4) % 4;
    current = next;
    
    if (diff == 0) return "f";
    if (diff == 1) return "r";
    if (diff == 2) return "b";
    return "l";
}

// 【重要修正】將單純 BFS 改為 Dijkstra 演算法，考慮實際距離 (weight)
PathInfo dijkstraShortest(int start, int end, const Graph& graph) {
    if (start == end) return {0, {}};
    
    // priority_queue 存放 {累積距離, {當前節點, 經過的方向歷史}}
    using State = pair<int, pair<int, vector<char>>>;
    priority_queue<State, vector<State>, greater<State>> pq;
    unordered_map<int, int> min_dist;

    pq.push({0, {start, {}}});
    min_dist[start] = 0;

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second.first;
        vector<char> p = pq.top().second.second;
        pq.pop();

        // 避免處理已找到更短路徑的舊狀態
        if (d > min_dist[u]) continue;
        if (u == end) return {d, p};

        if (graph.count(u)) {
            for (auto const& edge : graph.at(u)) {
                int next_d = d + edge.weight;
                // 如果找到更短的路線才更新並推入佇列
                if (min_dist.find(edge.target) == min_dist.end() || next_d < min_dist[edge.target]) {
                    min_dist[edge.target] = next_d;
                    vector<char> next_p = p;
                    next_p.push_back(edge.dir);
                    pq.push({next_d, {edge.target, next_p}});
                }
            }
        }
    }
    return {999999, {}}; // 找不到路徑的安全回傳
}

// =============================
// 3. 讀取 CSV
// =============================
Graph readMazeCSV(const string& filename) {
    Graph graph;
    ifstream fin(filename);
    string line; 
    getline(fin, line); // 標題行
    
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line); 
        string cell; 
        vector<string> f;
        while (getline(ss, cell, ',')) f.push_back(trim(cell));
        
        int idx = stoi(f[0]);
        
        // 【重要修正】抓取 CSV 的距離資訊 (若欄位為空則預設為 1)
        int nd = (f.size() > 5 && !f[5].empty()) ? stoi(f[5]) : 1;
        int sd = (f.size() > 6 && !f[6].empty()) ? stoi(f[6]) : 1;
        int wd = (f.size() > 7 && !f[7].empty()) ? stoi(f[7]) : 1;
        int ed = (f.size() > 8 && !f[8].empty()) ? stoi(f[8]) : 1;

        if (f.size() > 1 && !f[1].empty()) graph[idx].push_back({'N', stoi(f[1]), nd});
        if (f.size() > 2 && !f[2].empty()) graph[idx].push_back({'S', stoi(f[2]), sd});
        if (f.size() > 3 && !f[3].empty()) graph[idx].push_back({'W', stoi(f[3]), wd});
        if (f.size() > 4 && !f[4].empty()) graph[idx].push_back({'E', stoi(f[4]), ed});
    }
    return graph;
}

// =============================
// 4. TSP 邏輯 (Bitmask DP)
// =============================
int main() {
    try {
        Graph graph = readMazeCSV("big_maze_114.csv");

        int startNode = 25;
        vector<int> targets = {startNode}; // 索引 0 是起點
        
        for (auto const& [id, neighbors] : graph) {
            // 找出所有死路 (僅一條聯外道路，且不包含起點 1 避免重複)
            if (id != startNode && neighbors.size() == 1) targets.push_back(id);
        }

        int n = targets.size();
        
        // 事先計算所有死路(加上起點)之間的 Dijkstra 最短路徑與距離
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                memo_dist[targets[i]][targets[j]] = dijkstraShortest(targets[i], targets[j], graph);
            }
        }

        // DP 找最短遍歷路徑
        vector<vector<int>> dp(1 << n, vector<int>(n, 1000000));
        vector<vector<int>> parent(1 << n, vector<int>(n, -1));
        dp[1][0] = 0;

        for (int mask = 1; mask < (1 << n); ++mask) {
            for (int u = 0; u < n; ++u) {
                if (!(mask & (1 << u))) continue;
                for (int v = 0; v < n; ++v) {
                    if (mask & (1 << v)) continue;
                    int next_mask = mask | (1 << v);
                    int d = memo_dist[targets[u]][targets[v]].dist;
                    if (dp[next_mask][v] > dp[mask][u] + d) {
                        dp[next_mask][v] = dp[mask][u] + d;
                        parent[next_mask][v] = u;
                    }
                }
            }
        }

        int minTotal = 1000000, last = -1;
        for (int i = 1; i < n; ++i) {
            if (dp[(1 << n) - 1][i] < minTotal) {
                minTotal = dp[(1 << n) - 1][i];
                last = i;
            }
        }

        // 重建路徑順序
        vector<int> visitOrder;
        int currMask = (1 << n) - 1, currNode = last;
        while (currNode != -1) {
            visitOrder.push_back(currNode);
            int nextNode = parent[currMask][currNode];
            currMask ^= (1 << currNode);
            currNode = nextNode;
        }
        reverse(visitOrder.begin(), visitOrder.end());

        // 計算死路總得分 (各死路至 Node 1 的最短物理距離 * 10)
        int totalScore = 0;
        for (int t : targets) {
            if (t != startNode) {
                totalScore += memo_dist[targets[0]][t].dist * 10;
            }
        }

        // 輸出資訊
        Heading h = Heading::SOUTH; // 預設初始朝南
        cout << "--- 掃蕩全場路徑資訊 ---" << endl;
        
        int currentPos = targets[visitOrder[0]];
        string finalCommands = "";
        
        for (size_t i = 1; i < visitOrder.size(); ++i) {
            int nextPos = targets[visitOrder[i]];
            vector<char> moves = memo_dist[currentPos][nextPos].dirs;
            for (char m : moves) {
                finalCommands += getTurnCommand(h, m);
            }
            currentPos = nextPos;
        }
        
        cout << "執行指令集: " << finalCommands << endl;
        cout << "花費總步數(總距離): " << minTotal << endl;
        cout << "死路節點總得分: " << totalScore << " 分" << endl;

    } catch (const exception& e) { 
        cerr << "發生錯誤，請檢查 maze.csv 格式。錯誤訊息: " << e.what() << endl; 
    }
    return 0;
}