#define _CRT_SECURE_NO_WARNINGS
#include <bits/stdc++.h>
using namespace std;

const int n = 200;
const int robot_num = 10;
const int berth_num = 10;
const int N = 210;
const int cargo_remain_time = 1000;
const int boat_num = 5;
const int total_zhen = 15000;
const int boat_moving_time = 500;

struct Robot
{
    int x, y, goods;
    int status;
    int mbx, mby;
    Robot() {}
    Robot(int startX, int startY) {
        x = startX;
        y = startY;
    }
    bool berthsCanBeReached[berth_num] = {false};
    bool active = false;
    bool carried = false;
    bool task_allocated = false;
    int choose_item_id = -1;
    int target_berth = -1;
    int carried_money = 0;
    list<int> choose_item_id_next;
    list<int> target_berth_next;
    list<int> carried_money_next;
    list<int> path_getting;
    list<int> path_pulling;
    list<list<int>> path_getting_next;
    list<list<int>> path_pulling_next;
    list<int> path_total;
    list<int> path_direction;
}robot[robot_num + 10];

Robot robotTest[robot_num+10];

struct Berth
{
    int x;
    int y;
    int transport_time;
    int loading_speed;
    bool BoatExist = false;
    list<int> goods;
    int money_total;
    bool closed = false;
    Berth() {}
    Berth(int x, int y, int transport_time, int loading_speed) {
        this->x = x;
        this->y = y;
        this->transport_time = transport_time;
        this->loading_speed = loading_speed;
    }
}berth[berth_num + 10];

struct Boat
{
    int num, pos, status;
    int capacity;
    int once_carried_cargo_number = 30;
    int final_berth = -1;
    int final_cycle_max_zhen = -1;
    int total_cycle_number = -1;
    list<int> berths_to_go;
    int upload_time;
    int cycle_total_time;

    int carried_money;
    bool need_to_ship = false;
    int shipping_target = -1;
    bool at_berth = false;
    bool is_final_cycle = false;
    int now_berth;
    int remaining_time = 0;
    bool need_to_go = false;
    int surplus_time = 0;
}boat[10];

int money, boat_capacity, id;
char ch[N][N];
int gds[N][N];



struct Cargo
{
    int x;
    int y;
    int point_number;
    int money;
    int surplus_zheng;
    int item_id;
    int status;  // 状态：0闲置，1正在被一个机器人选定，2已经被拿取
    bool CanBeGot = false;
    int DistanceToBerth = 999999;
    int nearestBerthNumber = -1;
    double value_distance_ratio;
    list<int> path_point;
    list<int> path_direction;
    Cargo() {}
    Cargo(int x, int y, int money, int item_id, int surplus_zheng = 1000, int status = 0) {
        this->x = x;
        this->y = y;
        this->point_number = x * n + y;
        this->money = money;
        this->item_id = item_id;
        this->surplus_zheng = surplus_zheng;
        this->status = status;
    }
};

static int CloseList[n * n];

template <typename T>
std::list<T> merge_in_order(const std::list<T>& list1, const std::list<T>& list2) {
    std::list<T> merged_list;

    // 将第一个链表的元素添加到合并后的链表中
    for (const auto& elem : list1) {
        merged_list.push_back(elem);
    }

    // 将第二个链表的元素添加到合并后的链表中
    for (const auto& elem : list2) {
        merged_list.push_back(elem);
    }

    return merged_list;
}

void Astar_DistCalc(int x_start, int y_start, int x_target, int y_target, int Node_movable_node[][4], int* CanReach, int* distance)
{
    int startP_number = x_start * n + y_start;
    int endP_number = x_target * n + y_target;
    list<int> openlist;
    list<int> openlist_new;
    int closelist[n * n] = { 0 };
    openlist.push_back(startP_number);
    *distance = -1;
    *CanReach = 0;
    while (*CanReach == 0) {
        if (openlist.empty()) {
            break;
        }
        *distance++;
        
        for (auto iter = openlist.begin(); iter != openlist.end(); iter++) {
            if (*iter == endP_number) {
                *CanReach = 1;
                break;
            }
            if (closelist[*iter] == 0) {
                closelist[*iter] = 1;
                for (int i = 0; i < 4; i++) {
                    if (Node_movable_node[*iter][i] != -1) {
                        openlist_new.push_back(Node_movable_node[*iter][i]);
                    }
                }
            }
            openlist = openlist_new;
            openlist_new.clear();
        }
    }
}

void scanBerthsCanBeReached(Robot robots[], Berth berths[], int Node_movable_node[][4])
{
    for (int i = 0; i < robot_num; i++) {
        int x_start = robots[i].x;
        int y_start = robots[i].y;
        int x_end[berth_num] = { 0 };
        int y_end[berth_num] = { 0 };
        for (int j = 0; j < berth_num; j++) {
            x_end[j] = berths[j].x;
            y_end[j] = berths[j].y;
        }
        //Astar部分
        int startP_number = x_start * n + y_start;
        int endP_number[berth_num] = { 0 };
        for (int j = 0; j < berth_num; j++) {
            endP_number[j] = x_end[j] * n + y_end[j];
        }
        list<int> openlist;
        list<int> openlist_new;
        int closelist[n * n] = { 0 };
        openlist.push_back(startP_number);

        while (true) {
            bool all_true = true;
            for (int j = 0; j < berth_num; j++) {
                if (!robots[i].berthsCanBeReached[j]) {
                    all_true = false;
                }
            }
            if (all_true) {
                break;
            }
            if (openlist.empty()) {
                break;
            }

            for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
                if (closelist[*iter] == 0) {
                    closelist[*iter] = 1;
                    for (int j = 0; j < 4; j++) {
                        if (Node_movable_node[*iter][j] != -1) {
                            openlist_new.push_back(Node_movable_node[*iter][j]);
                        }
                    }
                }
                for (int j = 0; j < berth_num; j++) {
                    if (*iter == endP_number[j]) {
                        robots[i].berthsCanBeReached[j] = true;
                    }
                }
            }
            openlist = openlist_new;
            openlist_new.clear();
        }
    }
}

void precalc_CargoToBerth(Cargo* cargo, Berth berths[], int Node_movable_node[][4], int Node_is_berth[], bool* CanBeGot,int* BerthNumber, int* distance)
{ // 预计算货物到最近泊位的距离。如果无法抵达泊位，认为该货物不可获取。
    int x_start = cargo->x;
    int y_start = cargo->y;
    int x_end[berth_num] = { 0 };
    int y_end[berth_num] = { 0 };
    for (int j = 0; j < berth_num; j++) {
        x_end[j] = berths[j].x;
        y_end[j] = berths[j].y;
    }
    //Astar部分
    int startP_number = x_start * n + y_start;

    struct openlist_unit {
        int pointNumber;
        int lastPoint;
        int diRection;
        openlist_unit() {}
        openlist_unit(int pointnumber, int lastpoint, int direction) {
            this->lastPoint = lastpoint;
            this->pointNumber = pointnumber;
            this->diRection = direction;
        }
    };

    list<openlist_unit> openlist;
    list<openlist_unit> openlist_new;
    int closelist[n * n][3] = {0};
    openlist_unit openP = openlist_unit(startP_number, -1, -1);
    openlist.push_back(openP);
    *distance = -1;
    int final_endPnumber;
    while (true) {
        if (openlist.empty()) {
            break;
        }
        *distance = *distance + 1;
        for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
            if (closelist[iter->pointNumber][0] == 0) {
                closelist[iter->pointNumber][0] = 1;
                closelist[iter->pointNumber][1] = iter->lastPoint;
                closelist[iter->pointNumber][2] = iter->diRection;
                for (int j = 0; j < 4; j++) {
                    if (Node_movable_node[iter->pointNumber][j] != -1) {
                        openP.pointNumber = Node_movable_node[iter->pointNumber][j];
                        openP.lastPoint = iter->pointNumber;
                        openP.diRection = j;
                        openlist_new.push_back(openP);
                    }
                }
            }
            if (Node_is_berth[iter->pointNumber] != -1) {
                *CanBeGot = true;
                *BerthNumber = Node_is_berth[iter->pointNumber];
                final_endPnumber = iter->pointNumber;
                break;
            }

            if (*CanBeGot == true) {
                break;
            };
        }
        if (*CanBeGot == true) {
            break;
        };
        openlist = openlist_new;
        openlist_new.clear();
    }
    //回溯路径
    if (*CanBeGot){
        int last_Pnumber = final_endPnumber;
        while (closelist[last_Pnumber][1] != -1) {
            cargo->path_point.push_front(last_Pnumber);
            cargo->path_direction.push_front(closelist[last_Pnumber][2]);
            last_Pnumber = closelist[last_Pnumber][1];
        }
    }
}



/*
********* 核心函数 ***********
* 机器人调度函数
* 选择最优价距比的货物并路径规划
*/
void TaskArrange(Robot* robot1, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged) {
    int x_start = robot1->x;
    int y_start = robot1->y;
    list<Cargo> Cargolist_new;
    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
        if (iter->status == 0 && robot1->berthsCanBeReached[iter->nearestBerthNumber])
        {
            Cargolist_new.push_back(*iter);
        }
    }
    if (!Cargolist_new.empty()){
        //Astar部分
        int startP_number = x_start * n + y_start;
        struct openlist_unit {
            int pointNumber;
            int lastPoint;
            int diRection;
            openlist_unit() {}
            openlist_unit(int pointnumber, int lastpoint, int direction) {
                this->lastPoint = lastpoint;
                this->pointNumber = pointnumber;
                this->diRection = direction;
            }
        };
        list<openlist_unit> openlist;
        list<openlist_unit> openlist_new;
        int closelist[n * n][3] = { 0 };
        openlist_unit openP = openlist_unit(startP_number, -1, -1);
        openlist.push_back(openP);
        int distance = -1;
        double max_value_distance_ratio = 0;
        Cargo* chooseCargo = new Cargo();
        int max_scan_distance = 99999;
        bool successful_plan = false;
        while (true) {
            if (openlist.empty()) {
                break;
            }
            distance++;
            if (distance > max_scan_distance) {
                break;
            }
            for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
                if (closelist[iter->pointNumber][0] == 0) {
                    closelist[iter->pointNumber][0] = 1;
                    closelist[iter->pointNumber][1] = iter->lastPoint;
                    closelist[iter->pointNumber][2] = iter->diRection;
                    for (int j = 0; j < 4; j++) {
                        if (Node_movable_node[iter->pointNumber][j] != -1) {
                            openP.pointNumber = Node_movable_node[iter->pointNumber][j];
                            openP.lastPoint = iter->pointNumber;
                            openP.diRection = j;
                            openlist_new.push_back(openP);
                        }
                    }
                }
                bool scanRatio = false;
                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                    // 剔除无法获取的货物
                    if (iter2->surplus_zheng - distance < 20) {
                        iter2 = Cargolist_new.erase(iter2);
                    }
                    // 检测是否可以取到货物
                    else if (iter->pointNumber == iter2->point_number) {
                        successful_plan = true;
                        //计算价距比
                        iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                        if (iter2->value_distance_ratio > max_value_distance_ratio) {
                            chooseCargo->point_number = iter2->point_number;
                            chooseCargo->item_id = iter2->item_id;
                            chooseCargo->path_point = iter2->path_point;
                            chooseCargo->path_direction = iter2->path_direction;
                            chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                            chooseCargo->money = iter2->money;
                            max_value_distance_ratio = iter2->value_distance_ratio;
                            scanRatio = true;
                            iter2 = Cargolist_new.erase(iter2);
                        }
                        else {
                            iter2 = Cargolist_new.erase(iter2);
                            if (iter2 == Cargolist_new.end()) {
                                break;
                            }
                        }
                    }
                    else {
                        iter2++;
                    }
                }
                if (scanRatio) {
                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                        if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
                            iter2 = Cargolist_new.erase(iter2);
                        }
                        else {
                            int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
                            if (over_distance < max_scan_distance) {
                                max_scan_distance = over_distance;
                            }
                            iter2++;
                        }
                    }
                }
            }
            openlist = openlist_new;
            openlist_new.clear();
        }
        if (successful_plan){
            *task_arranged = true;
            //回溯路径
            robot1->choose_item_id = chooseCargo->item_id;
            int last_Pnumber = chooseCargo->point_number;
            while (closelist[last_Pnumber][1] != -1) {
                robot1->path_getting.push_front(last_Pnumber);
                robot1->path_total.push_front(last_Pnumber);
                robot1->path_direction.push_front(closelist[last_Pnumber][2]);
                last_Pnumber = closelist[last_Pnumber][1];
            }
            robot1->path_pulling = chooseCargo->path_point;
            robot1->path_total = merge_in_order(robot1->path_total, chooseCargo->path_point);
            robot1->path_direction = merge_in_order(robot1->path_direction, chooseCargo->path_direction);
            //修改货物状态
            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                if (iter->item_id == chooseCargo->item_id) {
                    iter->status = 1;
                }
            }
            robot1->task_allocated = true;
            robot1->choose_item_id = chooseCargo->item_id;
            robot1->target_berth = chooseCargo->nearestBerthNumber;
            robot1->carried_money = chooseCargo->money;
        }
    }
}


