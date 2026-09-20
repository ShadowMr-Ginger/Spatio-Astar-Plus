#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

class Solution609 {
public:
    vector<vector<string>> findDuplicate(vector<string>& paths) {
        // 内容 -> 具有该内容的完整文件路径列表
        unordered_map<string, vector<string>> content_to_paths;
        for (string& path : paths) {
            // 1. 拆分目录与文件部分
            int space = path.find(' ');
            string dir = path.substr(0, space);
            int i = space + 1;
            // 2. 逐个解析 "文件名(内容)" 格式的文件
            while (i < (int)path.size()) {
                int l = path.find('(', i);
                int r = path.find(')', l);
                string name = path.substr(i, l - i);
                string content = path.substr(l + 1, r - l - 1);
                content_to_paths[content].push_back(dir + "/" + name);
                i = r + 2; // 跳过 ") " 或到达末尾
            }
        }
        // 3. 收集内容重复（出现次数大于 1）的路径组
        vector<vector<string>> result;
        for (auto& it : content_to_paths) {
            if (it.second.size() > 1) {
                result.push_back(it.second);
            }
        }
        return result;
    }
};

//int main() {
//	Solution609 solution;
//	vector<string> paths = {
//		"root/a 1.txt(abcd) 2.txt(efgh)",
//		"root/c 3.txt(abcd)",
//		"root/c/d 4.txt(efgh)",
//		"root 4.txt(efgh)"
//	};
//	vector<vector<string>> result = solution.findDuplicate(paths);
//	for (vector<string>& group : result) {
//		for (string& p : group) {
//			cout << p << " ";
//		}
//		cout << endl;
//	}
//	return 0;
//}
