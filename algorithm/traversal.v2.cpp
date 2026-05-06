#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>

using namespace std;

// =============================
// 1. 資料結構與全域變數
// =============================
struct Edge { char dir; int target; int weight; };
using Graph = unordered_map<int, vector<Edge>>;

struct PathInfo {
    int dist;
    vector<char> dirs;
};

unordered_map<int, unordered_map<int, PathInfo>> memo_dist;
enum class Heading { NORTH, EAST, SOUTH, WEST };

// =============================
// 2. 工具函式
// =============================
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, s.find_last_not_of(" \t\r\n") - start + 1);
}

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

PathInfo dijkstraShortest(int start, int end, const Graph& graph) {
    if (start == end) return {0, {}};
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

        if (d > min_dist[u]) continue;
        if (u == end) return {d, p};

        if (graph.count(u)) {
            for (auto const& edge : graph.at(u)) {
                int next_d = d + edge.weight;
                if (min_dist.find(edge.target) == min_dist.end() || next_d < min_dist[edge.target]) {
                    min_dist[edge.target] = next_d;
                    vector<char> next_p = p;
                    next_p.push_back(edge.dir);
                    pq.push({next_d, {edge.target, next_p}});
                }
            }
        }
    }
    return {999999, {}};
}

Graph readMazeCSV(const string& filename) {
    Graph graph;
    ifstream fin(filename);
    string line; 
    getline(fin, line); 
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line); 
        string cell; 
        vector<string> f;
        while (getline(ss, cell, ',')) f.push_back(trim(cell));
        int idx = stoi(f[0]);
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
// 3. 核心邏輯：限時最大得分 (Orienteering)
// =============================
int main() {
    try {
        Graph graph = readMazeCSV("big_maze_114.csv");
        int startNode = 25;
        vector<int> targets = {startNode}; 
        
        for (auto const& [id, neighbors] : graph) {
            if (id != startNode && neighbors.size() == 1) targets.push_back(id);
        }

        int n = targets.size();
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                memo_dist[targets[i]][targets[j]] = dijkstraShortest(targets[i], targets[j], graph);
            }
        }

        // 計算各死路得分 (曼哈頓距離 * 10)
        vector<int> scores(n, 0);
        for (int i = 0; i < n; i++) {
            if (targets[i] != startNode) {
                scores[i] = memo_dist[startNode][targets[i]].dist * 10;
            }
        }

        // DP: dp[mask][u] = 到達此狀態的最少步數
        const int STEP_LIMIT = 180;
        vector<vector<int>> dp(1 << n, vector<int>(n, 9999));
        vector<vector<int>> parent(1 << n, vector<int>(n, -1));
        dp[1][0] = 0; // 起點 Node 25 (mask=1, index=0)

        for (int mask = 1; mask < (1 << n); ++mask) {
            for (int u = 0; u < n; ++u) {
                if (dp[mask][u] > STEP_LIMIT) continue;
                for (int v = 0; v < n; ++v) {
                    if (!(mask & (1 << v))) {
                        int next_mask = mask | (1 << v);
                        int cost = memo_dist[targets[u]][targets[v]].dist;
                        if (dp[mask][u] + cost <= STEP_LIMIT) {
                            if (dp[next_mask][v] > dp[mask][u] + cost) {
                                dp[next_mask][v] = dp[mask][u] + cost;
                                parent[next_mask][v] = u;
                            }
                        }
                    }
                }
            }
        }

        // 找最高得分
        int maxScore = -1, bestMask = 0, lastNode = 0;
        for (int mask = 1; mask < (1 << n); mask++) {
            for (int u = 0; u < n; u++) {
                if (dp[mask][u] <= STEP_LIMIT) {
                    int currentScore = 0;
                    for (int i = 0; i < n; i++) if (mask & (1 << i)) currentScore += scores[i];
                    
                    if (currentScore > maxScore) {
                        maxScore = currentScore;
                        bestMask = mask;
                        lastNode = u;
                    }
                }
            }
        }

        // 重建路徑
        vector<int> visitOrder;
        int currMask = bestMask, currNode = lastNode;
        while (currNode != -1) {
            visitOrder.push_back(currNode);
            int prev = parent[currMask][currNode];
            currMask ^= (1 << currNode);
            currNode = prev;
        }
        reverse(visitOrder.begin(), visitOrder.end());

        // 輸出結果
        Heading h = Heading::SOUTH; 
        string finalCommands = "";
        int currentPos = targets[visitOrder[0]];
        for (size_t i = 1; i < visitOrder.size(); ++i) {
            int nextPos = targets[visitOrder[i]];
            for (char m : memo_dist[currentPos][nextPos].dirs) finalCommands += getTurnCommand(h, m);
            currentPos = nextPos;
        }

        cout << "--- 80步預算最優策略 ---" << endl;
        cout << "起點節點: " << startNode << endl;
        cout << "預計最高得分: " << maxScore << " 分" << endl;
        cout << "實際消耗步數: " << dp[bestMask][lastNode] << endl;
        cout << "造訪死路順序: ";
        for (int idx : visitOrder) cout << targets[idx] << " ";
        cout << "\nArduino 指令集: " << finalCommands << endl;

    } catch (const exception& e) { cerr << "錯誤: " << e.what() << endl; }
    return 0;
}