/*
********* 核心函数 ***********
* 多机器人死锁检测与路径重规划函数
*/

void A_star_plus_path_rearrange(Robot robots[],
    int main_robot_number,
    int endP_number,
    int robots_paths[][500],
    int Node_movable_node[][4],
    list<int> *rearrange_path_points,
    list<int> *rearrange_path_directions,
    bool* no_solution)
{
    *no_solution = false;
    int x_start = robots[main_robot_number].x;
    int y_start = robots[main_robot_number].y;
    //Astar_plus部分
    int startP_number = x_start * n + y_start;
    list<int*> openlist; // [point,time,min_r,lastNumber,direction]
    int startP[5] = { startP_number,0,0,-1,-1 };
    list<int*> openlist_new;
    int closelist[n * n][4] = { 0 };//[status,min_r,last_node,directions]
    for (int i = 0; i < n * n; i++) {
        closelist[i][1] = INT_MAX;
    }
    list<int> in_clostlist;
    openlist.push_back(startP);
    int nowPoint = startP_number;
    int time = -1;
    int min_r = -1;
    int lastPoint = -1;
    int direction = -1;
    int T_Limit = 500;
    list<int> reachable_list;
    list<int> deadlock_list;
    list<int*> t_transfer;
    while (true) {
        if (!openlist.empty()) {
            auto point1 = openlist.begin();
            nowPoint = **point1;
            time = *(*point1 + 1);
            min_r = *(*point1 + 2);
            lastPoint = *(*point1 + 3);
            direction = *(*point1 + 4);
            if (nowPoint == lastPoint) {
                int a = 1;
            }
            openlist.pop_front();
            bool can_new_points = true;
            if (time > 0) {
                for (int i = 0; i != robot_num; i++) {
                    if (i != main_robot_number) {
                        int lastPoint_otherRobot;
                        if (time == 1) {
                            lastPoint_otherRobot = robots[i].x * n + robots[i].y;
                        }
                        else {
                            lastPoint_otherRobot = robots_paths[i][time - 2];
                        }
                        int nowPoint_otherRobot = robots_paths[i][time - 1];
                        //判断死锁
                        if (nowPoint == lastPoint_otherRobot && lastPoint == nowPoint_otherRobot) {
                            can_new_points = false;
                            closelist[nowPoint][0] = 2;
                            bool can_new_deadlock = true;
                            for (auto iter = deadlock_list.begin(); iter != deadlock_list.end(); iter++) {
                                if (*iter == nowPoint) {
                                    can_new_deadlock = false;
                                    break;
                                }
                            }
                            if (can_new_deadlock){
                                deadlock_list.push_back(nowPoint);
                            }
                        }
                        //判断冲撞
                        else if (nowPoint == nowPoint_otherRobot) {
                            can_new_points = false;
                            closelist[nowPoint][0] = 2;
                            bool can_new_deadlock = true;
                            for (auto iter = deadlock_list.begin(); iter != deadlock_list.end(); iter++) {
                                if (*iter == nowPoint) {
                                    can_new_deadlock = false;
                                    break;
                                }
                            }
                            if (can_new_deadlock) {
                                deadlock_list.push_back(nowPoint);
                            }
                        }
                    }
                }
            }
            if (can_new_points) {
                //可抵达
                bool can_new_reachable = true;
                for (auto iter = reachable_list.begin(); iter != reachable_list.end(); iter++) {
                    if (*iter == nowPoint) {
                        can_new_reachable = false;
                        break;
                    }
                }
                if (can_new_reachable) {
                    reachable_list.push_back(nowPoint);
                }
                closelist[nowPoint][0] = 1;
                closelist[nowPoint][1] = min_r;
                closelist[nowPoint][2] = lastPoint;
                closelist[nowPoint][3] = direction;
                int* tt = new int[4];
                tt[0] = nowPoint;
                tt[1] = time;
                tt[2] = lastPoint;
                tt[3] = direction;
                if (nowPoint == lastPoint) {
                    int a = 1;
                }
                t_transfer.push_back(tt);
                for (int j = 0; j < 4; j++) {
                    if (Node_movable_node[nowPoint][j] != -1) {
                        int tgt = Node_movable_node[nowPoint][j];
                        bool can_new_point1 = true;
                        if (!in_clostlist.empty()) {
                            for (auto iter = in_clostlist.begin(); iter != in_clostlist.end(); iter++) {
                                if (*iter == tgt) {
                                    can_new_point1 = false;
                                    break;
                                }
                            }
                        }
                        if (min_r + 1 >= closelist[tgt][1]) {
                            can_new_point1 = false;
                        }
                        if (can_new_point1) {
                            int *P = new int[5];
                            P[0] = tgt;
                            P[1] = time + 1;
                            P[2] = min_r + 1;
                            P[3] = nowPoint;
                            P[4] = j;
                            if (P[0] == P[3]) {
                                int a = 1;
                            }
                            openlist_new.push_back(P);
                            in_clostlist.push_back(tgt);
                        }
                    }
                }
            }
            if (closelist[endP_number][0] == 1) {
                break;
            }
        }
        else {
            time++;
            if (time > T_Limit)
            {
                *no_solution = true;
                break;
            }
            // 检测可抵达节点是否需要更新为阻塞
            //reachable_list.unique();
            if (!reachable_list.empty()) {
                for (auto iter = reachable_list.begin(); iter != reachable_list.end();)
                {
                    bool erased = false;
                    for (int i = 0; i != robot_num; i++) {
                        if (i != main_robot_number) {
                            if (*iter == robots_paths[i][time - 1]) {
                                deadlock_list.push_back(*iter);
                                closelist[*iter][0] = 2;
                                iter = reachable_list.erase(iter);
                                erased = true;
                                if (iter == reachable_list.end()) {
                                    break;
                                }
                            }
                        }
                    }
                    if (!erased) {
                        iter++;
                    }
                }
            }
            // 检测阻塞节点是否能够更新为可抵达
            //deadlock_list.unique();
            if (!deadlock_list.empty()) {
                for (auto iter = deadlock_list.begin(); iter != deadlock_list.end();) {
                    bool erased = false;
                    bool can_renew = true;
                    for (int i = 0; i != robot_num; i++) {
                        if (i != main_robot_number) {
                            if (*iter == robots_paths[i][time - 1]) {
                                can_renew = false;
                            }
                        }
                    }
                    int min_R = 99999;
                    int* P = new int[5];
                    *P = -1;
                    for (int j = 0; j < 4; j++) {
                        if (Node_movable_node[*iter][j] != -1 && closelist[Node_movable_node[*iter][j]][0] == 1) {
                            if (min_R > closelist[Node_movable_node[*iter][j]][0] + 1) {
                                int tgt = Node_movable_node[*iter][j];
                                min_R = closelist[Node_movable_node[*iter][j]][0] + 1;
                                int direction;
                                if (j == 0) {
                                    direction = 1;
                                }
                                else if (j == 1) {
                                    direction = 0;
                                }
                                else if (j == 2) {
                                    direction = 3;
                                }
                                else {
                                    direction = 2;
                                }

                                P[0] = *iter;
                                P[1] = time;
                                P[2] = min_R;
                                P[3] = tgt;
                                P[4] = direction;
                                if (P[0] == P[3]) {
                                    int a = 1;
                                }
                            }
                        }
                    }
                    if (*P != -1) {
                        openlist_new.push_back(P);
                        iter = deadlock_list.erase(iter);
                        erased = true;
                        if (iter == deadlock_list.end()) {
                            break;
                        }
                    }
                    if (!erased) {
                        iter++;
                    }
                }
            }
            if (openlist.empty() && !openlist_new.empty()) {
                openlist = openlist_new;
                openlist_new.clear();
                in_clostlist.clear();
            }
        }
    }
    if (!*no_solution) {
        //回溯路径
        int t_reverse = time;
        int last_P = endP_number;
        int direction = closelist[last_P][3];
        //rearrange_path_points->push_front(last_P);
        //rearrange_path_directions->push_front(direction);
        while (t_reverse != 0) {
            bool last_time_transferred = false;
            for (auto iter = t_transfer.begin(); iter != t_transfer.end();) {
                bool erased = false;
                if (**iter == last_P && *(*iter + 1) == t_reverse) {
                    nowPoint = **iter;
                    last_P = *(*iter + 2);
                    direction = *(*iter + 3);
                    rearrange_path_points->push_front(nowPoint);
                    rearrange_path_directions->push_front(direction);
                    iter = t_transfer.erase(iter);
                    erased = true;
                    last_time_transferred = true;
                }
                if (iter == t_transfer.end()) {
                    break;
                }
                if (*(*iter + 1) > t_reverse) {
                    iter = t_transfer.erase(iter);
                    erased = true;
                }
                if (!erased) {
                    iter++;
                }
            }
            if (!last_time_transferred) {
                rearrange_path_points->push_front(last_P);
                rearrange_path_directions->push_front(-1);
            }
            t_reverse--;
        }

    }
}
void SolveDeadlock(Robot robots[], int scan_range, int Node_movable_node[][4]) {
    struct need_to_solve_pair {
        int robot1;
        int robot2;
        int robot1_target;
        int robot2_target;
        need_to_solve_pair() {}
        need_to_solve_pair(int Robot1, int Robot2 , int Robot1_target, int Robot2_target) {
            this->robot1 = Robot1;
            this->robot2 = Robot2;
            this->robot1_target=Robot1_target;
            this->robot2_target = Robot2_target;
        }
    };
    list<need_to_solve_pair> DeadlockList;
    bool robot_path_lower[robot_num] = { false };
    int robots_paths[robot_num][500];
    for (int j = 0; j < robot_num; j++) {
        if (!robots[j].path_total.empty()){
            auto iter = robots[j].path_total.begin();
            bool path_end = false;
            int path_end_number = -1;
            for (int i = 0; i < 500; i++) {
                if (!path_end) {
                    robots_paths[j][i] = *iter;
                    iter++;
                    if (iter == robots[j].path_total.end()) {
                        path_end = true;
                        iter--;
                        path_end_number = *iter;
                    }
                }
                else {
                    robots_paths[j][i] = path_end_number;
                }
            }
        }
        else {
            for (int i = 0; i < 500; i++) {
                robots_paths[j][i] = robots[j].x * n + robots[j].y;
            }
            robot_path_lower[j] = false;
        }
    }
    
    for (int j = 0; j < robot_num - 1; j++) {
        if (!robots[j].active) {
            continue;
        }
        list<int> robot1_path;
        int robot1_taskTime;
        int robot1_target;
        if (robots[j].carried) {
            robot1_path = robots[j].path_pulling;
        }
        else {
            robot1_path = robots[j].path_getting;
        }
        if (robot1_path.size() > scan_range) {
            robot1_taskTime = scan_range;
            robot1_target = robots_paths[j][scan_range - 1];
        }
        else {
            robot1_taskTime = robot1_path.size();
            robot1_target = robots_paths[j][robot1_taskTime - 1];
        }
        for (int k = j+1; k < robot_num; k++) {
            if (!robots[k].active) {
                continue;
            }
            list<int> robot2_path;
            int robot2_taskTime;
            int robot2_target;
            if (robots[k].carried) {
                robot2_path = robots[k].path_pulling;
            }
            else {
                robot2_path = robots[k].path_getting;
            }
            if (robot2_path.size() > scan_range) {
                robot2_taskTime = scan_range;
                robot2_target = robots_paths[k][scan_range - 1];
            }
            else {
                robot2_taskTime = robot2_path.size();
                robot2_target = robots_paths[k][robot2_taskTime - 1];
            }
            bool need_to_continue = false;
            for (auto iter = DeadlockList.begin(); iter != DeadlockList.end(); iter++) {
                if (iter->robot1 == j && iter->robot2 == k) {
                    need_to_continue = true;
                    break;
                }
            }
            if (need_to_continue) {
                continue;
            }
            //检测是否存在碰撞路径
            for (int i = 0; i < scan_range; i++) {
                if (robots_paths[j][i] == robots_paths[k][i]) {
                    if (robot1_taskTime == scan_range && robot2_taskTime == scan_range) {
                        need_to_solve_pair clash = need_to_solve_pair(j, k, robot1_target, robot2_target);
                        DeadlockList.push_back(clash);
                        break;
                    }
                    else{
                        if (robot1_taskTime < robot2_taskTime) {
                            need_to_solve_pair clash = need_to_solve_pair(j, k, -1, robot2_target);
                            DeadlockList.push_back(clash);
                            break;
                        }
                        else {
                            need_to_solve_pair clash = need_to_solve_pair(j, k, robot1_target, -1);
                            DeadlockList.push_back(clash);
                            break;
                        }
                    }
                }
            }
            //检测是否存在死锁路径
            for (int i = 0; i < scan_range -1; i++) {
                if (robots_paths[j][i] == robots_paths[k][i + 1] && robots_paths[j][i + 1] == robots_paths[k][i]) {
                    if (robot1_taskTime == scan_range && robot2_taskTime == scan_range) {
                        need_to_solve_pair deadlock = need_to_solve_pair(j, k, robot1_target, robot2_target);
                        DeadlockList.push_back(deadlock);
                        break;
                    }
                    else {
                        if (robot1_taskTime < robot2_taskTime) {
                            need_to_solve_pair deadlock = need_to_solve_pair(j, k, -1, robot2_target);
                            DeadlockList.push_back(deadlock);
                            break;
                        }
                        else {
                            need_to_solve_pair deadlock = need_to_solve_pair(j, k, robot1_target, -1);
                            DeadlockList.push_back(deadlock);
                            break;
                        }
                    }
                }
            }
        }
    }


    //路径重规划
    if (!DeadlockList.empty()) {
        for (auto iter = DeadlockList.begin(); iter != DeadlockList.end(); iter++) {
            bool no_solution = false;
            list<int> rearrange_path_points;
            list<int> rearrange_path_directions;
            int main_robot_number;
            int endP_number;
            if (iter->robot1_target == -1) {
                main_robot_number = iter->robot2;
                endP_number = iter->robot2_target;
                A_star_plus_path_rearrange(robots, main_robot_number, endP_number, robots_paths, Node_movable_node, &rearrange_path_points, &rearrange_path_directions, &no_solution);
            }
            else if (iter->robot2_target == -1) {
                main_robot_number = iter->robot1;
                endP_number = iter->robot1_target;
                A_star_plus_path_rearrange(robots, main_robot_number, endP_number, robots_paths, Node_movable_node, &rearrange_path_points, &rearrange_path_directions, &no_solution);
            }
            else {
                bool successful_rearrange = false;
                // 重规划robot1
                main_robot_number = iter->robot1;
                endP_number = iter->robot1_target;
                A_star_plus_path_rearrange(robots, main_robot_number, endP_number, robots_paths, Node_movable_node, &rearrange_path_points, &rearrange_path_directions, &no_solution);
                // 若 robot1 规划失败，尝试重规划robot2
                if (no_solution) {
                    main_robot_number = iter->robot2;
                    endP_number = iter->robot2_target;
                    A_star_plus_path_rearrange(robots, main_robot_number, endP_number, robots_paths, Node_movable_node, &rearrange_path_points, &rearrange_path_directions, &no_solution);
                }
            }
            if (!no_solution){
                if (robots[main_robot_number].carried) {
                    auto iter3 = robots[main_robot_number].path_total.begin();
                    auto iter4 = robots[main_robot_number].path_direction.begin();
                    for (auto iter2 = robots[main_robot_number].path_pulling.begin();
                        iter2 != robots[main_robot_number].path_pulling.end();) {
                        if (*iter2 == endP_number) {
                            iter2 = robots[main_robot_number].path_pulling.erase(iter2);
                            robots[main_robot_number].path_total.erase(iter3);
                            robots[main_robot_number].path_direction.erase(iter4);
                            break;
                        }
                        iter2 = robots[main_robot_number].path_pulling.erase(iter2);
                        iter3 = robots[main_robot_number].path_total.erase(iter3);
                        iter4 = robots[main_robot_number].path_direction.erase(iter4);
                    }
                    list<int> rearrange_path_points1 = rearrange_path_points;
                    rearrange_path_points = merge_in_order(rearrange_path_points, robots[main_robot_number].path_pulling);
                    rearrange_path_points1 = merge_in_order(rearrange_path_points1, robots[main_robot_number].path_total);
                    rearrange_path_directions = merge_in_order(rearrange_path_directions, robots[main_robot_number].path_direction);
                    robots[main_robot_number].path_pulling = rearrange_path_points;
                    robots[main_robot_number].path_total = rearrange_path_points1;
                    robots[main_robot_number].path_direction = rearrange_path_directions;
                }
                else {
                    auto iter3 = robots[main_robot_number].path_total.begin();
                    auto iter4 = robots[main_robot_number].path_direction.begin();
                    for (auto iter2 = robots[main_robot_number].path_getting.begin();
                        iter2 != robots[main_robot_number].path_getting.end();) {
                        if (*iter2 == endP_number) {
                            iter2 = robots[main_robot_number].path_getting.erase(iter2);
                            robots[main_robot_number].path_total.erase(iter3);
                            robots[main_robot_number].path_direction.erase(iter4);
                            break;
                        }
                        iter2 = robots[main_robot_number].path_getting.erase(iter2);
                        iter3 = robots[main_robot_number].path_total.erase(iter3);
                        iter4 = robots[main_robot_number].path_direction.erase(iter4);
                    }
                    list<int> rearrange_path_points1 = rearrange_path_points;
                    rearrange_path_points = merge_in_order(rearrange_path_points, robots[main_robot_number].path_getting);
                    rearrange_path_points1 = merge_in_order(rearrange_path_points1, robots[main_robot_number].path_total);
                    rearrange_path_directions = merge_in_order(rearrange_path_directions, robots[main_robot_number].path_direction);
                    robots[main_robot_number].path_getting = rearrange_path_points;
                    robots[main_robot_number].path_total = rearrange_path_points1;
                    robots[main_robot_number].path_direction = rearrange_path_directions;
                }
            }

        }
    }
}



