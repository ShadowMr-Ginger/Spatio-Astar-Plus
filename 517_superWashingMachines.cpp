#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
using namespace std;

class Solution517 {
public:
    int findMinMoves(vector<int>& machines) {
        int n = machines.size();
        int total = 0;
        for (int i = 0;i < n;i++) {
            total += machines[i];
        }
        if (total % n != 0) {
            return -1;
        }
        int avg = total / n;
        int balance = 0;
        int ans = 0;
        for (int i = 0;i < n;i++) {
            int diff = machines[i] - avg;
            balance += diff;
            // 前缀净流入与单点超出量都决定最小轮数
            ans = max(ans, max(abs(balance), abs(diff)));
        }
        return ans;
    }
};

//int main() {
//	Solution517 sol;
//	vector<int> machines = {1,0,5};
//	cout << sol.findMinMoves(machines) << endl;
//	vector<int> machines2 = {0,3,0};
//	cout << sol.findMinMoves(machines2) << endl;
//	vector<int> machines3 = {0,2,0};
//	cout << sol.findMinMoves(machines3) << endl;
//	return 0;
//}
