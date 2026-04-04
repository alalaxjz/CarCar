#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <map>
#include <set> 

using namespace std;

// =============================
// 1. 資料結構與全域變數
// =============================
struct Edge { char dir; int target; };
using Graph = unordered_map<int, vector<Edge>>;

struct NodeInfo {
    int x = 0, y = 0, score = 0;
    bool isDeadEnd = false;
};

unordered_map<int, NodeInfo> nodeData;
vector<char> bestPath;
int maxTotalScore = 0;

map<pair<int, std::set<int>>, int> memo;

// =============================
// 2. 工具函式
// =============================
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, s.find_last_not_of(" \t\r\n") - start + 1);
}

enum class Heading { NORTH, EAST, SOUTH, WEST };
string getTurnCommand(Heading& current, char nextMove) {
    Heading next;
    if (nextMove == 'N') next = Heading::NORTH;
    else if (nextMove == 'E') next = Heading::EAST;
    else if (nextMove == 'S') next = Heading::SOUTH;
    else next = Heading::WEST;
    int diff = (static_cast<int>(next) - static_cast<int>(current) + 4) % 4;
    current = next;
    if (diff == 0) return "Straight";
    if (diff == 1) return "Right Turn";
    if (diff == 2) return "U-Turn";
    return "Left Turn";
}

// =============================
// 3. 第一階段：BFS 座標推算與計分
// =============================
void buildMapData(int startNode, const Graph& graph) {
    queue<int> q;
    q.push(startNode);
    nodeData[startNode] = {0, 0, 0, false};
    unordered_set<int> visited;
    visited.insert(startNode);

    while (!q.empty()) {
        int u = q.front(); q.pop();

        if (graph.count(u) && graph.at(u).size() == 1 && u != startNode) {
            nodeData[u].isDeadEnd = true;
            nodeData[u].score = (abs(nodeData[u].x) + abs(nodeData[u].y)) * 10;
        }

        if (graph.count(u)) {
            for (auto const& edge : graph.at(u)) {
                if (visited.find(edge.target) == visited.end()) {
                    visited.insert(edge.target);
                    int nx = nodeData[u].x, ny = nodeData[u].y;
                    if (edge.dir == 'N') ny++;
                    else if (edge.dir == 'S') ny--;
                    else if (edge.dir == 'W') nx--;
                    else if (edge.dir == 'E') nx++;
                    nodeData[edge.target] = {nx, ny, 0, false};
                    q.push(edge.target);
                }
            }
        }
    }
}

// =============================
// 4. 第二階段：帶記憶化的 DFS 搜尋
// =============================
void solve(int u, int stepsLeft, int currentScore, vector<char>& currentPath, 
           std::set<int>& scoredNodes, const Graph& graph) {
    
    if (currentScore > maxTotalScore) {
        maxTotalScore = currentScore;
        bestPath = currentPath;
    }

    if (stepsLeft <= 0) return;

    if (memo.count({u, scoredNodes}) && memo[{u, scoredNodes}] >= stepsLeft) return;
    memo[{u, scoredNodes}] = stepsLeft;

    if (graph.count(u)) {
        for (auto const& edge : graph.at(u)) {
            if (!currentPath.empty()) {
                char last = currentPath.back();
                if ((last=='N'&&edge.dir=='S') || (last=='S'&&edge.dir=='N') ||
                    (last=='W'&&edge.dir=='E') || (last=='E'&&edge.dir=='W')) {
                    if (!nodeData[u].isDeadEnd) continue;
                }
            }

            int v = edge.target;
            int gain = 0;
            bool added = false;
            if (nodeData.count(v) && nodeData[v].isDeadEnd && scoredNodes.find(v) == scoredNodes.end()) {
                gain = nodeData[v].score;
                scoredNodes.insert(v);
                added = true;
            }

            currentPath.push_back(edge.dir);
            solve(v, stepsLeft - 1, currentScore + gain, currentPath, scoredNodes, graph);
            
            currentPath.pop_back();
            if (added) scoredNodes.erase(v);
        }
    }
}

// =============================
// 5. 讀取 CSV
// =============================
Graph readMazeCSV(const string& filename) {
    Graph graph;
    ifstream fin(filename);
    if (!fin.is_open()) throw runtime_error("File not found: " + filename);
    string line;
    getline(fin, line); 
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line);
        string cell;
        vector<string> f;
        while (getline(ss, cell, ',')) f.push_back(trim(cell));
        if (f.empty()) continue;
        int idx = stoi(f[0]);
        if (f.size() > 1 && !f[1].empty()) graph[idx].push_back({'N', stoi(f[1])});
        if (f.size() > 2 && !f[2].empty()) graph[idx].push_back({'S', stoi(f[2])});
        if (f.size() > 3 && !f[3].empty()) graph[idx].push_back({'W', stoi(f[3])});
        if (f.size() > 4 && !f[4].empty()) graph[idx].push_back({'E', stoi(f[4])});
    }
    return graph;
}

int main() {
    try {
        int STEP_LIMIT = 40; 
        Graph graph = readMazeCSV("maze.csv");
        
        buildMapData(1, graph);

        vector<char> currentPath;
        
        std::set<int> scoredNodes;
        
        cout << "運算中..." << endl;
        solve(1, STEP_LIMIT, 0, currentPath, scoredNodes, graph);

        cout << "\n=== 最佳路徑分析結果 ===" << endl;
        cout << "最高總得分: " << maxTotalScore << endl;
        cout << "總步數: " << bestPath.size() << endl;
        cout << "-----------------------" << endl;

        Heading head = Heading::NORTH; 
        for (size_t i = 0; i < bestPath.size(); ++i) {
            cout << "Step " << i + 1 << ": [" << bestPath[i] << "] -> " << getTurnCommand(head, bestPath[i]) << endl;
        }

    } catch (const exception& e) {
        cerr << "Runtime Error: " << e.what() << endl;
    }
    return 0;
}