/*
********* 核心函数 ***********
* 路径预规划函数
* 对存储路径即将走完的机器人进行路径预规划
*/
void path_Preplan(Robot robots[], int limit_time, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged) 
{
    struct openlist_unit {
        int pointNumber;
        int lastPoint;
        int diRection;
        openlist_unit() {}
        openlist_unit(int pointnumber, int lastpoint, int direction) {
            this->lastPoint = lastpoint;
            this->pointNumber = pointnumber;
            this->diRection = direction;
        }
    };
    bool rearranged = false;
    for (int i = 0; i < robot_num; i++) {
        if (robots[i].active) {
            while (robots[i].path_total.size() < limit_time) {
                bool successful_plan = false;
                int startP_number;
                if (!robots[i].path_total.empty()){
                    startP_number = robots[i].path_total.back();
                }
                else {
                    startP_number = robots[i].x * n + robots[i].y;
                }
                list<Cargo> Cargolist_new;
                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                    if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
                    {
                        Cargolist_new.push_back(*iter);
                    }
                }
                if (Cargolist_new.empty()) {
                    bool successful_plan = false;
                }
                rearranged = true;
                //Astar部分
                list<openlist_unit> openlist;
                list<openlist_unit> openlist_new;
                int closelist[n * n][3] = { 0 };
                openlist_unit openP = openlist_unit(startP_number, -1, -1);
                openlist.push_back(openP);
                int distance = -1;
                double max_value_distance_ratio = 0;
                Cargo* chooseCargo = new Cargo();
                int max_scan_distance = 999;
                if (!Cargolist_new.empty()) {
                    while (true) {
                        if (openlist.empty()) {
                            break;
                        }
                        distance++;
                        if (distance > max_scan_distance) {
                            break;
                        }
                        for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
                            if (closelist[iter->pointNumber][0] == 0) {
                                closelist[iter->pointNumber][0] = 1;
                                closelist[iter->pointNumber][1] = iter->lastPoint;
                                closelist[iter->pointNumber][2] = iter->diRection;
                                for (int j = 0; j < 4; j++) {
                                    if (Node_movable_node[iter->pointNumber][j] != -1) {
                                        openP.pointNumber = Node_movable_node[iter->pointNumber][j];
                                        openP.lastPoint = iter->pointNumber;
                                        openP.diRection = j;
                                        openlist_new.push_back(openP);
                                    }
                                }
                            }
                            bool scanRatio = false;
                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
                                // 剔除无法获取的货物
                                if (iter2->surplus_zheng - distance < 20) {
                                    iter2 = Cargolist_new.erase(iter2);
                                }
                                // 检测是否可以取到货物
                                else if (iter->pointNumber == iter2->point_number) {
                                    successful_plan = true;
                                    //计算价距比
                                    iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                                    if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                        chooseCargo->point_number = iter2->point_number;
                                        chooseCargo->item_id = iter2->item_id;
                                        chooseCargo->path_point = iter2->path_point;
                                        chooseCargo->path_direction = iter2->path_direction;
                                        chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                        chooseCargo->money = iter2->money;
                                        max_value_distance_ratio = iter2->value_distance_ratio;
                                        scanRatio = true;
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                    else {
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                }
                                else {
                                    iter2++;
                                }
                            }
                            if (scanRatio) {
                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                    if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                    else {
                                        int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
                                        if (over_distance < max_scan_distance) {
                                            max_scan_distance = over_distance;
                                        }
                                        iter2++;
                                    }
                                }
                            }
                        }
                        openlist = openlist_new;
                        openlist_new.clear();
                    }
                }
                //回溯路径
                if (successful_plan){
                    *task_arranged = true;
                    robots[i].choose_item_id_next.push_back(chooseCargo->item_id);
                    list<int> path_getting_next_plus;
                    list<int> path_pulling_next_plus;
                    list<int> path_total_plus;
                    list<int> direction_total_plus;
                    int last_Pnumber = chooseCargo->point_number;
                    while (closelist[last_Pnumber][1] != -1) {
                        path_getting_next_plus.push_front(last_Pnumber);
                        path_total_plus.push_front(last_Pnumber);
                        direction_total_plus.push_front(closelist[last_Pnumber][2]);
                        last_Pnumber = closelist[last_Pnumber][1];
                    }
                    path_pulling_next_plus = chooseCargo->path_point;
                    robots[i].path_getting_next.push_back(path_getting_next_plus);
                    robots[i].path_pulling_next.push_back(path_pulling_next_plus);
                    robot[i].path_total = merge_in_order(robot[i].path_total, path_total_plus);
                    robot[i].path_total = merge_in_order(robot[i].path_total, chooseCargo->path_point);
                    robot[i].path_direction = merge_in_order(robot[i].path_direction, direction_total_plus);
                    robot[i].path_direction = merge_in_order(robot[i].path_direction, chooseCargo->path_direction);
                    robot[i].target_berth_next.push_back(chooseCargo->nearestBerthNumber);
                    robot[i].carried_money_next.push_back(chooseCargo->money);
                    //修改货物状态
                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                        if (iter->item_id == chooseCargo->item_id) {
                            iter->status = 1;
                        }
                    }
                    robot[i].task_allocated = true;
                    if (robot[i].path_total.size() > limit_time) {
                        break;
                    }
                }
                else {
                    // 没有可行的规划路径  建立一个虚拟取货路径

                    rearranged = true;
                    //Astar部分
                    list<openlist_unit> openlist;
                    list<openlist_unit> openlist_new;
                    int closelist[n * n][3] = { 0 };
                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
                    openlist.push_back(openP);
                    int distance = -1;
                    Cargo* chooseCargo = new Cargo();
                    chooseCargo->money = 0;
                    int max_scan_distance = 999;
                    bool successful_plan = false;
                    while (true) {
                        if (openlist.empty()) {
                            break;
                        }
                        distance++;
                        if (distance == limit_time) {
                            for (auto iter = openlist.begin(); iter != openlist.end(); iter++) {
                                if (closelist[iter->pointNumber][0] == 0) {
                                    closelist[iter->pointNumber][0] = 1;
                                    closelist[iter->pointNumber][1] = iter->lastPoint;
                                    closelist[iter->pointNumber][2] = iter->diRection;
                                    chooseCargo->point_number = iter->pointNumber;
                                    chooseCargo->item_id = -1;
                                    chooseCargo->nearestBerthNumber = -1;
                                    successful_plan = true;
                                    break;
                                }
                            }
                            break;
                        }
                        for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
                            if (closelist[iter->pointNumber][0] == 0) {
                                closelist[iter->pointNumber][0] = 1;
                                closelist[iter->pointNumber][1] = iter->lastPoint;
                                closelist[iter->pointNumber][2] = iter->diRection;
                                for (int j = 0; j < 4; j++) {
                                    if (Node_movable_node[iter->pointNumber][j] != -1) {
                                        openP.pointNumber = Node_movable_node[iter->pointNumber][j];
                                        openP.lastPoint = iter->pointNumber;
                                        openP.diRection = j;
                                        openlist_new.push_back(openP);
                                    }
                                }
                            }
                        }
                        openlist = openlist_new;
                        openlist_new.clear();
                    }
                    //回溯路径
                    if (successful_plan) {
                        *task_arranged = true;
                        robots[i].choose_item_id_next.push_back(chooseCargo->item_id);
                        list<int> path_getting_next_plus;
                        list<int> path_pulling_next_plus;
                        list<int> path_total_plus1;
                        list<int> path_total_plus2;
                        list<int> direction_total_plus1;
                        list<int> direction_total_plus2;
                        int last_Pnumber = chooseCargo->point_number;
                        while (closelist[last_Pnumber][1] != -1) {
                            path_getting_next_plus.push_front(last_Pnumber);
                            path_pulling_next_plus.push_back(last_Pnumber);
                            path_total_plus1.push_front(last_Pnumber);
                            path_total_plus2.push_back(last_Pnumber);
                            direction_total_plus1.push_front(closelist[last_Pnumber][2]);
                            if (closelist[last_Pnumber][2] == 0) {
                                direction_total_plus2.push_back(1);
                            }
                            else if(closelist[last_Pnumber][2] == 1){
                                direction_total_plus2.push_back(0);
                            }
                            else if (closelist[last_Pnumber][2] == 2) {
                                direction_total_plus2.push_back(3);
                            }
                            else {
                                direction_total_plus2.push_back(2);
                            }
                            last_Pnumber = closelist[last_Pnumber][1];
                        }
                        path_pulling_next_plus.pop_front();
                        path_total_plus2.pop_front();
                        direction_total_plus2.pop_back();
                        robots[i].path_getting_next.push_back(path_getting_next_plus);
                        robots[i].path_pulling_next.push_back(path_pulling_next_plus);
                        robot[i].path_total = merge_in_order(robot[i].path_total, path_total_plus1);
                        robot[i].path_total = merge_in_order(robot[i].path_total, path_total_plus2);
                        robot[i].path_direction = merge_in_order(robot[i].path_direction, direction_total_plus1);
                        robot[i].path_direction = merge_in_order(robot[i].path_direction, direction_total_plus2);
                        robot[i].target_berth_next.push_back(chooseCargo->nearestBerthNumber);
                        robot[i].carried_money_next.push_back(chooseCargo->money);
                        //修改货物状态
                        robot[i].task_allocated = true;
                        if (robot[i].path_total.size() > limit_time) {
                            break;
                        }
                    }

                    break;
                }
            }
            //if (rearranged) {
            //    break;
            //}
        }
    }
}

