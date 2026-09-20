#include <iostream>
#include <vector>

using namespace std;

class Solution1027 {
public:
    int longestArithSeqLength(vector<int>& nums) {
        int n = nums.size();
        // dp[i][d] 表示以 nums[i] 结尾、公差为 d-500 的最长等差子序列长度
        // nums[i] 范围 [0,500]，公差范围 [-500,500]，整体加 500 偏移作下标
        vector<vector<int>> dp(n, vector<int>(1001, 1));
        int max_len = 2;
        // 1. 枚举数对 (j, i) 作为等差子序列的最后两项
        for (int i = 1;i < n;i++) {
            for (int j = 0;j < i;j++) {
                int diff = nums[i] - nums[j] + 500;
                // 2. 接在以 j 结尾、同公差的序列后；取 max 防止同一 i 被较小值覆盖
                dp[i][diff] = max(dp[i][diff], dp[j][diff] + 1);
                max_len = max(max_len, dp[i][diff]);
            }
        }
        return max_len;
    }

private:

//int main() {
//	vector<int> nums = {3, 6, 9, 12};
//	Solution1027 solution;
//	cout << solution.longestArithSeqLength(nums) << endl;
//	return 0;
//}
};