/*
********* 核心函数 ***********
* 优化搬运目标函数
* 针对已有搬运任务的机器人，若其未携带货物，扫描其他货物并考虑更换更优的搬运目标
*/
void optimize_task(Robot robots[], int start_rearrange_robot_number, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged)
{
    struct openlist_unit {
        int pointNumber;
        int lastPoint;
        int diRection;
        openlist_unit() {}
        openlist_unit(int pointnumber, int lastpoint, int direction) {
            this->lastPoint = lastpoint;
            this->pointNumber = pointnumber;
            this->diRection = direction;
        }
    };
    int K = start_rearrange_robot_number;
    int rearrange_robot_list[robot_num];
    for (int i = 0; i < robot_num; i++) {
        rearrange_robot_list[i] = K;
        if (K == robot_num - 1) {
            K = 0;
        }
        else {
            K++;
        }
    }
    // Step1 : 优先重规划虚拟搬运路径的机器人
    bool rearranged = false;
    int rearranged_robot_number = -1;
    for (int m = 0; m < robot_num; m++) {
        int i = rearrange_robot_list[m];
        if (robot[i].choose_item_id == -1) {
            int startP_number;
            startP_number = robots[i].x * n + robots[i].y;
            list<Cargo> Cargolist_new;
            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
                {
                    Cargolist_new.push_back(*iter);
                }
            }
            if (Cargolist_new.empty()) {
                break;
            }
            rearranged = true;
            //Astar部分
            list<openlist_unit> openlist;
            list<openlist_unit> openlist_new;
            int closelist[n * n][3] = { 0 };
            openlist_unit openP = openlist_unit(startP_number, -1, -1);
            openlist.push_back(openP);
            int distance = -1;
            double max_value_distance_ratio = 0;
            Cargo* chooseCargo = new Cargo();
            int max_scan_distance = 999;
            bool successful_plan = false;
            while (true) {
                if (openlist.empty()) {
                    break;
                }
                distance++;
                if (distance > max_scan_distance) {
                    break;
                }
                for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
                    if (closelist[iter->pointNumber][0] == 0) {
                        closelist[iter->pointNumber][0] = 1;
                        closelist[iter->pointNumber][1] = iter->lastPoint;
                        closelist[iter->pointNumber][2] = iter->diRection;
                        for (int j = 0; j < 4; j++) {
                            if (Node_movable_node[iter->pointNumber][j] != -1) {
                                openP.pointNumber = Node_movable_node[iter->pointNumber][j];
                                openP.lastPoint = iter->pointNumber;
                                openP.diRection = j;
                                openlist_new.push_back(openP);
                            }
                        }
                    }
                    bool scanRatio = false;
                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
                        // 剔除无法获取的货物
                        if (iter2->surplus_zheng - distance < 10) {
                            iter2 = Cargolist_new.erase(iter2);
                        }
                        // 检测是否可以取到货物
                        else if (iter->pointNumber == iter2->point_number) {
                            successful_plan = true;
                            //计算价距比
                            iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                            if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                chooseCargo->point_number = iter2->point_number;
                                chooseCargo->item_id = iter2->item_id;
                                chooseCargo->path_point = iter2->path_point;
                                chooseCargo->path_direction = iter2->path_direction;
                                chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                chooseCargo->money = iter2->money;
                                max_value_distance_ratio = iter2->value_distance_ratio;
                                scanRatio = true;
                                iter2 = Cargolist_new.erase(iter2);
                            }
                            else {
                                iter2 = Cargolist_new.erase(iter2);
                            }
                        }
                        else {
                            iter2++;
                        }
                    }
                    if (scanRatio) {
                        for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                            if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
                                iter2 = Cargolist_new.erase(iter2);
                            }
                            else {
                                int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
                                if (over_distance < max_scan_distance) {
                                    max_scan_distance = over_distance;
                                }
                                iter2++;
                            }
                        }
                    }
                }
                openlist = openlist_new;
                openlist_new.clear();
            }
            //回溯路径
            if (successful_plan) {
                *task_arranged = true;
                list<int> path_getting_alternative;
                list<int> path_pulling_alternative;
                list<int> path_total_alternative;
                list<int> direction_total_alternative;
                int last_Pnumber = chooseCargo->point_number;
                while (closelist[last_Pnumber][1] != -1) {
                    path_getting_alternative.push_front(last_Pnumber);
                    path_total_alternative.push_front(last_Pnumber);
                    direction_total_alternative.push_front(closelist[last_Pnumber][2]);
                    last_Pnumber = closelist[last_Pnumber][1];
                }
                path_pulling_alternative = chooseCargo->path_point;
                path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
                direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
                if (!robot[i].path_getting.empty()) {
                    auto iter2 = robot[i].path_total.begin();
                    auto iter3 = robot[i].path_direction.begin();
                    for (auto iter = robot[i].path_getting.begin(); iter != robot[i].path_getting.end();) {
                        iter = robot[i].path_getting.erase(iter);
                        iter2 = robot[i].path_total.erase(iter2);
                        iter3 = robot[i].path_direction.erase(iter3);
                    }
                }
                if (!robot[i].path_pulling.empty()) {
                    auto iter2 = robot[i].path_total.begin();
                    auto iter3 = robot[i].path_direction.begin();
                    for (auto iter = robot[i].path_pulling.begin(); iter != robot[i].path_pulling.end();) {
                        iter = robot[i].path_pulling.erase(iter);
                        iter2 = robot[i].path_total.erase(iter2);
                        iter3 = robot[i].path_direction.erase(iter3);
                    }
                }
                
                robots[i].path_getting = path_getting_alternative;
                robots[i].path_pulling = path_pulling_alternative;
                robot[i].path_total = path_total_alternative;
                robot[i].path_direction = direction_total_alternative;
                robot[i].path_getting_next.clear();
                robot[i].path_pulling_next.clear();
                for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
                    if (robot[i].choose_item_id == iter2->item_id) {
                        iter2->status = 0;
                    }
                    for (auto iter = robot[i].choose_item_id_next.begin(); iter != robot[i].choose_item_id_next.end(); iter++) {

                        if (*iter == iter2->item_id) {
                            iter2->status = 0;
                        }
                    }
                }
                robot[i].target_berth = chooseCargo->nearestBerthNumber;
                robot[i].carried_money = chooseCargo->money;
                robot[i].carried = false;
                //修改货物状态
                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                    if (iter->item_id == chooseCargo->item_id) {
                        iter->status = 1;
                    }
                }
                robot[i].choose_item_id = chooseCargo->item_id;
                robot[i].task_allocated = true;
            }
        }
    }

    // Step2: 检查是否存在机器人无法按时取货，若有，重新规划取货路径
    for (int m = 0; m < robot_num; m++) {
        int i = rearrange_robot_list[m];
        if (!robot[i].carried) {
            int now_cargo_surplus_zheng = 0;
            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                if (iter->item_id == robots[i].choose_item_id) {
                    now_cargo_surplus_zheng = iter->surplus_zheng;
                }
            }
            if (!robot[i].carried && robot[i].path_getting.size() > now_cargo_surplus_zheng) {
                int startP_number;
                startP_number = robots[i].x * n + robots[i].y;
                list<Cargo> Cargolist_new;
                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                    if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
                    {
                        Cargolist_new.push_back(*iter);
                    }
                }
                if (Cargolist_new.empty()) {
                    break;
                }
                rearranged = true;
                //Astar部分
                list<openlist_unit> openlist;
                list<openlist_unit> openlist_new;
                int closelist[n * n][3] = { 0 };
                openlist_unit openP = openlist_unit(startP_number, -1, -1);
                openlist.push_back(openP);
                int distance = -1;
                double max_value_distance_ratio = 0;
                Cargo* chooseCargo = new Cargo();
                int max_scan_distance = 999;
                bool successful_plan = false;
                while (true) {
                    if (openlist.empty()) {
                        break;
                    }
                    distance++;
                    if (distance > max_scan_distance) {
                        break;
                    }
                    for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
                        if (closelist[iter->pointNumber][0] == 0) {
                            closelist[iter->pointNumber][0] = 1;
                            closelist[iter->pointNumber][1] = iter->lastPoint;
                            closelist[iter->pointNumber][2] = iter->diRection;
                            for (int j = 0; j < 4; j++) {
                                if (Node_movable_node[iter->pointNumber][j] != -1) {
                                    openP.pointNumber = Node_movable_node[iter->pointNumber][j];
                                    openP.lastPoint = iter->pointNumber;
                                    openP.diRection = j;
                                    openlist_new.push_back(openP);
                                }
                            }
                        }
                        bool scanRatio = false;
                        for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
                            // 剔除无法获取的货物
                            if (iter2->surplus_zheng - distance < 10) {
                                iter2 = Cargolist_new.erase(iter2);
                            }
                            // 检测是否可以取到货物
                            else if (iter->pointNumber == iter2->point_number) {
                                successful_plan = true;
                                //计算价距比
                                iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                                if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                    chooseCargo->point_number = iter2->point_number;
                                    chooseCargo->item_id = iter2->item_id;
                                    chooseCargo->path_point = iter2->path_point;
                                    chooseCargo->path_direction = iter2->path_direction;
                                    chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                    chooseCargo->money = iter2->money;
                                    max_value_distance_ratio = iter2->value_distance_ratio;
                                    scanRatio = true;
                                    iter2 = Cargolist_new.erase(iter2);
                                }
                                else {
                                    iter2 = Cargolist_new.erase(iter2);
                                }
                            }
                            else {
                                iter2++;
                            }
                        }
                        if (scanRatio) {
                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
                                    iter2 = Cargolist_new.erase(iter2);
                                }
                                else {
                                    int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
                                    if (over_distance < max_scan_distance) {
                                        max_scan_distance = over_distance;
                                    }
                                    iter2++;
                                }
                            }
                        }
                    }
                    openlist = openlist_new;
                    openlist_new.clear();
                }
                //回溯路径
                if (successful_plan) {
                    *task_arranged = true;
                    list<int> path_getting_alternative;
                    list<int> path_pulling_alternative;
                    list<int> path_total_alternative;
                    list<int> direction_total_alternative;
                    int last_Pnumber = chooseCargo->point_number;
                    while (closelist[last_Pnumber][1] != -1) {
                        path_getting_alternative.push_front(last_Pnumber);
                        path_total_alternative.push_front(last_Pnumber);
                        direction_total_alternative.push_front(closelist[last_Pnumber][2]);
                        last_Pnumber = closelist[last_Pnumber][1];
                    }
                    path_pulling_alternative = chooseCargo->path_point;
                    path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
                    direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
                    if (!robot[i].path_getting.empty()) {
                        auto iter2 = robot[i].path_total.begin();
                        auto iter3 = robot[i].path_direction.begin();
                        for (auto iter = robot[i].path_getting.begin(); iter != robot[i].path_getting.end();) {
                            iter = robot[i].path_getting.erase(iter);
                            iter2 = robot[i].path_total.erase(iter2);
                            iter3 = robot[i].path_direction.erase(iter3);
                        }
                    }
                    if (!robot[i].path_pulling.empty()) {
                        auto iter2 = robot[i].path_total.begin();
                        auto iter3 = robot[i].path_direction.begin();
                        for (auto iter = robot[i].path_pulling.begin(); iter != robot[i].path_pulling.end();) {
                            iter = robot[i].path_pulling.erase(iter);
                            iter2 = robot[i].path_total.erase(iter2);
                            iter3 = robot[i].path_direction.erase(iter3);
                        }
                    }

                    robots[i].path_getting = path_getting_alternative;
                    robots[i].path_pulling = path_pulling_alternative;
                    robot[i].path_total = path_total_alternative;
                    robot[i].path_direction = direction_total_alternative;
                    robot[i].path_getting_next.clear();
                    robot[i].path_pulling_next.clear();
                    for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
                        if (robot[i].choose_item_id == iter2->item_id) {
                            iter2->status = 0;
                        }
                        for (auto iter = robot[i].choose_item_id_next.begin(); iter != robot[i].choose_item_id_next.end(); iter++) {

                            if (*iter == iter2->item_id) {
                                iter2->status = 0;
                            }
                        }
                    }
                    robot[i].target_berth = chooseCargo->nearestBerthNumber;
                    robot[i].carried_money = chooseCargo->money;
                    robot[i].carried = false;
                    //修改货物状态
                    int item_id_origin = robot[i].choose_item_id;
                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                        if (iter->item_id == item_id_origin) {
                            iter->status = 0;
                        }
                        if (iter->item_id == chooseCargo->item_id) {
                            iter->status = 1;
                        }
                    }
                    robot[i].task_allocated = true;
                    robot[i].choose_item_id = chooseCargo->item_id;
                    break;
                }
            }
        }
    }

    // Step3: 优化目前已有任务的机器人

    //if (!rearranged){
    //    for (int m = 0; m < robot_num; m++) {
    //        int i = rearrange_robot_list[m];
    //        if (!robots[i].carried) {
    //            int startP_number = robots[i].x * n + robots[i].y;
    //            list<Cargo> Cargolist_new;
    //            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
    //                if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
    //                {
    //                    Cargolist_new.push_back(*iter);
    //                }
    //            }
    //            if (Cargolist_new.empty()) {
    //                break;
    //            }
    //            rearranged = true;
    //            //Astar部分
    //            list<openlist_unit> openlist;
    //            list<openlist_unit> openlist_new;
    //            int closelist[n * n][3] = { 0 };
    //            openlist_unit openP = openlist_unit(startP_number, -1, -1);
    //            openlist.push_back(openP);
    //            int distance = -1;
    //            double max_value_distance_ratio = (double)robots[i].carried_money / (robots[i].path_getting.size() + robots[i].path_pulling.size());
    //            bool task_changed = false;
    //            Cargo* chooseCargo = new Cargo();
    //            int max_scan_distance = 999;
    //            while (true) {
    //                if (openlist.empty()) {
    //                    break;
    //                }
    //                distance++;
    //                if (distance > max_scan_distance) {
    //                    break;
    //                }
    //                for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
    //                    if (closelist[iter->pointNumber][0] == 0) {
    //                        closelist[iter->pointNumber][0] = 1;
    //                        closelist[iter->pointNumber][1] = iter->lastPoint;
    //                        closelist[iter->pointNumber][2] = iter->diRection;
    //                        for (int j = 0; j < 4; j++) {
    //                            if (Node_movable_node[iter->pointNumber][j] != -1) {
    //                                openP.pointNumber = Node_movable_node[iter->pointNumber][j];
    //                                openP.lastPoint = iter->pointNumber;
    //                                openP.diRection = j;
    //                                openlist_new.push_back(openP);
    //                            }
    //                        }
    //                    }
    //                    bool scanRatio = false;
    //                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
    //                        // 剔除无法获取的货物
    //                        if (iter2->surplus_zheng - distance < 10) {
    //                            iter2 = Cargolist_new.erase(iter2);
    //                        }
    //                        // 检测是否可以取到货物
    //                        else if (iter->pointNumber == iter2->point_number) {
    //                            //计算价距比
    //                            iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
    //                            if (iter2->value_distance_ratio > max_value_distance_ratio) {
    //                                task_changed = true;
    //                                chooseCargo->point_number = iter2->point_number;
    //                                chooseCargo->item_id = iter2->item_id;
    //                                chooseCargo->path_point = iter2->path_point;
    //                                chooseCargo->path_direction = iter2->path_direction;
    //                                chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
    //                                chooseCargo->money = iter2->money;
    //                                max_value_distance_ratio = iter2->value_distance_ratio;
    //                                scanRatio = true;
    //                                iter2 = Cargolist_new.erase(iter2);
    //                            }
    //                            else {
    //                                iter2 = Cargolist_new.erase(iter2);
    //                            }
    //                        }
    //                        else {
    //                            iter2++;
    //                        }
    //                    }
    //                    if (scanRatio) {
    //                        for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
    //                            if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
    //                                iter2 = Cargolist_new.erase(iter2);
    //                            }
    //                            else {
    //                                int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
    //                                if (over_distance < max_scan_distance) {
    //                                    max_scan_distance = over_distance;
    //                                }
    //                                iter2++;
    //                            }
    //                        }
    //                    }
    //                }
    //                openlist = openlist_new;
    //                openlist_new.clear();
    //            }
    //            //回溯路径
    //            if (task_changed) {
    //                *task_arranged = true;
    //                list<int> path_getting_alternative;
    //                list<int> path_pulling_alternative;
    //                list<int> path_total_alternative;
    //                list<int> direction_total_alternative;
    //                int last_Pnumber = chooseCargo->point_number;
    //                while (closelist[last_Pnumber][1] != -1) {
    //                    path_getting_alternative.push_front(last_Pnumber);
    //                    path_total_alternative.push_front(last_Pnumber);
    //                    direction_total_alternative.push_front(closelist[last_Pnumber][2]);
    //                    last_Pnumber = closelist[last_Pnumber][1];
    //                }
    //                path_pulling_alternative = chooseCargo->path_point;
    //                path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
    //                direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
    //                if (!robot[i].path_getting.empty()) {
    //                    auto iter2 = robot[i].path_total.begin();
    //                    auto iter3 = robot[i].path_direction.begin();
    //                    for (auto iter = robot[i].path_getting.begin(); iter != robot[i].path_getting.end();) {
    //                        iter = robot[i].path_getting.erase(iter);
    //                        iter2 = robot[i].path_total.erase(iter2);
    //                        iter3 = robot[i].path_direction.erase(iter3);
    //                    }
    //                }
    //                if (!robot[i].path_pulling.empty()) {
    //                    auto iter2 = robot[i].path_total.begin();
    //                    auto iter3 = robot[i].path_direction.begin();
    //                    for (auto iter = robot[i].path_pulling.begin(); iter != robot[i].path_pulling.end();) {
    //                        iter = robot[i].path_pulling.erase(iter);
    //                        iter2 = robot[i].path_total.erase(iter2);
    //                        iter3 = robot[i].path_direction.erase(iter3);
    //                    }
    //                }

    //                robots[i].path_getting = path_getting_alternative;
    //                robots[i].path_pulling = path_pulling_alternative;
    //                robot[i].path_total = path_total_alternative;
    //                robot[i].path_direction = direction_total_alternative;
    //                robot[i].path_getting_next.clear();
    //                robot[i].path_pulling_next.clear();
    //                for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
    //                    if (robot[i].choose_item_id == iter2->item_id) {
    //                        iter2->status = 0;
    //                    }
    //                    for (auto iter = robot[i].choose_item_id_next.begin(); iter != robot[i].choose_item_id_next.end(); iter++) {
    //                    
    //                        if (*iter == iter2->item_id) {
    //                            iter2->status = 0;
    //                        }
    //                    }
    //                }
    //                robot[i].target_berth = chooseCargo->nearestBerthNumber;
    //                robot[i].carried_money = chooseCargo->money;
    //                robot[i].carried = false;
    //                

    //                //修改货物状态
    //                int item_id_origin = robot[i].choose_item_id;
    //                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
    //                    if (iter->item_id == item_id_origin) {
    //                        iter->status = 0;
    //                    }
    //                    if (iter->item_id == chooseCargo->item_id) {
    //                        iter->status = 1;
    //                    }
    //                }
    //                robot[i].choose_item_id = chooseCargo->item_id;
    //                robot[i].task_allocated = true;
    //                break;
    //            }
    //        }
    //    }
    //}
}


/*
* 货船任务规划函数
*/
void BoatPlan(Boat boats[], Berth berths[], Robot robots[],int zhen_now) 
{
    // 检测船的状态、码头的货物，给出当前帧执行的指令。
    for (int num = 0; num < boat_num; num++) {
        //在虚拟点位并空闲时，向泊位处移动
        if (boat[num].status == 1 && boat[num].pos == -1) {
            boat[num].carried_money = 0;
            boat[num].total_cycle_number--;
            boat[num].need_to_ship = true;
            boat[num].shipping_target = boat[num].berths_to_go.front();
            // 若已经开始最后一个循环，发出信号，关闭其他泊位。
            if (boat[num].total_cycle_number == 0) {
                boat[num].is_final_cycle = true;
                for (auto iter = boat[num].berths_to_go.begin(); iter != boat[num].berths_to_go.end(); iter++) {
                    if (*iter != boat[num].final_berth) {
                        berth[*iter].closed = true;
                    }
                    else {
                        break;
                    }
                }
            }
        }

        if (!boat[num].at_berth && boat[num].status != 0 && boat[num].pos != -1) {
            boat[num].at_berth = true;
            boat[num].remaining_time = boat[num].remaining_time + (boat[num].once_carried_cargo_number / berth[boat[num].pos].loading_speed) + 3;
            boat[num].now_berth = boat[num].shipping_target;
            boat[num].shipping_target = -1;
        }

        // 若在泊位已经达到预定的停留时间，向下一个泊位移动或送货
        if (boat[num].at_berth && boat[num].remaining_time == 0) {
            if (boat[num].now_berth == boat[num].final_berth) {
                boat[num].at_berth = false;
                boat[num].need_to_go = true;
            }
            else {
                for (auto iter = boat[num].berths_to_go.begin(); iter != boat[num].berths_to_go.end(); iter++) {
                    if (*iter == boat[num].now_berth) {
                        boat[num].at_berth = false;
                        iter++;
                        boat[num].need_to_ship = true;
                        boat[num].shipping_target = *iter;
                        break;
                    }
                }
            }
        }
        // 若在泊位，但未达到预定的停留时间，则更新停留时间; 若下一个泊位货物较多，则提前前往下一个泊位
        if (!boat[num].is_final_cycle){
            if (boat[num].at_berth && boat[num].remaining_time != 0) {
                boat[num].remaining_time--;
                if (!berth[boat[num].now_berth].goods.empty()) {
                    auto iter = berth[boat[num].now_berth].goods.begin();
                    for (int i = 0; i < berth[boat[num].now_berth].loading_speed; i++) {
                        if (iter == berth[boat[num].now_berth].goods.end()) {
                            break;
                        }
                        boat[num].carried_money = boat[num].carried_money + *iter;
                        iter = berth[boat[num].now_berth].goods.erase(iter);
                    }
                }
                if (boat[num].now_berth != boat[num].final_berth) {
                    int next_berth_num = -1;
                    for (auto iter = boat[num].berths_to_go.begin(); iter != boat[num].berths_to_go.end(); iter++) {
                        if (*iter == boat[num].now_berth) {
                            iter++;
                            next_berth_num = *iter;
                            break;
                        }
                    }
                    if (berth[boat[num].now_berth].goods.size() == 0 && berth[next_berth_num].goods.size() / berth[next_berth_num].loading_speed + 1 > boat[num].once_carried_cargo_number / berth[next_berth_num].loading_speed + boat[num].remaining_time - 1) {
                        boat[num].at_berth = false;
                        boat[num].need_to_ship = true;
                        for (auto iter = boat[num].berths_to_go.begin(); iter != boat[num].berths_to_go.end(); iter++) {
                            if (*iter == boat[num].now_berth) {
                                iter++;
                                boat[num].shipping_target = *iter;
                                break;
                            }
                        }
                    }
                }
            }
        }
        //若为最后一个循环
        else {
            if (boat[num].at_berth) {
                if (boat[num].now_berth != boat[num].final_berth) {
                    if (!berth[boat[num].now_berth].goods.empty()) {
                        auto iter = berth[boat[num].now_berth].goods.begin();
                        for (int i = 0; i < berth[boat[num].now_berth].loading_speed; i++) {
                            if (iter == berth[boat[num].now_berth].goods.end()) {
                                break;
                            }
                            boat[num].carried_money = boat[num].carried_money + *iter;
                            iter = berth[boat[num].now_berth].goods.erase(iter);
                        }
                    }
                    else {
                        bool all_picked = true;
                        for (int i = 0; i < robot_num; i++) {
                            if (robots[i].choose_item_id == boat[num].now_berth) {
                                all_picked = false;
                            }
                        }
                        if (all_picked) {
                            boat[num].remaining_time = 0;
                        }
                    }
                }
                else {
                    if (zhen_now < total_zhen - berth[boat[num].now_berth].transport_time - 2) {
                        if (!berth[boat[num].now_berth].goods.empty()) {
                            auto iter = berth[boat[num].now_berth].goods.begin();
                            for (int i = 0; i < berth[boat[num].now_berth].loading_speed; i++) {
                                if (iter == berth[boat[num].now_berth].goods.end()) {
                                    break;
                                }
                                boat[num].carried_money = boat[num].carried_money + *iter;
                                iter = berth[boat[num].now_berth].goods.erase(iter);
                            }
                        }
                    }
                    else {
                        boat[num].at_berth = false;
                        boat[num].need_to_go = true;
                    }
                }
            }
        }

    }
}

//初始化函数     读取并预处理地图信息等
FILE* outputFile;
FILE* money_statistic;
void Init(int Node_movable_directions[][4], int Node_movable_node[][4], int Berth_information[][5], int Node_is_berth[], int* Boat_capacity, Robot robot_init[])
{
    // 读取地图信息，存储到Node_movable_directions中
    int Node_can_move[n * n];
    for (int i = 1; i <= n; i++)
    {
        scanf("%s", ch[i] + 1);
        fprintf(outputFile, "%s\n", ch[i] + 1);
    }
    int robotNumber = -1;
    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= n; j++)
        {
            if (ch[i][j] == 'B') {
                Node_is_berth[(i - 1) * n + j - 1] = 1;
            }
            if (ch[i][j] == '.' || ch[i][j] == 'A' || ch[i][j] == 'B')
            {
                Node_can_move[(i - 1) * n + j - 1] = 1;
                if (j < n) {
                    if (ch[i][j + 1] == '.' || ch[i][j + 1] == 'A' || ch[i][j + 1] == 'B') {
                        Node_movable_directions[(i - 1) * n + j - 1][0] = 1;
                        Node_movable_node[(i - 1) * n + j - 1][0] = (i - 1) * n + j;
                    }
                } 
                if (j > 1) {
                    if (ch[i][j - 1] == '.' || ch[i][j - 1] == 'A'|| ch[i][j - 1] == 'B') {
                        Node_movable_directions[(i - 1) * n + j - 1][1] = 1;
                        Node_movable_node[(i - 1) * n + j - 1][1] = (i - 1) * n + j - 2;
                    }
                }
                if (i > 1) {
                    if (ch[i - 1][j] == '.' || ch[i - 1][j] == 'A' || ch[i-1][j] == 'B') {
                        Node_movable_directions[(i - 1) * n + j - 1][2] = 1;
                        Node_movable_node[(i - 1) * n + j - 1][2] = (i - 2) * n + j - 1;
                    }
                }
                if (i < n) {
                    if (ch[i + 1][j] == '.' || ch[i + 1][j] == 'A' || ch[i+1][j] == 'B') {
                        Node_movable_directions[(i - 1) * n + j - 1][3] = 1;
                        Node_movable_node[(i - 1) * n + j - 1][3] = i * n + j - 1;
                    }
                }
                // 再记录初始机器人的位置信息
                if (ch[i][j] == 'A') {
                    robotNumber++;
                    robot_init[robotNumber].x = i - 1;
                    robot_init[robotNumber].y = j - 1;
                }
            }
            else {
                Node_can_move[(i - 1) * n + j - 1] = 0;
            }
        }
    }

    // 读取泊位信息，存储到Berth_information中    
    for(int i = 0; i < berth_num; i ++)
    {
        int id;
        scanf("%d", &id);
        scanf("%d%d%d%d", &berth[id].x, &berth[id].y, &berth[id].transport_time, &berth[id].loading_speed);
        fprintf(outputFile, "%d %d %d %d %d\n", id,berth[id].x, berth[id].y, berth[id].transport_time, berth[id].loading_speed);
        Berth_information[i][0] = id;
        Berth_information[i][1] = berth[id].x;
        Berth_information[i][2] = berth[id].y;
        Berth_information[i][3] = berth[id].transport_time;
        Berth_information[i][4] = berth[id].loading_speed;
        //if (berth[id].y > 0 && Node_can_move[berth[id].x * n + berth[id].y - 1] == 1) {
        //    Node_movable_directions[berth[id].x * n + berth[id].y][1] = 1;
        //    Node_movable_directions[berth[id].x * n + berth[id].y - 1][0] = 1;
        //    Node_movable_node[berth[id].x * n + berth[id].y][1] = berth[id].x * n + berth[id].y - 1;
        //    Node_movable_node[berth[id].x * n + berth[id].y - 1][0] = berth[id].x * n + berth[id].y;
        //}
        //if (berth[id].y < n-1 && Node_can_move[berth[id].x * n + berth[id].y + 1] == 1) {
        //    Node_movable_directions[berth[id].x * n + berth[id].y][0] = 1;
        //    Node_movable_directions[berth[id].x * n + berth[id].y + 1][1] = 1;
        //    Node_movable_node[berth[id].x * n + berth[id].y][0] = berth[id].x * n + berth[id].y + 1;
        //    Node_movable_node[berth[id].x * n + berth[id].y + 1][1] = berth[id].x * n + berth[id].y;
        //}
        //if (berth[id].x > 0 && Node_can_move[(berth[id].x - 1) * n + berth[id].y] == 1) {
        //    Node_movable_directions[berth[id].x * n + berth[id].y][2] = 1;
        //    Node_movable_directions[(berth[id].x - 1) * n + berth[id].y][3] = 1;
        //    Node_movable_node[berth[id].x * n + berth[id].y][2] = (berth[id].x - 1) * n + berth[id].y;
        //    Node_movable_node[(berth[id].x - 1) * n + berth[id].y][3] = berth[id].x * n + berth[id].y;
        //}
        //if (berth[id].x < n-1 && Node_can_move[(berth[id].x + 1) * n + berth[id].y] == 1) {
        //    Node_movable_directions[berth[id].x * n + berth[id].y][3] = 1;
        //    Node_movable_directions[(berth[id].x + 1) * n + berth[id].y][2] = 1;
        //    Node_movable_node[berth[id].x * n + berth[id].y][2] = (berth[id].x + 1) * n + berth[id].y;
        //    Node_movable_node[(berth[id].x + 1) * n + berth[id].y][2] = berth[id].x * n + berth[id].y;
        //}
        
    }
    for (int i = 0; i < berth_num; i++) {
        for (int j = 0; j < n * n; j++) {
            int x_point = j / n;
            int y_point = j % n;
            if (Node_is_berth[j] == 1) {
                if (x_point - berth[i].x <= 3 && x_point - berth[i].x >= 0 && y_point - berth[i].y <= 3 && y_point - berth[i].y >= 0) {
                    Node_is_berth[j] = i;
                }
            }
        }
    }

    /*
* 检测机器人可以抵达的所有泊位
* 如果该机器人不可抵达任何泊位，则该机器人无法执行任务，判断其为无效机器人。
*/
    scanBerthsCanBeReached(robot_init, berth, Node_movable_node);
    for (int i = 0; i < robot_num; i++) {
        for (int j = 0; j < berth_num; j++) {
            if (robot_init[i].berthsCanBeReached[j]) {
                robot_init[i].active = true;
            }
        }
    }

    // 记录船的容量
    scanf("%d", &boat_capacity);
    fprintf(outputFile, "%d\n", boat_capacity);
    *Boat_capacity = boat_capacity;

    for (int i = 0; i < boat_num; i++) {
        boat[i].capacity = boat_capacity;
        //if (boat_capacity > 100) {
        //    boat[i].once_carried_cargo_number = 50;
        //}
        //else {
        //    boat[i].once_carried_cargo_number = boat_capacity/2;
        //}
        boat[i].once_carried_cargo_number = boat_capacity;
    }

    /*
    * 试探性计算  找到5个最终港口
    */ 
    int distance_to_berth_test[berth_num][30];
    for (int i = 0; i < berth_num; i++) {
        for (int j = 0; j < 30; j++) {
            distance_to_berth_test[i][j] = n * 2;
        }
    }
    srand((unsigned)time(NULL));
    for (int num = 0; num < 30; num++) {
        while (true) {
            int point_number = rand() % (n * n);
            if (Node_movable_node[point_number][0] + Node_movable_node[point_number][1] + Node_movable_node[point_number][2] + Node_movable_node[point_number][3] != -4) {
                list<int> openlist;
                list<int> openlist_new;
                int closelist[n * n] = { 0 };
                openlist.push_back(point_number);
                int distance = -1;
                bool berth_reached[berth_num] = { false };
                while (true) {
                    if (openlist.empty()) {
                        break;
                    }
                    distance++;
                    bool all_reached = true;
                    for (auto iter = openlist.begin(); iter != openlist.end(); iter++) {
                        closelist[*iter] = 1;
                        int point_x = *iter / n;
                        int point_y = *iter % n;
                        for (int i = 0; i < berth_num; i++) {
                            if (!berth_reached[i] &&point_x - berth[i].x <= 3 && point_y - berth[i].y <= 3 && point_x - berth[i].x >= 0 && point_y - berth[i].y >= 0) {
                                berth_reached[i] = true;
                                distance_to_berth_test[i][num] = distance;
                            }
                        }
                        for (int j = 0; j < 4; j++) {
                            if (Node_movable_node[*iter][j] != -1 && closelist[Node_movable_node[*iter][j]] == 0) {
                                closelist[Node_movable_node[*iter][j]] = 1;
                                openlist_new.push_back(Node_movable_node[*iter][j]);
                            }
                        }
                    }
                    for (int i = 0; i < berth_num; i++) {
                        if (!berth_reached[i]) {
                            all_reached = false;
                        }
                    }
                    if (all_reached) {
                        break;
                    }
                    openlist = openlist_new;
                    openlist_new.clear();
                }
                break;
            }
        }
    }

    double test_score_berths[berth_num] = { 0 };
    for (int i = 0; i < berth_num; i++) {
        for (int j = 0; j < 30; j++) {
            test_score_berths[i] = test_score_berths[i] + distance_to_berth_test[i][j];
        }
    }
    for (int i = 0; i < berth_num; i++) {
        test_score_berths[i] = test_score_berths[i] / 30;
        test_score_berths[i] = test_score_berths[i] + berth[i].transport_time;
    }
    //冒泡排序
    int IDX_test_score_berths[berth_num] = {0};
    for (int i = 0; i < berth_num; i++) {
        IDX_test_score_berths[i] = i;
    }
    for (int i = 0; i < berth_num; i++)//外层循环是比较的轮数，数组内有10个数，那么就应该比较10-1=9轮
    {
        int temp;
        int IDX_temp;
        for (int j = 0; j < berth_num - i - 1; j++)//内层循环比较的是当前一轮的比较次数，例如：第一轮比较9-1=8次，第二轮比较9-2=7次
        {
            if (test_score_berths[j] > test_score_berths[j + 1])//相邻两个数如果逆序，则交换位置
            {
                temp = test_score_berths[j];
                IDX_temp = IDX_test_score_berths[j];
                test_score_berths[j] = test_score_berths[j + 1];
                IDX_test_score_berths[j] = IDX_test_score_berths[j + 1];
                test_score_berths[j + 1] = temp;
                IDX_test_score_berths[j + 1] = IDX_temp;
            }
        }
    }

    // 统计地图上的“区域”数量
    int block_num = 0;
    bool berths_included[berth_num] = { false };
    list<bool*> block_points;
    for (int i = 0; i < robot_num; i++) {
        for (int j = 0; j < berth_num; j++) {
            if (robot_init[i].berthsCanBeReached[j] && !berths_included[j]) {
                block_num = block_num + 1;
                berths_included[j] = true;
                block_points.push_back(robot_init[i].berthsCanBeReached);
                for (int k = 0; k < berth_num; k++) {
                    if (robot_init[i].berthsCanBeReached[k] && !berths_included[k]) {
                        berths_included[k] = true;
                    }
                }
            }
        }
    }

    //给至少每个区域分配一艘最终船只
    auto iter = block_points.begin();
    bool berths_choosed[berth_num] = { false };
    int choose_berth;
    for (int i = 0; i < boat_num; i++) {
        if (iter != block_points.end()) {
            for (int j = 0; j < berth_num; j++) {
                choose_berth = IDX_test_score_berths[j];
                if (berths_choosed[choose_berth]) {
                    continue;
                }
                if (*(*iter + choose_berth)) {
                    boat[i].final_berth = choose_berth;
                    boat[i].berths_to_go.push_front(choose_berth);
                    boat[i].cycle_total_time = boat[i].cycle_total_time + boat[i].once_carried_cargo_number / berth[choose_berth].loading_speed + 5 + berth[choose_berth].transport_time;
                    berths_choosed[choose_berth] = true;
                    break;
                }   
            }
            iter++;
        }
        else {
            for (int j = 0; j < berth_num; j++) {
                choose_berth = IDX_test_score_berths[j];
                if (berths_choosed[choose_berth]) {
                    continue;
                }
                boat[i].final_berth = choose_berth;
                boat[i].berths_to_go.push_front(choose_berth);
                boat[i].cycle_total_time = boat[i].cycle_total_time + boat[i].once_carried_cargo_number / berth[choose_berth].loading_speed + 5 + berth[choose_berth].transport_time;
                berths_choosed[choose_berth] = true;
                break;
            }
        }
    }


    //策略①
    //将其余泊位分配给所有船只
    bool berths_all_choosed = false;
    while (!berths_all_choosed){
        berths_all_choosed = true;
        for (int i = 0; i < berth_num; i++) {
            if (!berths_choosed[i]) {
                berths_all_choosed = false;
            }
        }
        int min_cycle_time = INT_MAX;
        int min_cycle_time_boatNumber = -1;
        for (int i = 0; i < boat_num; i++) {
            if (min_cycle_time > boat[i].cycle_total_time) {
                min_cycle_time_boatNumber = i;
                min_cycle_time = boat[i].cycle_total_time;
            }
        }
        for (int i = berth_num - 1; i >= 0; i--) {
            int choose_berth = IDX_test_score_berths[i];
            if (berths_choosed[choose_berth]) {
                continue;
            }
            boat[min_cycle_time_boatNumber].berths_to_go.push_front(choose_berth);
            boat[min_cycle_time_boatNumber].cycle_total_time = boat[min_cycle_time_boatNumber].cycle_total_time + boat[min_cycle_time_boatNumber].once_carried_cargo_number / berth[choose_berth].loading_speed + 5 + berth[choose_berth].transport_time;
            berths_choosed[choose_berth] = true;
            break;
        }
    }


    ////策略②  
    ////直接禁用其余泊位
    //for (int i = berth_num - 1; i >= 0; i--) {
    //    if (!berths_choosed[i]) {
    //        berth[i].closed = true;
    //    }
    //}



    for (int i = 0; i < boat_num; i++) {
        boat[i].once_carried_cargo_number = boat[i].capacity;
    }



    for (int i = 0; i < boat_num; i++) {
        // 计算最后一个循环的最晚开始时间
        boat[i].final_cycle_max_zhen = total_zhen - boat[i].cycle_total_time - 5;//跳帧预留5帧
        //计算每艘船最多进行循环的次数
        boat[i].total_cycle_number = boat[i].final_cycle_max_zhen / boat[i].cycle_total_time;
        boat[i].surplus_time = boat[i].final_cycle_max_zhen % boat[i].cycle_total_time;
    }

    char okk[100];
    scanf("%s", okk);
    fprintf(outputFile, "%s\n", okk);
    //cout << okk << endl;



    printf("OK\n");
    fflush(stdout);
}

int Input(list<Cargo>* Cargolist, int Node_movable_node[][4], int Node_is_berth[], int* item_id, bool* cargo_added, Robot robotsTest[])
{
    scanf("%d%d", &id, &money);
    fprintf(outputFile, "%d %d\n", id, money);
    int money1 = 0;
    for (int i = 0; i < berth_num; i++) {
        money1 = money1 + berth[i].money_total;
    }
    fprintf(money_statistic, "%d\n", money1);
    
    // 读入新增货物信息，将新增的cargo扩展到Cargolist中。
    int num;
    scanf("%d", &num);
    fprintf(outputFile, "%d\n", num);
    if (num >= 1) {
        *cargo_added = true;
    }
    for(int i = 1; i <= num; i ++)
    {
        int x, y, val;
        scanf("%d%d%d", &x, &y, &val);
        fprintf(outputFile, "%d %d %d\n", x, y,val);
        *item_id = *item_id + 1;
        Cargo cargo = Cargo(x, y, val, *item_id, cargo_remain_time, 0);
        int StartP = x * n + y;
        if (Node_movable_node[StartP][0] + Node_movable_node[StartP][1] + Node_movable_node[StartP][2] + Node_movable_node[StartP][3] != -4) {
            precalc_CargoToBerth(&cargo, berth, Node_movable_node, Node_is_berth, &cargo.CanBeGot, &cargo.nearestBerthNumber, &cargo.DistanceToBerth);
            if (cargo.CanBeGot) {
                Cargolist->push_back(cargo);
            }
        }
    }
    for(int i = 0; i < robot_num; i ++)
    {
        int sts;
        scanf("%d%d%d%d", &robot[i].goods, &robot[i].x, &robot[i].y, &sts);
        fprintf(outputFile, "%d %d %d %d\n", robot[i].goods, robot[i].x, robot[i].y,sts);
        robot[i].status = sts;
        if (id > 1) {
            //robot[i].x = robotsTest[i].x;
            //robot[i].y = robotsTest[i].y;
        }
        
    }
    for(int i = 0; i < 5; i ++){
        scanf("%d%d\n", &boat[i].status, &boat[i].pos);
        fprintf(outputFile, "%d %d\n", boat[i].status, boat[i].pos);
    }
    char okk[100];
    scanf("%s", okk);
    fprintf(outputFile, "%s\n", okk);
    return id;
}




int main()
{
    outputFile = fopen("input.txt", "w");
    money_statistic = fopen("MoneyInBerth.txt", "w");
    /////////////////////////////////////////////////运行初始化///////////////////////////////////////////////////////
    /*
    *  创建节点通径表，一个节点一行,一共40000行，从左上角开始从左往右顺序编号，换行继续编号。
    *  每一行为4个0-1变量，分别表示是否能够向右/左/上/下移动
    */
    int Node_movable_directions[n * n][4];
    int Node_movable_node[n * n][4];
    int Node_is_berth[n * n];
    for (int i = 0; i < n * n; i++ ) {
        Node_is_berth[i] = -1;
        for (int j = 0; j < 4; j++) {
            Node_movable_directions[i][j] = 0;
            Node_movable_node[i][j] = -1;
        }
    }

    /*
    *  创建泊位信息表，一个泊位一行,一共10行，按输入值编号。
    *  每一行为5个参数，[泊位编号, 泊位坐标x, y, 运输时间, 装载时间]
    */
    int Berth_information[berth_num][5];
    for (int i = 0; i < berth_num; i++) {
        for (int j = 0; j < 5; j++) {
            Berth_information[i][j] = 0;
        }
    }

    /*
    *  货船装载量
    */
    int Boat_capacity;

    /*
    * 创建一个初始的机器人群，以便在开始计算前先确定机器人是否可以运行。
    */
    Robot Robot_init[robot_num];

    /*
    * 初始化
    */
    Init( Node_movable_directions, Node_movable_node, Berth_information, Node_is_berth, &Boat_capacity, Robot_init);

    /*
    * 创建货物信息表
    */
    auto Cargolist = new list<Cargo>;
    int item_id = 0;

    /*
    * 算法初参数
    */
    int start_rearrange_robot_number = 0;
    int limit_time = 50; //剩余路径不足limit_time的机器人将规划未来路径
    int scan_range = 20; // 检查路径冲突的范围

    /////////////////////////////////////////////////开始逐帧计算///////////////////////////////////////////////////////
    for (int zhen = 1; zhen <= 15000; zhen++)
    {
        // 查错①
        if (zhen == 28) {
            int a = 1;
        }
        if (zhen == 135) {
            int a = 1;
        }
        if (zhen == 14500) {
            int a = 1;
        }
        if (!Cargolist->empty()){
            if (Cargolist->front().item_id == -1) {
                int a = 1;
            }
        }

        bool cargo_added = false;
        int id = Input(Cargolist, Node_movable_node, Node_is_berth, &item_id, &cargo_added,robotTest);  //每一帧的输入
        // 查错②
        if (zhen > 1) {
            for (int i = 0; i < robot_num; i++) {
                if (robot[i].x != robotTest[i].x || robot[i].y != robotTest[i].y) {
                    int a = 1;
                }
                if (!robot[i].path_total.empty()){
                    bool A = true;
                }
                if (!robot[i].path_getting.empty()) {
                    if (robot[i].path_total.front() != robot[i].path_getting.front()) {
                        int a = 1;
                    }
                }
                else if (!robot[i].path_pulling.empty()){
                    if (robot[i].path_total.front() != robot[i].path_pulling.front()) {
                        int a = 1;
                    }

                }
                if (!robot[i].path_total.empty()) {
                    if (robot[i].path_total.front() - (robot[i].x * 200 + robot[i].y) > 200 || robot[i].path_total.front() - (robot[i].x * 200 + robot[i].y) < -200) {
                        int a = 1;
                    }
                    if (robot[i].choose_item_id == -1 && robot[i].carried_money > 1) {
                        int a = 1;
                    }
                }
            }
            for (auto iter = Cargolist->begin(); iter != Cargolist->end();iter++) {
                if (iter->item_id == 3 && iter->status == 1) {
                    int a = 1;
                }
            }
        }
        /*
        * 开局，找到无效机器人并取消其路径规划
        */
        if (zhen == 1) {
            for (int i = 0; i < robot_num; i++) {
                for (int j = 0; j < robot_num; j++) {
                    if (Robot_init[i].x == robot[j].x && Robot_init[i].y == robot[j].y)
                    {
                        robot[j].active = Robot_init[i].active;
                        for (int k = 0; k < berth_num; k++) {
                            robot[j].berthsCanBeReached[k] = Robot_init[i].berthsCanBeReached[k];
                        }
                    }
                }
            }

            for (int i = 0; i < robot_num + 10; i++) {
                robotTest[i] = robot[i];
            }
        }


        /*
        * 更新货物表中货物的剩余时间(surplus_zheng -= 1)
        * 删除被搬运的货物(status = 2)以及到达最大滞留时间的货物(surplus_zheng = 0)
        */

        if (!Cargolist->empty())
        {
            for (auto i = Cargolist->begin(); i != Cargolist->end(); i++) {
                i->surplus_zheng -= 1;
            }
            auto ite = Cargolist->begin();
            while (ite != Cargolist->end()) {
                if (ite->surplus_zheng == 0) {
                    //查错③
                    if (ite->status == 1) {
                        int a = 1;
                    }
                    ite = Cargolist->erase(ite);
                }
                else if (ite->status == 2) {
                    ite = Cargolist->erase(ite);
                }
                else {
                    ite++;
                }
            }
        }

        /*
        * 根据当前状态，执行一次搬运任务的规划
        */
        bool cargo_unchoosen = false;
        bool task_arranged = false;
        if (!Cargolist->empty()) {
            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                if (iter->status == 0) {
                    cargo_unchoosen = true;
                }
            }
        }

        if (!cargo_added && cargo_unchoosen) {
            for (int i = 0; i < robot_num; i++) {
                if (robot[i].active && !robot[i].task_allocated) {
                    TaskArrange(&robot[i], Cargolist, Node_movable_node, &task_arranged);
                    break;
                }
            }
        }

        /*
        * 搬运任务的优化
        */
        //if (!task_arranged){
            if (start_rearrange_robot_number == robot_num) {
                start_rearrange_robot_number = 0;
            }
            else {
                start_rearrange_robot_number++;
            }
            if (!cargo_added && cargo_unchoosen) {
                optimize_task(robot, start_rearrange_robot_number, Cargolist, Node_movable_node, &task_arranged);
            }
       // }

        /*
        * 对路径即将走完的机器人预先规划后续路径
        */

        path_Preplan(robot, limit_time, Cargolist, Node_movable_node, &task_arranged);
        

        /*
        * 检查路径冲突
        */
 

        SolveDeadlock(robot, scan_range, Node_movable_node);

        /*
        * 对存储的路径处理格式并输出
        */

        /*
        * 规划轮船
        */
        BoatPlan(boat, berth, robot, zhen);
        for (int i = 0; i < n * n; i++) {
            if (Node_is_berth[i] != -1) {
                if (berth[Node_is_berth[i]].closed) {
                    Node_is_berth[i] = -1;
                }
            }
        }


        for (int i = 0; i < robot_num; i++) {
            if (robot[i].active) {
                if (!robot[i].path_getting_next.empty() && robot[i].path_getting.empty() && robot[i].path_pulling.empty()) {
                    robot[i].path_getting = robot[i].path_getting_next.front();
                    robot[i].path_getting_next.pop_front();
                    robot[i].path_pulling = robot[i].path_pulling_next.front();
                    robot[i].path_pulling_next.pop_front();
                    robot[i].choose_item_id = robot[i].choose_item_id_next.front();
                    robot[i].choose_item_id_next.pop_front();
                    robot[i].target_berth = robot[i].target_berth_next.front();
                    robot[i].target_berth_next.pop_front();
                    robot[i].carried_money = robot[i].carried_money_next.front();
                    robot[i].carried_money_next.pop_front();
                }
            }
        }


        bool one_moved[robot_num] = { false };
        bool all_moved = false;
        int try_times = 0;
        while (true) {
            try_times++;
            for (int i = 0; i < robot_num; i++) {
                if (robot[i].active && !robot[i].path_total.empty()) {
                    if (one_moved[i]) {
                        continue;
                    }
                    int direction = -1;
                    int target_point = -1;
                    bool can_move = true;
                    bool toGet = false;
                    bool toPull = false;
                    direction = robot[i].path_direction.front();
                    target_point = robot[i].path_total.front();
                    for (int j = 0; j < robot_num; j++) {
                        if (j != i && robotTest[j].x * n + robotTest[j].y == target_point) {
                            can_move = false;
                            break;
                        }
                    }
                    if (!can_move) {
                        continue;
                    }
                    one_moved[i] = true;
                    robot[i].path_direction.pop_front();
                    robot[i].path_total.pop_front();
                    if (!robot[i].carried && !robot[i].path_getting.empty()) {
                        robot[i].path_getting.pop_front();
                        if (robot[i].path_getting.empty()) {
                            toGet = true;
                            for (auto iter = Cargolist->begin(); iter != Cargolist->end();) {
                                if (iter->item_id == robot[i].choose_item_id) {
                                    iter = Cargolist->erase(iter);
                                    break;
                                }
                                iter++;
                            }
                        }
                    }
                    else if (robot[i].carried && !robot[i].path_pulling.empty()) {
                        robot[i].path_pulling.pop_front();
                        if (robot[i].path_pulling.empty()) {
                            toPull = true;
                        }
                    }
                    if (direction != -1) {
                        printf("move %d %d\n", i, direction);
                        if (direction == 0) {
                            robotTest[i].y++;
                        }
                        else if (direction == 1) {
                            robotTest[i].y--;
                        }
                        else if (direction == 2) {
                            robotTest[i].x--;
                        }
                        else if (direction == 3) {
                            robotTest[i].x++;
                        }

                    }
                    if (toGet) {
                        if (robot[i].target_berth != -1) {
                            printf("get %d\n", i);
                            robot[i].carried = true;
                        }
                        else {
                            robot[i].carried = true;
                        }
                    }
                    if (toPull) {
                        if (robot[i].target_berth != -1) {
                            printf("pull %d\n", i);
                            robot[i].carried = false;
                            berth[robot[i].target_berth].goods.push_back(robot[i].carried_money);
                            berth[robot[i].target_berth].money_total = berth[robot[i].target_berth].money_total + robot[i].carried_money;
                        }
                        else {
                            robot[i].carried = false;
                        }
                    }
                }
                else {
                    one_moved[i] = true;
                }

            }

            all_moved = true;
            for (int i = 0; i < robot_num; i++) {
                if (!one_moved[i]) {
                    all_moved = false;
                }
            }
            if (all_moved) {
                break;
            }
            if (try_times > 20) {
                break;
            }
        }
        for (int i = 0; i < robot_num; i++) {
            if (robot[i].active) {
                if (!robot[i].path_getting_next.empty() && robot[i].path_getting.empty() && robot[i].path_pulling.empty()) {
                    robot[i].path_getting = robot[i].path_getting_next.front();
                    robot[i].path_getting_next.pop_front();
                    robot[i].path_pulling = robot[i].path_pulling_next.front();
                    robot[i].path_pulling_next.pop_front();
                    robot[i].choose_item_id = robot[i].choose_item_id_next.front();
                    robot[i].choose_item_id_next.pop_front();
                    robot[i].target_berth = robot[i].target_berth_next.front();
                    robot[i].target_berth_next.pop_front();
                    robot[i].carried_money = robot[i].carried_money_next.front();
                    robot[i].carried_money_next.pop_front();
                }
            }
        }

        // 执行船的指令
        for (int num = 0; num < boat_num; num++) {
            if (boat[num].status == 1) {
               int a = 1;
            }

            if (boat[num].need_to_ship) {
                boat[num].need_to_ship = false;
                printf("ship %d %d\n", num, boat[num].shipping_target);
            }
            if (boat[num].need_to_go) {
                boat[num].need_to_go = false;
                printf("go %d\n", num);
            }
        }
        puts("OK");
        fflush(stdout);
    }
    //fclose(stdout);
    return 0;
}