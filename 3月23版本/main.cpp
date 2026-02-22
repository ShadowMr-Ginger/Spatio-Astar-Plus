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

struct Cargo
{
    int x;
    int y;
    int point_number;
    int money;
    int surplus_zheng;
    int item_id;
    int status;  // 状态：0闲置，1正在被一个机器人选定，2已经被拿取，3位于某个机器人的预备列表中
    bool CanBeGot = false;
    bool BerthsCanReach[berth_num] = { false };
    int BerthsReachedPointNumber[berth_num];
    int DistanceToBerth[berth_num];
    int nearestBerthNumber = -1;
    list<int> path_point[berth_num];
    list<int> path_direction[berth_num];
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

struct Robot
{
    int x, y, goods;
    int point_number;
    int status;
    int mbx, mby;
    Robot() {}
    Robot(int startX, int startY) {
        x = startX;
        y = startY;
    }
    bool berthsCanBeReached[berth_num] = { false };
    bool active = false;
    bool carried = false;
    bool task_allocated = false;
    //当前正在去取的货物的属性：
    int choose_item_id = -1; // 货物id
    int target_berth = -1; // 目标泊位
    int carried_money = 0; // 已在搬运中的货物价格
    int get_cargo_time = -1; // 搬起该货物还需要的时间，-1表示已经搬起。
    int pull_cargo_time = -1; // 放下当前货物还需要的时间
    list<int> path_getting; //  当前的取货路径
    list<int> path_getting_direction; //对应的方向顺序
    list<int> path_pulling; //  当前的送货路径
    list<int> path_pulling_direction; //对应的方向顺序
    Cargo now_cargo;
    //未来将要去取的货物的属性：
    list<int> choose_item_id_next;
    list<int> target_berth_next;
    list<int> carried_money_next;
    list<int> get_cargo_time_next;
    list<int> pull_cargo_time_next;
    list<list<int>> path_getting_next;
    list<list<int>> path_getting_direction_next;
    list<list<int>> path_pulling_next;
    list<list<int>> path_pulling_direction_next;
    list<Cargo> cargo_next;
    // 整体属性
    int task_total_money = 0; //所有搬运任务的货物金额之和
    list<int> path_total; //搬运所有货物时的总路径
    list<int> path_direction;  // 搬运所有货物时的总移动方向路径
}robot[robot_num + 10];

Robot robotTest[robot_num + 10];

struct Berth
{
    int x;
    int y;
    int transport_time;
    int round_trip_transport_time; // 货船在该泊位来回运输一船货物的耗时
    int loading_speed;
    int cargo_capacity = 0;//泊位还可运送的货物量
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
    int capacity;// 轮船的载货量

    list<int> berths_to_go;// 轮船的排班表
    int final_berth = -1;// 最终前往的泊位
    int final_trip_start_time = -1; // 最终前往该泊位的时间

    // 状态以及指令集
    int carried_money;
    bool need_to_ship = false;
    int shipping_target = -1;
    bool at_berth = false;
    bool is_final_cycle = false;
    int now_berth;
    int remaining_time = 0;
    bool need_to_go = false;
    int surplus_time = 0;
    int cargo_number = 0;
}boat[10];

int money, boat_capacity, id;
char ch[N][N];
int gds[N][N];





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

void Astar_pathCalc(int startP_number, int endP_number, int Node_movable_node[][4], int* distance, list<int> path, list<int>directions)
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
    list<openlist_unit> openlist;
    list<openlist_unit> openlist_new;
    int closelist[n * n][3] = { 0 };
    openlist_unit openP = openlist_unit(startP_number, -1, -1);
    openlist.push_back(openP);
    *distance = -1;

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
        }
        if (closelist[endP_number][1] == 1) {
            //回溯路径
            int last_Pnumber = endP_number;
            while (closelist[last_Pnumber][1] != -1) {
                path.push_front(last_Pnumber);
                directions.push_front(closelist[last_Pnumber][2]);
                last_Pnumber = closelist[last_Pnumber][1];
            }
            break;
        }
        openlist = openlist_new;
        openlist_new.clear();
    }
}

/*
* （Init中调用的函数）
* 检测每个机器人可以抵达的泊位
*/
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

/*
* Step 1 中调用的函数
* 计算货物到所有泊位的距离和路径
*/
void precalc_CargoToBerth(Cargo* cargo, Berth berths[], int Node_movable_node[][4], int Node_is_berth[])
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
    int closelist[n * n][3] = { 0 };
    openlist_unit openP = openlist_unit(startP_number, -1, -1);
    openlist.push_back(openP);
    int distance = -1;
    int final_endPnumber;
    //检查泊位是否都已经关闭，若是，则不进行规划
    bool all_closed = true;
    for (int i = 0; i < berth_num; i++) {
        if (!berth[i].closed) {
            all_closed = false;
            break;
        }
    }
    if (all_closed) {
        cargo->CanBeGot = false;
    }
    else {
        bool never_reached = true;
        while (true) {
            if (openlist.empty()) {
                break;
            }
            bool all_reached = true;
            for (int i = 0; i < berth_num; i++) {
                if (!cargo->BerthsCanReach[i]) {
                    all_reached = false;
                }
            }
            if (all_reached) {
                break;
            }
            distance = distance + 1;
            // 距离大于250的直接取消
            if (distance > 250)
            {
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
                if (Node_is_berth[iter->pointNumber] != -1) {
                    if (cargo->BerthsCanReach[Node_is_berth[iter->pointNumber]]) {
                        continue;
                    }
                    cargo->CanBeGot = true;
                    cargo->DistanceToBerth[Node_is_berth[iter->pointNumber]] = distance;
                    cargo->BerthsCanReach[Node_is_berth[iter->pointNumber]] = true;
                    cargo->BerthsReachedPointNumber[Node_is_berth[iter->pointNumber]] = iter->pointNumber;
                    if (never_reached) {
                        cargo->nearestBerthNumber = Node_is_berth[iter->pointNumber];
                    }
                    //回溯路径
                    final_endPnumber = iter->pointNumber;
                    int last_Pnumber = final_endPnumber;
                    while (closelist[last_Pnumber][1] != -1) {
                        cargo->path_point[Node_is_berth[iter->pointNumber]].push_front(last_Pnumber);
                        cargo->path_direction[Node_is_berth[iter->pointNumber]].push_front(closelist[last_Pnumber][2]);
                        last_Pnumber = closelist[last_Pnumber][1];
                    }
                }

            }
            openlist = openlist_new;
            openlist_new.clear();
        }
    }
}


/*
* Step 2 中调用的函数
* 状态更新函数
*/
void StatusRefresh() {
    for (int i = 0; i < robot_num; i++) {
        if (robot[i].active) {
            //更新机器人所在的点编号
            robot[i].point_number = robot[i].x * n + robot[i].y;
            //更新时间轴
            int last_cargo_finish_time = 0;
            if (!robot[i].path_getting.empty()) {
                last_cargo_finish_time = last_cargo_finish_time + robot[i].path_getting.size();
                robot[i].get_cargo_time = last_cargo_finish_time;
            }
            else {
                robot[i].get_cargo_time = -1;
            }
            if (!robot[i].path_pulling.empty()) {
                last_cargo_finish_time = last_cargo_finish_time + robot[i].path_pulling.size();
                robot[i].pull_cargo_time = last_cargo_finish_time;
            }
            robot[i].get_cargo_time = robot[i].path_getting.size();
            robot[i].get_cargo_time_next.clear();
            if (!robot[i].path_getting_next.empty()) {
                auto pulling_iter = robot[i].path_pulling_next.begin();
                for (auto getting_iter = robot[i].path_getting_next.begin(); getting_iter != robot[i].path_getting_next.end();) {
                    last_cargo_finish_time = last_cargo_finish_time+ getting_iter->size();
                    robot[i].get_cargo_time_next.push_back(last_cargo_finish_time);
                    last_cargo_finish_time = last_cargo_finish_time + pulling_iter->size();
                    robot[i].pull_cargo_time_next.push_back(last_cargo_finish_time);
                    getting_iter++;
                    pulling_iter++;
                }
            }

        }
    }
}


/*
* Step 3 中调用的函数
********* 核心函数 ***********
* 机器人调度函数
* 选择最优价距比的货物并路径规划
*/

void TaskArrange(list<Cargo> Cargolist, int Node_movable_node[][4]) {
    
    //为仍无路径的小车优先规划路径
    //为了防止跳帧，如果在这个函数中执行过1次路径规划，不再执行第二次的路径规划。
    bool pathPlanned = false;
    for (int num = 0; num < robot_num; num++) {
        if (Cargolist.empty()) {
            break;
        }
        if (pathPlanned) {
            break;
        }
        list<Cargo> Cargolist_new;
        for (auto iter = Cargolist.begin(); iter != Cargolist.end(); iter++) {
            if (iter->status == 0) {
                Cargolist_new.push_back(*iter);
            }
        }
        if (robot[num].path_total.empty()) {
            for (auto cargo = Cargolist_new.begin(); cargo != Cargolist_new.end(); cargo++) {
                int choose_berth = cargo->nearestBerthNumber;
                if (robot[num].berthsCanBeReached[choose_berth]) {
                    int startP_number = robot[num].point_number;
                    int endP_number = cargo->point_number;
                    int distance;
                    list<int> path;
                    list<int> directions;
                    Astar_pathCalc(startP_number, endP_number, Node_movable_node, &distance, path, directions);
                    pathPlanned = true;
                    if (distance + 5 > cargo->surplus_zheng) { // 预留5帧容错
                        continue;
                    }
                    else {
                        robot[num].path_getting = path; //取货路径
                        robot[num].path_pulling = cargo->path_point[cargo->nearestBerthNumber];//送货路径
                        robot[num].path_getting_direction = directions;//取货方向
                        robot[num].path_pulling_direction = cargo->path_direction[cargo->nearestBerthNumber];//放货方向
                        robot[num].choose_item_id = cargo->item_id; // 货物id
                        robot[num].target_berth = cargo->nearestBerthNumber; // 目标泊位
                        robot[num].carried_money = cargo->money; // 已在搬运中的货物价格
                        cargo->status = 1;
                        break;
                    }
                }
                else {
                    continue;
                }
            }
        }
    }

    // 生成一个序列，对任何已有路径的小车，规划其针对下一个和下下个运输任务的路径
    if (!pathPlanned && !Cargolist.empty()) {
        // 首先计算重新规划前的方案的目标函数
        double maxF = 0;
        // 扣除惩罚项
        for (int i = 0; i < robot_num; i++) {
            maxF = maxF - (2 - robot[i].path_getting_next.size()) * 10000;
        }
        int total_money = 0;
        double distance_value_ratio_perRobot[robot_num];
        double distance_value_ratio_total = 0;
        int max_time = 0;
        for (int num = 0; num < robot_num; num++) {
            if (robot[num].path_total.size() > max_time) {
                max_time = robot[num].path_total.size();
            }
        }
        for (int num = 0; num < robot_num; num++) {
            if (robot[num].path_total.empty()) {
                distance_value_ratio_perRobot[num] = 0;
            }
            else {
                distance_value_ratio_perRobot[num] = (double)robot[num].task_total_money / robot[num].pull_cargo_time_next.back();
            }
            distance_value_ratio_total = distance_value_ratio_total + distance_value_ratio_perRobot[num];
        }
        maxF = maxF + distance_value_ratio_total;

        //srand(unsigned(time(0)));
        // 重复随机规划10次
        for (int times = 0; times < 10; times++) {
            //随机优先规划的顺序
            int IDX[robot_num];
            for (int i = 0; i < robot_num; i++) {
                IDX[i] = i;
            }
            random_shuffle(IDX, IDX + 10);
            //复制一个可被用于规划的货物列表
            list<Cargo> Cargolist_new;
            for (auto iter = Cargolist.begin(); iter != Cargolist.end(); iter++) {
                if (iter->status == 0 || iter->status == 3) {
                    Cargolist_new.push_back(*iter);
                }
            }
            //复制一份新的机器人表
            Robot robot_new[robot_num];
            for (int i = 0; i < robot_num; i++) {
                robot_new[i] = robot[i];
                //去除它未来被规划的运输方案
                robot_new[i].carried_money_next.clear();
                robot_new[i].choose_item_id_next.clear();
                robot_new[i].get_cargo_time_next.clear();
                robot_new[i].path_getting_direction_next.clear();
                robot_new[i].path_getting_next.clear();
                robot_new[i].path_pulling_next.clear();
                robot_new[i].path_pulling_direction_next.clear();
                robot_new[i].pull_cargo_time_next.clear();
                robot_new[i].target_berth_next.clear();
                robot_new[i].task_total_money = robot_new[i].carried_money;
            }
            for (int num = 0; num < robot_num; num++) {
                int i = IDX[num];
                double now_value_distance_ratio = 0;
                if (Cargolist_new.size() == 0) {
                    break;
                }
                //货物表只有一个货物的情况
                else if (Cargolist_new.size() == 1) {
                    auto cargo = Cargolist_new.begin();
                    int choose_berth = -1;
                    // 若没变则不作更改
                    if (robot[i].cargo_next.size() == 1 && robot[i].cargo_next.front().item_id == cargo->item_id) {
                        continue;
                    }
                    if (robot_new[i].carried) {
                        choose_berth = robot_new[i].target_berth;
                        if (!cargo->BerthsCanReach[choose_berth]) {
                            continue;
                        }
                        robot_new[i].cargo_next.push_back(*cargo);
                        robot_new[i].carried_money_next.push_back(cargo->money);
                        robot_new[i].task_total_money = robot_new[i].task_total_money + cargo->money;
                        robot_new[i].target_berth_next.push_back(cargo->nearestBerthNumber);
                        robot_new[i].pull_cargo_time_next.push_back(robot_new[i].pull_cargo_time + cargo->DistanceToBerth[choose_berth] + cargo->DistanceToBerth[cargo->nearestBerthNumber]);
                        Cargolist_new.erase(cargo);
                    }
                    else {
                        int best_final_time = INT_MAX;
                        for (int j = 0; j < berth_num; j++) {
                            if (!robot_new[i].now_cargo.BerthsCanReach[j] || !cargo->BerthsCanReach[j]) {
                                continue;
                            }
                            int distance_now = cargo->DistanceToBerth[cargo->nearestBerthNumber];
                            distance_now = distance_now + robot_new[i].now_cargo.DistanceToBerth[j] + cargo->DistanceToBerth[j];
                            if (distance_now < best_final_time){
                                best_final_time = distance_now;
                                choose_berth = j;
                            }
                        }
                        robot_new[i].target_berth = choose_berth;
                        robot_new[i].path_pulling = robot_new[i].now_cargo.path_point[choose_berth];
                        robot_new[i].path_pulling_direction = robot_new[i].now_cargo.path_direction[choose_berth];
                        robot_new[i].cargo_next.push_back(*cargo);
                        robot_new[i].carried_money_next.push_back(cargo->money);
                        robot_new[i].task_total_money = robot_new[i].task_total_money + cargo->money;
                        robot_new[i].target_berth_next.push_back(cargo->nearestBerthNumber);
                        robot_new[i].pull_cargo_time = robot_new[i].get_cargo_time + robot_new[i].path_pulling.size();
                        robot_new[i].pull_cargo_time_next.push_back(robot_new[i].pull_cargo_time + cargo->DistanceToBerth[choose_berth] + cargo->DistanceToBerth[cargo->nearestBerthNumber]);

                    }
                }
                //  货物表有两个以上货物的情况
                else if (Cargolist_new.size()>=2) {
                    double max_VD_ratio = 0;
                    Cargo choose_cargo1;
                    Cargo choose_cargo2;
                    int choose_berth1 = -1;
                    int choose_berth2 = -1;
                    for (auto cargo1 = Cargolist.begin(); cargo1 != Cargolist.end(); cargo1++) {
                        for (auto cargo2 = Cargolist.begin(); cargo2 != Cargolist.end(); cargo2++) {
                            int t_choose_berth1= -1;
                            int t_choose_berth2= -1;
                            if (cargo1->item_id == cargo2->item_id) {
                                continue;
                            }
                            if (robot[i].cargo_next.size() == 2 && robot[i].cargo_next.front().item_id == cargo1->item_id && robot[i].cargo_next.back().item_id == cargo2->item_id) {
                                continue;
                            }
                            int best_final_time = INT_MAX;
                            for (int berth2 = 0; berth2 < berth_num; berth2++) {
                                if (!cargo1->BerthsCanReach[berth2] || !cargo2->BerthsCanReach[berth2]) {
                                    continue;
                                }
                                if (robot_new[i].carried) {
                                    int berth1 = robot_new[i].target_berth;
                                    if (!cargo1->BerthsCanReach[berth1]) {
                                        continue;
                                    }
                                    int distance_now = robot_new[i].pull_cargo_time + cargo2->DistanceToBerth[cargo2->nearestBerthNumber];
                                    distance_now = distance_now + cargo1->DistanceToBerth[berth1] + cargo2->DistanceToBerth[berth1];
                                    if (distance_now < best_final_time) {
                                        best_final_time = distance_now;
                                        t_choose_berth1 = berth1;
                                        t_choose_berth2 = berth2;
                                    }
                                    
                                }
                                else {
                                    for (int berth1 = 0; berth1 < berth_num; berth1++) {
                                        if (!robot_new[i].now_cargo.BerthsCanReach[berth1] || !cargo1->BerthsCanReach[berth1]) {
                                            continue;
                                        }
                                        int distance_now= cargo2->DistanceToBerth[cargo2->nearestBerthNumber];
                                        distance_now = distance_now + robot[i].now_cargo.DistanceToBerth[berth1] + cargo1->DistanceToBerth[berth1];
                                        distance_now = distance_now + cargo1->DistanceToBerth[berth2] + cargo2->DistanceToBerth[berth2];
                                        if (distance_now < best_final_time) {
                                            best_final_time = distance_now;
                                            t_choose_berth1 = berth1;
                                            t_choose_berth2 = berth2;
                                        }
                                    }
                                }
                            }
                            // 迭代最优值
                            double VD_ratio = robot[i].carried_money + cargo1->money + cargo2->money;
                            if (robot[i].carried)
                            {
                                VD_ratio = VD_ratio / (robot[i].pull_cargo_time + cargo1->DistanceToBerth[t_choose_berth1] + cargo1->DistanceToBerth[t_choose_berth2] + cargo2->DistanceToBerth[t_choose_berth2] + cargo2->DistanceToBerth[cargo2->nearestBerthNumber]);
                            }
                            else {
                                VD_ratio = VD_ratio / (robot[i].get_cargo_time + robot[i].now_cargo.DistanceToBerth[t_choose_berth1] + cargo1->DistanceToBerth[t_choose_berth1] + cargo1->DistanceToBerth[t_choose_berth2] + cargo2->DistanceToBerth[t_choose_berth2] + cargo2->DistanceToBerth[cargo2->nearestBerthNumber]);
                            }
                            if (VD_ratio > max_VD_ratio) {
                                max_VD_ratio = VD_ratio;
                                choose_cargo1 = *cargo1;
                                choose_cargo2 = *cargo2;
                                choose_berth1 = t_choose_berth1;
                                choose_berth2 = t_choose_berth2;
                            }
                        }
                    }
                    robot_new[i].cargo_next.push_back(choose_cargo1);
                    robot_new[i].cargo_next.push_back(choose_cargo2);
                    robot_new[i].carried_money_next.push_back(choose_cargo1.money);
                    robot_new[i].carried_money_next.push_back(choose_cargo2.money);
                    robot_new[i].task_total_money = robot_new[i].task_total_money + choose_cargo1.money + choose_cargo2.money;
                    robot_new[i].target_berth = choose_berth1;
                    robot_new[i].target_berth_next.push_back(choose_berth2);
                    robot_new[i].target_berth_next.push_back(choose_cargo2.nearestBerthNumber);
                    if (robot_new[i].carried) {
                        robot_new[i].pull_cargo_time_next.push_back(robot_new[i].pull_cargo_time + choose_cargo1.DistanceToBerth[choose_berth1] + choose_cargo1.DistanceToBerth[choose_berth2]);
                        robot_new[i].pull_cargo_time_next.push_back(robot_new[i].pull_cargo_time_next.back() + choose_cargo2.DistanceToBerth[choose_berth2] + choose_cargo2.DistanceToBerth[choose_cargo2.nearestBerthNumber]);
                    }
                    else {
                        robot_new[i].path_pulling = robot_new[i].now_cargo.path_point[choose_berth1];
                        robot_new[i].path_pulling_direction = robot_new[i].now_cargo.path_direction[choose_berth1];
                        robot_new[i].pull_cargo_time = robot_new[i].get_cargo_time + robot_new[i].path_pulling.size();
                        robot_new[i].pull_cargo_time_next.push_back(robot_new[i].pull_cargo_time + choose_cargo1.DistanceToBerth[choose_berth1] + choose_cargo1.DistanceToBerth[choose_berth2]);
                        robot_new[i].pull_cargo_time_next.push_back(robot_new[i].pull_cargo_time_next.back() + choose_cargo2.DistanceToBerth[choose_berth2] + choose_cargo2.DistanceToBerth[choose_cargo2.nearestBerthNumber]);
                    }
                }
            }
            double maxF_now = 0;
            double distance_value_ratio_perRobot_now[robot_num] = { 0 };
            for (int i = 0; i < robot_num; i++) {
                maxF_now = maxF_now - (2 - robot_new[i].path_getting_next.size()) * 10000;
            }
            for (int num = 0; num < robot_num; num++) {
                distance_value_ratio_perRobot_now[num] = (double)robot_new[num].task_total_money / robot_new[num].pull_cargo_time_next.back();
                maxF_now = maxF_now + distance_value_ratio_perRobot_now[num];
            }
            if (maxF_now > maxF) {
                maxF = maxF_now;
                for (int i = 0; i < robot_num; i++) {
                    robot[i] = robot_new[i];
                }
            }
        }
    }

}
    

/*
* Step 4 中调用的函数
* 机器人方案处理函数
*/
void PlanProcess() {
    for (int i = 0; i < robot_num;i++) {
        if (!robot[i].active) {
            continue;
        }
        if (robot[i].path_pulling.empty()){
            continue;
        }

        if (robot[i].cargo_next.empty()) {
            continue;
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
    list<int>* rearrange_path_points,
    list<int>* rearrange_path_directions,
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
                            if (can_new_deadlock) {
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
                            int* P = new int[5];
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
        need_to_solve_pair(int Robot1, int Robot2, int Robot1_target, int Robot2_target) {
            this->robot1 = Robot1;
            this->robot2 = Robot2;
            this->robot1_target = Robot1_target;
            this->robot2_target = Robot2_target;
        }
    };
    list<need_to_solve_pair> DeadlockList;
    bool robot_path_lower[robot_num] = { false };
    int robots_paths[robot_num][500];
    for (int j = 0; j < robot_num; j++) {
        if (!robots[j].path_total.empty()) {
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
        for (int k = j + 1; k < robot_num; k++) {
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
                    else {
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
            for (int i = 0; i < scan_range - 1; i++) {
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
            if (!no_solution) {
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
//void path_Preplan(Robot robots[], int limit_time, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged)
//{
//    struct openlist_unit {
//        int pointNumber;
//        int lastPoint;
//        int diRection;
//        openlist_unit() {}
//        openlist_unit(int pointnumber, int lastpoint, int direction) {
//            this->lastPoint = lastpoint;
//            this->pointNumber = pointnumber;
//            this->diRection = direction;
//        }
//    };
//    bool rearranged = false;
//    for (int i = 0; i < robot_num; i++) {
//        if (robots[i].active) {
//            while (robots[i].path_total.size() < limit_time) {
//                bool successful_plan = false;
//                int startP_number;
//                if (!robots[i].path_total.empty()) {
//                    startP_number = robots[i].path_total.back();
//                }
//                else {
//                    startP_number = robots[i].x * n + robots[i].y;
//                }
//                list<Cargo> Cargolist_new;
//                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                    if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
//                    {
//                        Cargolist_new.push_back(*iter);
//                    }
//                }
//                if (Cargolist_new.empty()) {
//                    bool successful_plan = false;
//                }
//                rearranged = true;
//                //Astar部分
//                list<openlist_unit> openlist;
//                list<openlist_unit> openlist_new;
//                int closelist[n * n][3] = { 0 };
//                openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                openlist.push_back(openP);
//                int distance = -1;
//                double max_value_distance_ratio = 0;
//                Cargo* chooseCargo = new Cargo();
//                int max_scan_distance = 999;
//                if (!Cargolist_new.empty()) {
//                    while (true) {
//                        if (openlist.empty()) {
//                            break;
//                        }
//                        distance++;
//                        if (distance > max_scan_distance) {
//                            break;
//                        }
//                        for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                            if (closelist[iter->pointNumber][0] == 0) {
//                                closelist[iter->pointNumber][0] = 1;
//                                closelist[iter->pointNumber][1] = iter->lastPoint;
//                                closelist[iter->pointNumber][2] = iter->diRection;
//                                for (int j = 0; j < 4; j++) {
//                                    if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                        openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                        openP.lastPoint = iter->pointNumber;
//                                        openP.diRection = j;
//                                        openlist_new.push_back(openP);
//                                    }
//                                }
//                            }
//                            bool scanRatio = false;
//                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
//                                // 剔除无法获取的货物
//                                if (iter2->surplus_zheng - distance < 20) {
//                                    iter2 = Cargolist_new.erase(iter2);
//                                }
//                                // 检测是否可以取到货物
//                                else if (iter->pointNumber == iter2->point_number) {
//                                    successful_plan = true;
//                                    //计算价距比
//                                    iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
//                                    if (iter2->value_distance_ratio > max_value_distance_ratio) {
//                                        chooseCargo->point_number = iter2->point_number;
//                                        chooseCargo->item_id = iter2->item_id;
//                                        chooseCargo->path_point = iter2->path_point;
//                                        chooseCargo->path_direction = iter2->path_direction;
//                                        chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
//                                        chooseCargo->money = iter2->money;
//                                        max_value_distance_ratio = iter2->value_distance_ratio;
//                                        scanRatio = true;
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                    else {
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                }
//                                else {
//                                    iter2++;
//                                }
//                            }
//                            if (scanRatio) {
//                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
//                                    if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                    else {
//                                        int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
//                                        if (over_distance < max_scan_distance) {
//                                            max_scan_distance = over_distance;
//                                        }
//                                        iter2++;
//                                    }
//                                }
//                            }
//                        }
//                        openlist = openlist_new;
//                        openlist_new.clear();
//                    }
//                }
//                //回溯路径
//                if (successful_plan) {
//                    *task_arranged = true;
//                    robots[i].choose_item_id_next.push_back(chooseCargo->item_id);
//                    list<int> path_getting_next_plus;
//                    list<int> path_pulling_next_plus;
//                    list<int> path_total_plus;
//                    list<int> direction_total_plus;
//                    int last_Pnumber = chooseCargo->point_number;
//                    while (closelist[last_Pnumber][1] != -1) {
//                        path_getting_next_plus.push_front(last_Pnumber);
//                        path_total_plus.push_front(last_Pnumber);
//                        direction_total_plus.push_front(closelist[last_Pnumber][2]);
//                        last_Pnumber = closelist[last_Pnumber][1];
//                    }
//                    path_pulling_next_plus = chooseCargo->path_point;
//                    robots[i].path_getting_next.push_back(path_getting_next_plus);
//                    robots[i].path_pulling_next.push_back(path_pulling_next_plus);
//                    robot[i].path_total = merge_in_order(robot[i].path_total, path_total_plus);
//                    robot[i].path_total = merge_in_order(robot[i].path_total, chooseCargo->path_point);
//                    robot[i].path_direction = merge_in_order(robot[i].path_direction, direction_total_plus);
//                    robot[i].path_direction = merge_in_order(robot[i].path_direction, chooseCargo->path_direction);
//                    robot[i].target_berth_next.push_back(chooseCargo->nearestBerthNumber);
//                    robot[i].carried_money_next.push_back(chooseCargo->money);
//                    //修改货物状态
//                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                        if (iter->item_id == chooseCargo->item_id) {
//                            iter->status = 1;
//                        }
//                    }
//                    robot[i].task_allocated = true;
//                    if (robot[i].path_total.size() > limit_time) {
//                        break;
//                    }
//                }
//                else {
//                    // 没有可行的规划路径  建立一个虚拟取货路径
//
//                    rearranged = true;
//                    //Astar部分
//                    list<openlist_unit> openlist;
//                    list<openlist_unit> openlist_new;
//                    int closelist[n * n][3] = { 0 };
//                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                    openlist.push_back(openP);
//                    int distance = -1;
//                    Cargo* chooseCargo = new Cargo();
//                    chooseCargo->money = 0;
//                    int max_scan_distance = 999;
//                    bool successful_plan = false;
//                    while (true) {
//                        if (openlist.empty()) {
//                            break;
//                        }
//                        distance++;
//                        if (distance == limit_time) {
//                            for (auto iter = openlist.begin(); iter != openlist.end(); iter++) {
//                                if (closelist[iter->pointNumber][0] == 0) {
//                                    closelist[iter->pointNumber][0] = 1;
//                                    closelist[iter->pointNumber][1] = iter->lastPoint;
//                                    closelist[iter->pointNumber][2] = iter->diRection;
//                                    chooseCargo->point_number = iter->pointNumber;
//                                    chooseCargo->item_id = -1;
//                                    chooseCargo->nearestBerthNumber = -1;
//                                    successful_plan = true;
//                                    break;
//                                }
//                            }
//                            break;
//                        }
//                        for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                            if (closelist[iter->pointNumber][0] == 0) {
//                                closelist[iter->pointNumber][0] = 1;
//                                closelist[iter->pointNumber][1] = iter->lastPoint;
//                                closelist[iter->pointNumber][2] = iter->diRection;
//                                for (int j = 0; j < 4; j++) {
//                                    if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                        openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                        openP.lastPoint = iter->pointNumber;
//                                        openP.diRection = j;
//                                        openlist_new.push_back(openP);
//                                    }
//                                }
//                            }
//                        }
//                        openlist = openlist_new;
//                        openlist_new.clear();
//                    }
//                    //回溯路径
//                    if (successful_plan) {
//                        *task_arranged = true;
//                        robots[i].choose_item_id_next.push_back(chooseCargo->item_id);
//                        list<int> path_getting_next_plus;
//                        list<int> path_pulling_next_plus;
//                        list<int> path_total_plus1;
//                        list<int> path_total_plus2;
//                        list<int> direction_total_plus1;
//                        list<int> direction_total_plus2;
//                        int last_Pnumber = chooseCargo->point_number;
//                        while (closelist[last_Pnumber][1] != -1) {
//                            path_getting_next_plus.push_front(last_Pnumber);
//                            path_pulling_next_plus.push_back(last_Pnumber);
//                            path_total_plus1.push_front(last_Pnumber);
//                            path_total_plus2.push_back(last_Pnumber);
//                            direction_total_plus1.push_front(closelist[last_Pnumber][2]);
//                            if (closelist[last_Pnumber][2] == 0) {
//                                direction_total_plus2.push_back(1);
//                            }
//                            else if (closelist[last_Pnumber][2] == 1) {
//                                direction_total_plus2.push_back(0);
//                            }
//                            else if (closelist[last_Pnumber][2] == 2) {
//                                direction_total_plus2.push_back(3);
//                            }
//                            else {
//                                direction_total_plus2.push_back(2);
//                            }
//                            last_Pnumber = closelist[last_Pnumber][1];
//                        }
//                        path_pulling_next_plus.pop_front();
//                        path_total_plus2.pop_front();
//                        direction_total_plus2.pop_back();
//                        robots[i].path_getting_next.push_back(path_getting_next_plus);
//                        robots[i].path_pulling_next.push_back(path_pulling_next_plus);
//                        robot[i].path_total = merge_in_order(robot[i].path_total, path_total_plus1);
//                        robot[i].path_total = merge_in_order(robot[i].path_total, path_total_plus2);
//                        robot[i].path_direction = merge_in_order(robot[i].path_direction, direction_total_plus1);
//                        robot[i].path_direction = merge_in_order(robot[i].path_direction, direction_total_plus2);
//                        robot[i].target_berth_next.push_back(chooseCargo->nearestBerthNumber);
//                        robot[i].carried_money_next.push_back(chooseCargo->money);
//                        //修改货物状态
//                        robot[i].task_allocated = true;
//                        if (robot[i].path_total.size() > limit_time) {
//                            break;
//                        }
//                    }
//
//                    break;
//                }
//            }
//            //if (rearranged) {
//            //    break;
//            //}
//        }
//    }
//}

/*
********* 核心函数 ***********
* 优化搬运目标函数
* 针对已有搬运任务的机器人，若其未携带货物，扫描其他货物并考虑更换更优的搬运目标
*/
//void optimize_task(Robot robots[], int start_rearrange_robot_number, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged, int node_is_berth[])
//{
//    struct openlist_unit {
//        int pointNumber;
//        int lastPoint;
//        int diRection;
//        openlist_unit() {}
//        openlist_unit(int pointnumber, int lastpoint, int direction) {
//            this->lastPoint = lastpoint;
//            this->pointNumber = pointnumber;
//            this->diRection = direction;
//        }
//    };
//    int K = start_rearrange_robot_number;
//    int rearrange_robot_list[robot_num];
//    for (int i = 0; i < robot_num; i++) {
//        rearrange_robot_list[i] = K;
//        if (K == robot_num - 1) {
//            K = 0;
//        }
//        else {
//            K++;
//        }
//    }
//    // Step1 : 优先重规划虚拟搬运路径的机器人
//    bool rearranged = false;
//    int rearranged_robot_number = -1;
//    for (int m = 0; m < robot_num; m++) {
//        int i = rearrange_robot_list[m];
//        if (robot[i].active){
//            if (robot[i].choose_item_id == -1) {
//                int startP_number;
//                startP_number = robots[i].x * n + robots[i].y;
//                list<Cargo> Cargolist_new;
//                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                    if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
//                    {
//                        Cargolist_new.push_back(*iter);
//                    }
//                }
//                if (Cargolist_new.empty()) {
//                    break;
//                }
//                rearranged = true;
//                //Astar部分
//                list<openlist_unit> openlist;
//                list<openlist_unit> openlist_new;
//                int closelist[n * n][3] = { 0 };
//                openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                openlist.push_back(openP);
//                int distance = -1;
//                double max_value_distance_ratio = 0;
//                Cargo* chooseCargo = new Cargo();
//                int max_scan_distance = 999;
//                bool successful_plan = false;
//                while (true) {
//                    if (openlist.empty()) {
//                        break;
//                    }
//                    distance++;
//                    if (distance > max_scan_distance) {
//                        break;
//                    }
//                    for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                        if (closelist[iter->pointNumber][0] == 0) {
//                            closelist[iter->pointNumber][0] = 1;
//                            closelist[iter->pointNumber][1] = iter->lastPoint;
//                            closelist[iter->pointNumber][2] = iter->diRection;
//                            for (int j = 0; j < 4; j++) {
//                                if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                    openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                    openP.lastPoint = iter->pointNumber;
//                                    openP.diRection = j;
//                                    openlist_new.push_back(openP);
//                                }
//                            }
//                        }
//                        bool scanRatio = false;
//                        for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
//                            // 剔除无法获取的货物
//                            if (iter2->surplus_zheng - distance < 10) {
//                                iter2 = Cargolist_new.erase(iter2);
//                            }
//                            // 检测是否可以取到货物
//                            else if (iter->pointNumber == iter2->point_number) {
//                                successful_plan = true;
//                                //计算价距比
//                                iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
//                                if (iter2->value_distance_ratio > max_value_distance_ratio) {
//                                    chooseCargo->point_number = iter2->point_number;
//                                    chooseCargo->item_id = iter2->item_id;
//                                    chooseCargo->path_point = iter2->path_point;
//                                    chooseCargo->path_direction = iter2->path_direction;
//                                    chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
//                                    chooseCargo->money = iter2->money;
//                                    max_value_distance_ratio = iter2->value_distance_ratio;
//                                    scanRatio = true;
//                                    iter2 = Cargolist_new.erase(iter2);
//                                }
//                                else {
//                                    iter2 = Cargolist_new.erase(iter2);
//                                }
//                            }
//                            else {
//                                iter2++;
//                            }
//                        }
//                        if (scanRatio) {
//                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
//                                if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
//                                    iter2 = Cargolist_new.erase(iter2);
//                                }
//                                else {
//                                    int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
//                                    if (over_distance < max_scan_distance) {
//                                        max_scan_distance = over_distance;
//                                    }
//                                    iter2++;
//                                }
//                            }
//                        }
//                    }
//                    openlist = openlist_new;
//                    openlist_new.clear();
//                }
//                //回溯路径
//                if (successful_plan) {
//                    *task_arranged = true;
//                    list<int> path_getting_alternative;
//                    list<int> path_pulling_alternative;
//                    list<int> path_total_alternative;
//                    list<int> direction_total_alternative;
//                    int last_Pnumber = chooseCargo->point_number;
//                    while (closelist[last_Pnumber][1] != -1) {
//                        path_getting_alternative.push_front(last_Pnumber);
//                        path_total_alternative.push_front(last_Pnumber);
//                        direction_total_alternative.push_front(closelist[last_Pnumber][2]);
//                        last_Pnumber = closelist[last_Pnumber][1];
//                    }
//                    path_pulling_alternative = chooseCargo->path_point;
//                    path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
//                    direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
//                    if (!robot[i].path_getting.empty()) {
//                        auto iter2 = robot[i].path_total.begin();
//                        auto iter3 = robot[i].path_direction.begin();
//                        for (auto iter = robot[i].path_getting.begin(); iter != robot[i].path_getting.end();) {
//                            iter = robot[i].path_getting.erase(iter);
//                            iter2 = robot[i].path_total.erase(iter2);
//                            iter3 = robot[i].path_direction.erase(iter3);
//                        }
//                    }
//                    if (!robot[i].path_pulling.empty()) {
//                        auto iter2 = robot[i].path_total.begin();
//                        auto iter3 = robot[i].path_direction.begin();
//                        for (auto iter = robot[i].path_pulling.begin(); iter != robot[i].path_pulling.end();) {
//                            iter = robot[i].path_pulling.erase(iter);
//                            iter2 = robot[i].path_total.erase(iter2);
//                            iter3 = robot[i].path_direction.erase(iter3);
//                        }
//                    }
//
//                    robots[i].path_getting = path_getting_alternative;
//                    robots[i].path_pulling = path_pulling_alternative;
//                    robot[i].path_total = path_total_alternative;
//                    robot[i].path_direction = direction_total_alternative;
//                    robot[i].path_getting_next.clear();
//                    robot[i].path_pulling_next.clear();
//                    for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
//                        if (robot[i].choose_item_id == iter2->item_id) {
//                            iter2->status = 0;
//                        }
//                        for (auto iter = robot[i].choose_item_id_next.begin(); iter != robot[i].choose_item_id_next.end(); iter++) {
//
//                            if (*iter == iter2->item_id) {
//                                iter2->status = 0;
//                            }
//                        }
//                    }
//                    robot[i].target_berth = chooseCargo->nearestBerthNumber;
//                    robot[i].carried_money = chooseCargo->money;
//                    robot[i].carried = false;
//                    //修改货物状态
//                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                        if (iter->item_id == chooseCargo->item_id) {
//                            iter->status = 1;
//                        }
//                    }
//                    robot[i].choose_item_id = chooseCargo->item_id;
//                    robot[i].task_allocated = true;
//                }
//            }
//        }
//    }
//
//    // Step2: 检查是否存在机器人无法按时取货，若有，重新规划取货路径
//    for (int m = 0; m < robot_num; m++){
//        int i = rearrange_robot_list[m];
//        if (robot[i].active){
//            if (!robot[i].carried) {
//                int now_cargo_surplus_zheng = 0;
//                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                    if (iter->item_id == robots[i].choose_item_id) {
//                        now_cargo_surplus_zheng = iter->surplus_zheng;
//                    }
//                }
//                if (!robot[i].carried && robot[i].path_getting.size() > now_cargo_surplus_zheng) {
//                    int startP_number;
//                    startP_number = robots[i].x * n + robots[i].y;
//                    list<Cargo> Cargolist_new;
//                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                        if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
//                        {
//                            Cargolist_new.push_back(*iter);
//                        }
//                    }
//                    if (Cargolist_new.empty()) {
//                        break;
//                    }
//                    rearranged = true;
//                    //Astar部分
//                    list<openlist_unit> openlist;
//                    list<openlist_unit> openlist_new;
//                    int closelist[n * n][3] = { 0 };
//                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                    openlist.push_back(openP);
//                    int distance = -1;
//                    double max_value_distance_ratio = 0;
//                    Cargo* chooseCargo = new Cargo();
//                    int max_scan_distance = 999;
//                    bool successful_plan = false;
//                    while (true) {
//                        if (openlist.empty()) {
//                            break;
//                        }
//                        distance++;
//                        if (distance > max_scan_distance) {
//                            break;
//                        }
//                        for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                            if (closelist[iter->pointNumber][0] == 0) {
//                                closelist[iter->pointNumber][0] = 1;
//                                closelist[iter->pointNumber][1] = iter->lastPoint;
//                                closelist[iter->pointNumber][2] = iter->diRection;
//                                for (int j = 0; j < 4; j++) {
//                                    if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                        openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                        openP.lastPoint = iter->pointNumber;
//                                        openP.diRection = j;
//                                        openlist_new.push_back(openP);
//                                    }
//                                }
//                            }
//                            bool scanRatio = false;
//                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
//                                // 剔除无法获取的货物
//                                if (iter2->surplus_zheng - distance < 10) {
//                                    iter2 = Cargolist_new.erase(iter2);
//                                }
//                                // 检测是否可以取到货物
//                                else if (iter->pointNumber == iter2->point_number) {
//                                    successful_plan = true;
//                                    //计算价距比
//                                    iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
//                                    if (iter2->value_distance_ratio > max_value_distance_ratio) {
//                                        chooseCargo->point_number = iter2->point_number;
//                                        chooseCargo->item_id = iter2->item_id;
//                                        chooseCargo->path_point = iter2->path_point;
//                                        chooseCargo->path_direction = iter2->path_direction;
//                                        chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
//                                        chooseCargo->money = iter2->money;
//                                        max_value_distance_ratio = iter2->value_distance_ratio;
//                                        scanRatio = true;
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                    else {
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                }
//                                else {
//                                    iter2++;
//                                }
//                            }
//                            if (scanRatio) {
//                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
//                                    if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                    else {
//                                        int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
//                                        if (over_distance < max_scan_distance) {
//                                            max_scan_distance = over_distance;
//                                        }
//                                        iter2++;
//                                    }
//                                }
//                            }
//                        }
//                        openlist = openlist_new;
//                        openlist_new.clear();
//                    }
//                    //回溯路径
//                    if (successful_plan) {
//                        *task_arranged = true;
//                        list<int> path_getting_alternative;
//                        list<int> path_pulling_alternative;
//                        list<int> path_total_alternative;
//                        list<int> direction_total_alternative;
//                        int last_Pnumber = chooseCargo->point_number;
//                        while (closelist[last_Pnumber][1] != -1) {
//                            path_getting_alternative.push_front(last_Pnumber);
//                            path_total_alternative.push_front(last_Pnumber);
//                            direction_total_alternative.push_front(closelist[last_Pnumber][2]);
//                            last_Pnumber = closelist[last_Pnumber][1];
//                        }
//                        path_pulling_alternative = chooseCargo->path_point;
//                        path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
//                        direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
//                        if (!robot[i].path_getting.empty()) {
//                            auto iter2 = robot[i].path_total.begin();
//                            auto iter3 = robot[i].path_direction.begin();
//                            for (auto iter = robot[i].path_getting.begin(); iter != robot[i].path_getting.end();) {
//                                iter = robot[i].path_getting.erase(iter);
//                                iter2 = robot[i].path_total.erase(iter2);
//                                iter3 = robot[i].path_direction.erase(iter3);
//                            }
//                        }
//                        if (!robot[i].path_pulling.empty()) {
//                            auto iter2 = robot[i].path_total.begin();
//                            auto iter3 = robot[i].path_direction.begin();
//                            for (auto iter = robot[i].path_pulling.begin(); iter != robot[i].path_pulling.end();) {
//                                iter = robot[i].path_pulling.erase(iter);
//                                iter2 = robot[i].path_total.erase(iter2);
//                                iter3 = robot[i].path_direction.erase(iter3);
//                            }
//                        }
//
//                        robots[i].path_getting = path_getting_alternative;
//                        robots[i].path_pulling = path_pulling_alternative;
//                        robot[i].path_total = path_total_alternative;
//                        robot[i].path_direction = direction_total_alternative;
//                        robot[i].path_getting_next.clear();
//                        robot[i].path_pulling_next.clear();
//                        for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
//                            if (robot[i].choose_item_id == iter2->item_id) {
//                                iter2->status = 0;
//                            }
//                            for (auto iter = robot[i].choose_item_id_next.begin(); iter != robot[i].choose_item_id_next.end(); iter++) {
//
//                                if (*iter == iter2->item_id) {
//                                    iter2->status = 0;
//                                }
//                            }
//                        }
//                        robot[i].target_berth = chooseCargo->nearestBerthNumber;
//                        robot[i].carried_money = chooseCargo->money;
//                        robot[i].carried = false;
//                        //修改货物状态
//                        int item_id_origin = robot[i].choose_item_id;
//                        for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                            if (iter->item_id == item_id_origin) {
//                                iter->status = 0;
//                            }
//                            if (iter->item_id == chooseCargo->item_id) {
//                                iter->status = 1;
//                            }
//                        }
//                        robot[i].task_allocated = true;
//                        robot[i].choose_item_id = chooseCargo->item_id;
//                        break;
//                    }
//                }
//            }
//        }
//    }
//
//    //Step3: 对即将关闭的泊位，若有超过其容量限制的货物正在运送，调整超出部分小车的去向
//    int node_is_berth_new[n * n];
//    bool shutdown_berth[berth_num] = { false };
//    for (int i = 0; i < n * n; i++) {
//        node_is_berth_new[i] = node_is_berth[i];
//    }
//    for (int i = 0; i < berth_num; i++) {
//        int surplus_berth_capacity;
//        if (!berth[i].closed) {
//            surplus_berth_capacity = berth[i].cargo_capacity;
//        }
//        else {
//            surplus_berth_capacity = 0;
//        }
//        
//        int cargo_on_transporting = 0;
//        int cargo_price[robot_num] = { 0 };
//        for (int j = 0; j < robot_num; j++) {
//            cargo_price[j] = INT_MAX;
//        }
//        for (int j = 0; j < robot_num; j++) {
//            if (robot[j].active && robot[j].target_berth == i) {
//                cargo_on_transporting++;
//                cargo_price[j] = robot[j].carried_money;
//            }
//        }
//        if (cargo_on_transporting > surplus_berth_capacity) {
//            shutdown_berth[i] = true;
//            for (int k = 0; k < n * n; k++) {
//                if (node_is_berth_new[k] == i) {
//                    node_is_berth_new[k] = -1;
//                }
//            }
//            for (int t = 0; t < cargo_on_transporting - surplus_berth_capacity; t++) {
//                int min_price_cargo = INT_MAX;
//                int rearrange_robot_number = -1;
//                for (int j = 0; j < robot_num; j++) {
//                    if (cargo_price[j] < min_price_cargo) {
//                        min_price_cargo = cargo_price[j];
//                        rearranged_robot_number = j;
//                    }
//                }
//                // 重排运输价值最小的货物小车
//                bool rearrange_successful = false;
//                if (!robot[rearranged_robot_number].carried) {
//                    list<Cargo> Cargolist_new;
//                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                        if (iter->status == 0 && !shutdown_berth[iter->nearestBerthNumber]) {
//                            Cargolist_new.push_back(*iter);
//                        }
//                    }
//                    if (!Cargolist_new.empty()) {
//                        //Astar部分
//                        int startP_number = robot[rearranged_robot_number].x * n + robot[rearranged_robot_number].y;
//                        list<openlist_unit> openlist;
//                        list<openlist_unit> openlist_new;
//                        int closelist[n * n][3] = { 0 };
//                        openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                        openlist.push_back(openP);
//                        int distance = -1;
//                        double max_value_distance_ratio = 0;
//                        Cargo* chooseCargo = new Cargo();
//                        int max_scan_distance = 999;
//                        bool successful_plan = false;
//                        while (true) {
//                            if (openlist.empty()) {
//                                break;
//                            }
//                            distance++;
//                            if (distance > max_scan_distance) {
//                                break;
//                            }
//                            for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                                if (closelist[iter->pointNumber][0] == 0) {
//                                    closelist[iter->pointNumber][0] = 1;
//                                    closelist[iter->pointNumber][1] = iter->lastPoint;
//                                    closelist[iter->pointNumber][2] = iter->diRection;
//                                    for (int j = 0; j < 4; j++) {
//                                        if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                            openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                            openP.lastPoint = iter->pointNumber;
//                                            openP.diRection = j;
//                                            openlist_new.push_back(openP);
//                                        }
//                                    }
//                                }
//                                bool scanRatio = false;
//                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
//                                    // 剔除无法获取的货物
//                                    if (iter2->surplus_zheng - distance < 10) {
//                                        iter2 = Cargolist_new.erase(iter2);
//                                    }
//                                    // 检测是否可以取到货物
//                                    else if (iter->pointNumber == iter2->point_number) {
//                                        successful_plan = true;
//                                        //计算价距比
//                                        iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
//                                        if (iter2->value_distance_ratio > max_value_distance_ratio) {
//                                            chooseCargo->point_number = iter2->point_number;
//                                            chooseCargo->item_id = iter2->item_id;
//                                            chooseCargo->path_point = iter2->path_point;
//                                            chooseCargo->path_direction = iter2->path_direction;
//                                            chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
//                                            chooseCargo->money = iter2->money;
//                                            max_value_distance_ratio = iter2->value_distance_ratio;
//                                            scanRatio = true;
//                                            iter2 = Cargolist_new.erase(iter2);
//                                        }
//                                        else {
//                                            iter2 = Cargolist_new.erase(iter2);
//                                        }
//                                    }
//                                    else {
//                                        iter2++;
//                                    }
//                                }
//                                if (scanRatio) {
//                                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
//                                        if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
//                                            iter2 = Cargolist_new.erase(iter2);
//                                        }
//                                        else {
//                                            int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
//                                            if (over_distance < max_scan_distance) {
//                                                max_scan_distance = over_distance;
//                                            }
//                                            iter2++;
//                                        }
//                                    }
//                                }
//                            }
//                            openlist = openlist_new;
//                            openlist_new.clear();
//                        }
//                        //回溯路径
//                        if (successful_plan) {
//                            rearrange_successful = true;
//                            *task_arranged = true;
//                            list<int> path_getting_alternative;
//                            list<int> path_pulling_alternative;
//                            list<int> path_total_alternative;
//                            list<int> direction_total_alternative;
//                            int last_Pnumber = chooseCargo->point_number;
//                            while (closelist[last_Pnumber][1] != -1) {
//                                path_getting_alternative.push_front(last_Pnumber);
//                                path_total_alternative.push_front(last_Pnumber);
//                                direction_total_alternative.push_front(closelist[last_Pnumber][2]);
//                                last_Pnumber = closelist[last_Pnumber][1];
//                            }
//                            path_pulling_alternative = chooseCargo->path_point;
//                            path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
//                            direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
//                            if (!robot[rearranged_robot_number].path_getting.empty()) {
//                                auto iter2 = robot[rearranged_robot_number].path_total.begin();
//                                auto iter3 = robot[rearranged_robot_number].path_direction.begin();
//                                for (auto iter = robot[rearranged_robot_number].path_getting.begin(); iter != robot[rearranged_robot_number].path_getting.end();) {
//                                    iter = robot[rearranged_robot_number].path_getting.erase(iter);
//                                    iter2 = robot[rearranged_robot_number].path_total.erase(iter2);
//                                    iter3 = robot[rearranged_robot_number].path_direction.erase(iter3);
//                                }
//                            }
//                            if (!robot[rearranged_robot_number].path_pulling.empty()) {
//                                auto iter2 = robot[rearranged_robot_number].path_total.begin();
//                                auto iter3 = robot[rearranged_robot_number].path_direction.begin();
//                                for (auto iter = robot[rearranged_robot_number].path_pulling.begin(); iter != robot[rearranged_robot_number].path_pulling.end();) {
//                                    iter = robot[rearranged_robot_number].path_pulling.erase(iter);
//                                    iter2 = robot[rearranged_robot_number].path_total.erase(iter2);
//                                    iter3 = robot[rearranged_robot_number].path_direction.erase(iter3);
//                                }
//                            }
//
//                            robots[rearranged_robot_number].path_getting = path_getting_alternative;
//                            robots[rearranged_robot_number].path_pulling = path_pulling_alternative;
//                            robot[rearranged_robot_number].path_total = path_total_alternative;
//                            robot[rearranged_robot_number].path_direction = direction_total_alternative;
//                            robot[rearranged_robot_number].path_getting_next.clear();
//                            robot[rearranged_robot_number].path_pulling_next.clear();
//                            for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
//                                if (robot[rearranged_robot_number].choose_item_id == iter2->item_id) {
//                                    iter2->status = 0;
//                                }
//                                for (auto iter = robot[rearranged_robot_number].choose_item_id_next.begin(); iter != robot[rearranged_robot_number].choose_item_id_next.end(); iter++) {
//                                    if (*iter == iter2->item_id) {
//                                        iter2->status = 0;
//                                    }
//                                }
//                            }
//                            robot[rearranged_robot_number].choose_item_id_next.clear();
//                            robot[rearranged_robot_number].target_berth = chooseCargo->nearestBerthNumber;
//                            robot[rearranged_robot_number].carried_money = chooseCargo->money;
//                            robot[rearranged_robot_number].carried = false;
//                            //修改货物状态
//                            int item_id_origin = robot[rearranged_robot_number].choose_item_id;
//                            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//                                if (iter->item_id == item_id_origin) {
//                                    iter->status = 0;
//                                }
//                                if (iter->item_id == chooseCargo->item_id) {
//                                    iter->status = 1;
//                                }
//                            }
//                            robot[rearranged_robot_number].task_allocated = true;
//                            robot[rearranged_robot_number].choose_item_id = chooseCargo->item_id;
//                            break;
//                        }
//                    }
//                }
//                else {
//                    //Astar部分
//                    int startP_number = robot[rearranged_robot_number].x * n + robot[rearranged_robot_number].y;
//                    list<openlist_unit> openlist;
//                    list<openlist_unit> openlist_new;
//                    int closelist[n * n][3] = { 0 };
//                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                    openlist.push_back(openP);
//                    int final_endPnumber;
//                    //检查泊位是否都已经关闭，若是，则不进行规划
//                    bool all_closed = true;
//                    for (int i = 0; i < berth_num; i++) {
//                        if (!berth[i].closed) {
//                            all_closed = false;
//                            break;
//                        }
//                    }
//                    if (all_closed) {
//                    }
//                    else {
//                        bool CanBeGot = false;
//                        while (true) {
//                            if (openlist.empty()) {
//                                break;
//                            }
//                            for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                                if (closelist[iter->pointNumber][0] == 0) {
//                                    closelist[iter->pointNumber][0] = 1;
//                                    closelist[iter->pointNumber][1] = iter->lastPoint;
//                                    closelist[iter->pointNumber][2] = iter->diRection;
//                                    for (int j = 0; j < 4; j++) {
//                                        if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                            openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                            openP.lastPoint = iter->pointNumber;
//                                            openP.diRection = j;
//                                            openlist_new.push_back(openP);
//                                        }
//                                    }
//                                }
//                                if (node_is_berth_new[iter->pointNumber] != -1) {
//                                    // 更换泊位
//                                    robot[rearranged_robot_number].target_berth = node_is_berth_new[iter->pointNumber];
//                                    CanBeGot = true;
//                                    final_endPnumber = iter->pointNumber;
//                                    break;
//                                }
//
//                            }
//                            if (CanBeGot) {
//                                break;
//                            };
//                            openlist = openlist_new;
//                            openlist_new.clear();
//                        }
//                        //回溯路径
//                        if (CanBeGot) {
//                            rearrange_successful = true;
//                            list<int> new_path;
//                            list<int> new_direction;
//                            int last_Pnumber = final_endPnumber;
//                            while (closelist[last_Pnumber][1] != -1) {
//                                new_path.push_front(last_Pnumber);
//                                new_direction.push_front(closelist[last_Pnumber][2]);
//                                last_Pnumber = closelist[last_Pnumber][1];
//                            }
//                            robot[rearranged_robot_number].path_pulling = new_path;
//                            robot[rearranged_robot_number].path_direction = new_direction;
//                            robot[rearranged_robot_number].path_total = new_path;
//                            robot[rearranged_robot_number].path_getting_next.clear();
//                            robot[rearranged_robot_number].path_pulling_next.clear();
//                            for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
//                                for (auto iter = robot[rearranged_robot_number].choose_item_id_next.begin(); iter != robot[rearranged_robot_number].choose_item_id_next.end(); iter++) {
//                                    if (*iter == iter2->item_id) {
//                                        iter2->status = 0;
//                                    }
//                                }
//                            }
//                            robot[rearranged_robot_number].choose_item_id_next.clear();
//                        }
//                    }
//                }
//                if (!robot[rearranged_robot_number].carried && !rearrange_successful && !robot[rearranged_robot_number].path_getting.empty())
//                {
//                    //Astar部分
//                    int startP_number = robot[rearranged_robot_number].path_getting.back();
//                    list<openlist_unit> openlist;
//                    list<openlist_unit> openlist_new;
//                    int closelist[n * n][3] = { 0 };
//                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
//                    openlist.push_back(openP);
//                    int final_endPnumber;
//                    //检查泊位是否都已经关闭，若是，则不进行规划
//                    bool all_closed = true;
//                    for (int i = 0; i < berth_num; i++) {
//                        if (!berth[i].closed) {
//                            all_closed = false;
//                            break;
//                        }
//                    }
//                    if (all_closed) {
//                    }
//                    else {
//                        bool CanBeGot = false;
//                        while (true) {
//                            if (openlist.empty()) {
//                                break;
//                            }
//                            for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//                                if (closelist[iter->pointNumber][0] == 0) {
//                                    closelist[iter->pointNumber][0] = 1;
//                                    closelist[iter->pointNumber][1] = iter->lastPoint;
//                                    closelist[iter->pointNumber][2] = iter->diRection;
//                                    for (int j = 0; j < 4; j++) {
//                                        if (Node_movable_node[iter->pointNumber][j] != -1) {
//                                            openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//                                            openP.lastPoint = iter->pointNumber;
//                                            openP.diRection = j;
//                                            openlist_new.push_back(openP);
//                                        }
//                                    }
//                                }
//                                if (node_is_berth_new[iter->pointNumber] != -1) {
//                                    // 更换泊位
//                                    robot[rearranged_robot_number].target_berth = node_is_berth_new[iter->pointNumber];
//                                    CanBeGot = true;
//                                    final_endPnumber = iter->pointNumber;
//                                    break;
//                                }
//
//                            }
//                            if (CanBeGot) {
//                                break;
//                            };
//                            openlist = openlist_new;
//                            openlist_new.clear();
//                        }
//                        //回溯路径
//                        if (CanBeGot) {
//                            rearrange_successful = true;
//                            list<int> new_path;
//                            list<int> new_direction;
//                            int last_Pnumber = final_endPnumber;
//                            while (closelist[last_Pnumber][1] != -1) {
//                                new_path.push_front(last_Pnumber);
//                                new_direction.push_front(closelist[last_Pnumber][2]);
//                                last_Pnumber = closelist[last_Pnumber][1];
//                            }
//                            robot[rearranged_robot_number].path_pulling = new_path;
//                            list<int> direction_getting;
//                            auto iter2 = robot[rearranged_robot_number].path_direction.begin();
//                            for (auto iter = robot[rearranged_robot_number].path_getting.begin(); iter != robot[rearranged_robot_number].path_getting.end(); iter++) {
//                                direction_getting.push_back(*iter2);
//                                iter2++;
//                            }
//                            robot[rearranged_robot_number].path_direction = merge_in_order(direction_getting, new_direction);
//                            robot[rearranged_robot_number].path_total = merge_in_order(robot[rearranged_robot_number].path_getting, new_path);
//                            robot[rearranged_robot_number].path_getting_next.clear();
//                            robot[rearranged_robot_number].path_pulling_next.clear();
//                            for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
//                                for (auto iter = robot[rearranged_robot_number].choose_item_id_next.begin(); iter != robot[rearranged_robot_number].choose_item_id_next.end(); iter++) {
//                                    if (*iter == iter2->item_id) {
//                                        iter2->status = 0;
//                                    }
//                                }
//                            }
//                            robot[rearranged_robot_number].choose_item_id_next.clear();
//                        }
//                    }
//                }
//            }
//            berth[i].closed = true;
//        }
//    }
//
//
//    // Step4: 优化目前已有任务的机器人
//
//    //if (!rearranged){
//    //    for (int m = 0; m < robot_num; m++) {
//    //        int i = rearrange_robot_list[m];
//    //        if (!robots[i].carried) {
//    //            int startP_number = robots[i].x * n + robots[i].y;
//    //            list<Cargo> Cargolist_new;
//    //            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//    //                if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber])
//    //                {
//    //                    Cargolist_new.push_back(*iter);
//    //                }
//    //            }
//    //            if (Cargolist_new.empty()) {
//    //                break;
//    //            }
//    //            rearranged = true;
//    //            //Astar部分
//    //            list<openlist_unit> openlist;
//    //            list<openlist_unit> openlist_new;
//    //            int closelist[n * n][3] = { 0 };
//    //            openlist_unit openP = openlist_unit(startP_number, -1, -1);
//    //            openlist.push_back(openP);
//    //            int distance = -1;
//    //            double max_value_distance_ratio = (double)robots[i].carried_money / (robots[i].path_getting.size() + robots[i].path_pulling.size());
//    //            bool task_changed = false;
//    //            Cargo* chooseCargo = new Cargo();
//    //            int max_scan_distance = 999;
//    //            while (true) {
//    //                if (openlist.empty()) {
//    //                    break;
//    //                }
//    //                distance++;
//    //                if (distance > max_scan_distance) {
//    //                    break;
//    //                }
//    //                for (auto iter = openlist.begin(); iter != openlist.end(); ++iter) {
//    //                    if (closelist[iter->pointNumber][0] == 0) {
//    //                        closelist[iter->pointNumber][0] = 1;
//    //                        closelist[iter->pointNumber][1] = iter->lastPoint;
//    //                        closelist[iter->pointNumber][2] = iter->diRection;
//    //                        for (int j = 0; j < 4; j++) {
//    //                            if (Node_movable_node[iter->pointNumber][j] != -1) {
//    //                                openP.pointNumber = Node_movable_node[iter->pointNumber][j];
//    //                                openP.lastPoint = iter->pointNumber;
//    //                                openP.diRection = j;
//    //                                openlist_new.push_back(openP);
//    //                            }
//    //                        }
//    //                    }
//    //                    bool scanRatio = false;
//    //                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end(); ) {
//    //                        // 剔除无法获取的货物
//    //                        if (iter2->surplus_zheng - distance < 10) {
//    //                            iter2 = Cargolist_new.erase(iter2);
//    //                        }
//    //                        // 检测是否可以取到货物
//    //                        else if (iter->pointNumber == iter2->point_number) {
//    //                            //计算价距比
//    //                            iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
//    //                            if (iter2->value_distance_ratio > max_value_distance_ratio) {
//    //                                task_changed = true;
//    //                                chooseCargo->point_number = iter2->point_number;
//    //                                chooseCargo->item_id = iter2->item_id;
//    //                                chooseCargo->path_point = iter2->path_point;
//    //                                chooseCargo->path_direction = iter2->path_direction;
//    //                                chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
//    //                                chooseCargo->money = iter2->money;
//    //                                max_value_distance_ratio = iter2->value_distance_ratio;
//    //                                scanRatio = true;
//    //                                iter2 = Cargolist_new.erase(iter2);
//    //                            }
//    //                            else {
//    //                                iter2 = Cargolist_new.erase(iter2);
//    //                            }
//    //                        }
//    //                        else {
//    //                            iter2++;
//    //                        }
//    //                    }
//    //                    if (scanRatio) {
//    //                        for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
//    //                            if ((double)iter2->money / (iter2->DistanceToBerth + distance) < max_value_distance_ratio) {
//    //                                iter2 = Cargolist_new.erase(iter2);
//    //                            }
//    //                            else {
//    //                                int over_distance = iter2->money / max_value_distance_ratio - iter2->DistanceToBerth;
//    //                                if (over_distance < max_scan_distance) {
//    //                                    max_scan_distance = over_distance;
//    //                                }
//    //                                iter2++;
//    //                            }
//    //                        }
//    //                    }
//    //                }
//    //                openlist = openlist_new;
//    //                openlist_new.clear();
//    //            }
//    //            //回溯路径
//    //            if (task_changed) {
//    //                *task_arranged = true;
//    //                list<int> path_getting_alternative;
//    //                list<int> path_pulling_alternative;
//    //                list<int> path_total_alternative;
//    //                list<int> direction_total_alternative;
//    //                int last_Pnumber = chooseCargo->point_number;
//    //                while (closelist[last_Pnumber][1] != -1) {
//    //                    path_getting_alternative.push_front(last_Pnumber);
//    //                    path_total_alternative.push_front(last_Pnumber);
//    //                    direction_total_alternative.push_front(closelist[last_Pnumber][2]);
//    //                    last_Pnumber = closelist[last_Pnumber][1];
//    //                }
//    //                path_pulling_alternative = chooseCargo->path_point;
//    //                path_total_alternative = merge_in_order(path_total_alternative, path_pulling_alternative);
//    //                direction_total_alternative = merge_in_order(direction_total_alternative, chooseCargo->path_direction);
//    //                if (!robot[i].path_getting.empty()) {
//    //                    auto iter2 = robot[i].path_total.begin();
//    //                    auto iter3 = robot[i].path_direction.begin();
//    //                    for (auto iter = robot[i].path_getting.begin(); iter != robot[i].path_getting.end();) {
//    //                        iter = robot[i].path_getting.erase(iter);
//    //                        iter2 = robot[i].path_total.erase(iter2);
//    //                        iter3 = robot[i].path_direction.erase(iter3);
//    //                    }
//    //                }
//    //                if (!robot[i].path_pulling.empty()) {
//    //                    auto iter2 = robot[i].path_total.begin();
//    //                    auto iter3 = robot[i].path_direction.begin();
//    //                    for (auto iter = robot[i].path_pulling.begin(); iter != robot[i].path_pulling.end();) {
//    //                        iter = robot[i].path_pulling.erase(iter);
//    //                        iter2 = robot[i].path_total.erase(iter2);
//    //                        iter3 = robot[i].path_direction.erase(iter3);
//    //                    }
//    //                }
//
//    //                robots[i].path_getting = path_getting_alternative;
//    //                robots[i].path_pulling = path_pulling_alternative;
//    //                robot[i].path_total = path_total_alternative;
//    //                robot[i].path_direction = direction_total_alternative;
//    //                robot[i].path_getting_next.clear();
//    //                robot[i].path_pulling_next.clear();
//    //                for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
//    //                    if (robot[i].choose_item_id == iter2->item_id) {
//    //                        iter2->status = 0;
//    //                    }
//    //                    for (auto iter = robot[i].choose_item_id_next.begin(); iter != robot[i].choose_item_id_next.end(); iter++) {
//    //                    
//    //                        if (*iter == iter2->item_id) {
//    //                            iter2->status = 0;
//    //                        }
//    //                    }
//    //                }
//    //                robot[i].target_berth = chooseCargo->nearestBerthNumber;
//    //                robot[i].carried_money = chooseCargo->money;
//    //                robot[i].carried = false;
//    //                
//
//    //                //修改货物状态
//    //                int item_id_origin = robot[i].choose_item_id;
//    //                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
//    //                    if (iter->item_id == item_id_origin) {
//    //                        iter->status = 0;
//    //                    }
//    //                    if (iter->item_id == chooseCargo->item_id) {
//    //                        iter->status = 1;
//    //                    }
//    //                }
//    //                robot[i].choose_item_id = chooseCargo->item_id;
//    //                robot[i].task_allocated = true;
//    //                break;
//    //            }
//    //        }
//    //    }
//    //}
//}


/*
* 货船任务规划函数
*/
void BoatPlan(Boat boats[], Berth berths[], Robot robots[], int zhen_now)
{
    for (int i = 0; i < boat_num; i++) {
        if (zhen_now < boat[i].surplus_time) {
            continue;
        }
        //Step 1 当前帧：货船运完货，移动到泊位
        if (boat[i].status == 1 && boat[i].pos == -1) {
            boat[i].cargo_number = 0;
            if (!boat[i].berths_to_go.empty() && boat[i].berths_to_go.size() > 1) {
                // 非最后一回合
                auto target_berth_iter = boat[i].berths_to_go.begin();
                boat[i].need_to_ship = true;
                boat[i].shipping_target = *target_berth_iter;
                boat[i].berths_to_go.erase(target_berth_iter);
            }
            else
            {
                // 最后一回合等到时间再开始运输
                if (zhen_now >= boat[i].final_trip_start_time) {
                    auto target_berth_iter = boat[i].berths_to_go.begin();
                    boat[i].need_to_ship = true;
                    boat[i].shipping_target = *target_berth_iter;
                    boat[i].berths_to_go.erase(target_berth_iter);
                }
            }
        }
        //Step 2 当前帧：货船移动到泊位，开始装货，计算停留时间
        if (boat[i].status != 0 && boat[i].pos != -1 && !boat[i].at_berth) {
            boat[i].at_berth = true;
            int target_berth = boat[i].shipping_target;
            boat[i].shipping_target = -1;
            boat[i].now_berth = target_berth;
            boat[i].remaining_time = boat[i].capacity / berth[target_berth].loading_speed + 1;
            // 查错
            if (boat[i].remaining_time < -2) {
                int a = 1;
            }
        }

        //Step 3 当前帧：货船正在装货，更新停留时间
        if (boat[i].remaining_time > 0) {
            boat[i].remaining_time--;
            int now_berth = boat[i].now_berth;
            if (!berth[now_berth].goods.empty()) {
                auto iter = berth[now_berth].goods.begin();
                for (int j = 0; j < berth[now_berth].loading_speed; j++) {
                    if (boat[i].cargo_number == boat[i].capacity) {
                        break;
                    }
                    iter = berth[now_berth].goods.erase(iter);
                    boat[i].cargo_number++;
                    if (iter == berth[now_berth].goods.end()) {
                        break;
                    }
                }
            }
        }
        //Step 4 当前帧：货船装载货物结束，开始运送
        if (boat[i].at_berth && boat[i].remaining_time == 0) {
            boat[i].at_berth = false;
            boat[i].need_to_go = true;
            // 检查是否需要关闭港口
            int leaving_berth = boat[i].now_berth;
            bool final_ship = true;
            for (int j = 0; j < boat_num; j++) {
                if (j != i && boat[j].shipping_target == leaving_berth) {
                    final_ship = false;
                }
                if (j != i && boat[j].at_berth && boat[j].now_berth == leaving_berth) {
                    final_ship = false;
                }
                if (!boat[j].berths_to_go.empty()) {
                    for (auto iter = boat[j].berths_to_go.begin(); iter != boat[j].berths_to_go.end(); iter++) {
                        if (*iter == leaving_berth) {
                            final_ship = false;
                        }
                    }
                }
            }
            if (final_ship) {
                berth[leaving_berth].closed = true;
                for (int j = 0; j < robot_num; j++) {
                    if ( robot[j].berthsCanBeReached[leaving_berth])
                    {
                        robot[j].berthsCanBeReached[leaving_berth] = false;
                        bool all_cant_reach = true;
                        for (int k = 0; k < berth_num; k++) {
                            if (robot[j].berthsCanBeReached[k]) {
                                all_cant_reach = false;
                            }
                        }
                        if (all_cant_reach) {
                            robot[j].active = false;
                            robot[j].path_getting.clear();
                            robot[j].path_pulling.clear();
                            robot[j].path_getting_next.clear();
                            robot[j].path_pulling_next.clear();
                            robot[j].path_total.clear();
                            robot[j].path_direction.clear();
                        }
                    }
                }
            }
        }
    }
}

/*
* 货物重定向函数
*/
//void cargo_reFind_berth(list<Cargo> Cargolist, int Node_movable_directions[][4], int Node_movable_node[][4], int Node_is_berth[])
//{
//    for (int t = 0; t < 10; t++) {
//        for (auto iter = Cargolist.begin(); iter != Cargolist.end(); iter++) {
//            if (berth[iter->nearestBerthNumber].closed && iter->status == 0) {
//
//                Cargo cargo = Cargo(iter->x, iter->y, iter->item_id, iter->item_id, iter->surplus_zheng, 0);
//                int StartP = iter->x * n + iter->y;
//                Cargolist.erase(iter);
//                if (Node_movable_node[StartP][0] + Node_movable_node[StartP][1] + Node_movable_node[StartP][2] + Node_movable_node[StartP][3] != -4) {
//                    precalc_CargoToBerth(&cargo, berth, Node_movable_node, Node_is_berth);
//                    if (cargo.CanBeGot) {
//                        Cargolist.push_back(cargo);
//                    }
//                }
//                break;
//            }
//        }
//    }
//}

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
                    if (ch[i][j - 1] == '.' || ch[i][j - 1] == 'A' || ch[i][j - 1] == 'B') {
                        Node_movable_directions[(i - 1) * n + j - 1][1] = 1;
                        Node_movable_node[(i - 1) * n + j - 1][1] = (i - 1) * n + j - 2;
                    }
                }
                if (i > 1) {
                    if (ch[i - 1][j] == '.' || ch[i - 1][j] == 'A' || ch[i - 1][j] == 'B') {
                        Node_movable_directions[(i - 1) * n + j - 1][2] = 1;
                        Node_movable_node[(i - 1) * n + j - 1][2] = (i - 2) * n + j - 1;
                    }
                }
                if (i < n) {
                    if (ch[i + 1][j] == '.' || ch[i + 1][j] == 'A' || ch[i + 1][j] == 'B') {
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
    for (int i = 0; i < berth_num; i++)
    {
        int id;
        scanf("%d", &id);
        scanf("%d%d%d%d", &berth[id].x, &berth[id].y, &berth[id].transport_time, &berth[id].loading_speed);
        fprintf(outputFile, "%d %d %d %d %d\n", id, berth[id].x, berth[id].y, berth[id].transport_time, berth[id].loading_speed);
        Berth_information[i][0] = id;
        Berth_information[i][1] = berth[id].x;
        Berth_information[i][2] = berth[id].y;
        Berth_information[i][3] = berth[id].transport_time;
        Berth_information[i][4] = berth[id].loading_speed;
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
    // 计算轮船在所有泊位来回运输一船货物的总耗时
    for (int i = 0; i < berth_num; i++) {
        berth[i].round_trip_transport_time = berth[i].transport_time * 2 + (boat_capacity / berth[i].loading_speed + 1) + 2; // 预留两帧容错
    }

    char okk[100];
    scanf("%s", okk);
    fprintf(outputFile, "%s\n", okk);
    //////////////////////////////////////////////// 输入完毕////////////////////////////////////////////////////////////////////////////////
    /*
    * 轮船排班开始
    */
    for (int i = 0; i < boat_num; i++) {
        boat[i].capacity = boat_capacity;
    }

    /*
    * 随机货物实验：
    * 随机生成 rand_cargo_numbe 个货物于可抵达的节点上，计算这些货物到所有泊位的距离。如果无法抵达，按400计算。
    * 统计抵达所有泊位的时间距离
    * 统计被选择为第一泊位的次数
    * 用于优化轮船的排班
    */
    const int rand_cargo_number = 200;

    //执行若干次货物实验
    //int berth_be_choosen_times[berth_num] = { 0 };
    //int distance_to_berth_test[berth_num][rand_cargo_number];
    //for (int i = 0; i < berth_num; i++) {
    //    for (int j = 0; j < rand_cargo_number; j++) {
    //        distance_to_berth_test[i][j] = n * 2;
    //    }
    //}
    ////srand((unsigned)time(NULL));
    //for (int num = 0; num < rand_cargo_number; num++) {
    //    while (true) {
    //        int point_number = rand() % (n * n);
    //        if (Node_movable_node[point_number][0] + Node_movable_node[point_number][1] + Node_movable_node[point_number][2] + Node_movable_node[point_number][3] != -4) {
    //            bool first_reached = false;
    //            list<int> openlist;
    //            list<int> openlist_new;
    //            int closelist[n * n] = { 0 };
    //            openlist.push_back(point_number);
    //            int distance = -1;
    //            bool berth_reached[berth_num] = { false };
    //            while (true) {
    //                if (openlist.empty()) {
    //                    break;
    //                }
    //                distance++;
    //                bool all_reached = true;
    //                for (auto iter = openlist.begin(); iter != openlist.end(); iter++) {
    //                    closelist[*iter] = 1;
    //                    int point_x = *iter / n;
    //                    int point_y = *iter % n;
    //                    for (int i = 0; i < berth_num; i++) {
    //                        if (!berth_reached[i] &&point_x - berth[i].x <= 3 && point_y - berth[i].y <= 3 && point_x - berth[i].x >= 0 && point_y - berth[i].y >= 0) {
    //                            berth_reached[i] = true;
    //                            if (!first_reached) {
    //                                first_reached = true;
    //                                berth_be_choosen_times[i]++;
    //                            }
    //                            distance_to_berth_test[i][num] = distance;
    //                        }
    //                    }
    //                    for (int j = 0; j < 4; j++) {
    //                        if (Node_movable_node[*iter][j] != -1 && closelist[Node_movable_node[*iter][j]] == 0) {
    //                            closelist[Node_movable_node[*iter][j]] = 1;
    //                            openlist_new.push_back(Node_movable_node[*iter][j]);
    //                        }
    //                    }
    //                }
    //                for (int i = 0; i < berth_num; i++) {
    //                    if (!berth_reached[i]) {
    //                        all_reached = false;
    //                    }
    //                }
    //                if (all_reached) {
    //                    break;
    //                }
    //                openlist = openlist_new;
    //                openlist_new.clear();
    //            }
    //            break;
    //        }
    //    }
    //}



    //double test_score_berths[berth_num] = { 0 };
    //for (int i = 0; i < berth_num; i++) {
    //    for (int j = 0; j < rand_cargo_number; j++) {
    //        test_score_berths[i] = test_score_berths[i] + distance_to_berth_test[i][j];
    //    }
    //}
    //for (int i = 0; i < berth_num; i++) {
    //    test_score_berths[i] = test_score_berths[i] / rand_cargo_number;
    //    test_score_berths[i] = test_score_berths[i] + berth[i].transport_time;
    //}


    // 查错/////////////////////////////////////////////////////////////////////////////
    int berth_be_choosen_times[berth_num] = { 32, 20, 25, 29, 15, 13, 14, 14, 18, 20 };
    double test_score_berths[berth_num] = { 922.62, 1202.44, 1105.23, 917.65, 1204.21, 1085.7, 1239.87, 918.28, 1170.54, 1345.39 };

    //冒泡排序
    int IDX_test_score_berths[berth_num] = { 0 };
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
                    //选择最终泊位
                    boat[i].final_berth = choose_berth;
                    boat[i].berths_to_go.push_front(choose_berth);
                    //计算剩余可分配时间
                    boat[i].final_trip_start_time = total_zhen - berth[choose_berth].round_trip_transport_time - 5;//预留5帧时间容错，防止跳帧影响
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
                //选择最终泊位
                boat[i].final_berth = choose_berth;
                boat[i].berths_to_go.push_front(choose_berth);
                //计算剩余可分配时间
                boat[i].final_trip_start_time = total_zhen - berth[choose_berth].round_trip_transport_time - 5;//预留5帧时间容错，防止跳帧影响
                berths_choosed[choose_berth] = true;
                break;
            }
        }
    }

    // 对剩余可分配时间进行排班

    // Step1 粗略计算能够安排货船的总班次
    int mean_round_trip_time = 0;
    for (int i = 0; i < berth_num; i++) {
        mean_round_trip_time = mean_round_trip_time + berth[i].round_trip_transport_time;
    }
    mean_round_trip_time = mean_round_trip_time / berth_num;

    int estimated_rounds = boat_num;
    for (int i = 0; i < boat_num; i++) {
        estimated_rounds = estimated_rounds + (boat[i].final_trip_start_time - 1000) / mean_round_trip_time;
    }

    double estimated_rounds_allocate_to_berths[berth_num] = { 3.5, 2.5, 2.5, 2.5, 2.5, 2.5, 3.5, 2.5, 2.5, 2.5 };;
    //for (int i = 0; i < berth_num; i++) {
    //    estimated_rounds_allocate_to_berths[i] = (double)estimated_rounds / rand_cargo_number * berth_be_choosen_times[i];
    //}
    


    // Step2 击毙不常用泊位
    //for (int i = 0; i < berth_num; i++) {
    //    if (estimated_rounds_allocate_to_berths[i] < 0.5) {
    //        berth[i].closed = true;
    //    }
    //}

    // Step3 为每个船只制定排班方案
    for (int i = 0; i < boat_num; i++) {
        estimated_rounds_allocate_to_berths[boat[i].final_berth] = estimated_rounds_allocate_to_berths[boat[i].final_berth] - 1;
        boat[i].surplus_time = boat[i].final_trip_start_time;
    }
    int ii = 0;
    for (int j = 0; j < berth_num; j++) {
        if (berths_choosed[j]) {
            continue;
        }
        if (berth[j].closed) {
            continue;
        }
        if (estimated_rounds_allocate_to_berths[j] > 0) {
            if (boat[ii].surplus_time > berth[j].round_trip_transport_time) {
                boat[ii].berths_to_go.push_front(j);
                estimated_rounds_allocate_to_berths[j] = estimated_rounds_allocate_to_berths[j] - 1;
                boat[ii].surplus_time = boat[ii].surplus_time - berth[j].round_trip_transport_time;
            }
        }
        if (ii < boat_num - 1) {
            ii++;
        }
        else {
            ii = 0;
        }
    }


    while (true) {
        int j = 0;
        bool berths_can_not_arrange_boat[berth_num] = { false };
        for (int i = 0; i < berth_num; i++) {
            if (estimated_rounds_allocate_to_berths[i] < 0) {
                berths_can_not_arrange_boat[i] = true;
                continue;
            }
            int try_time = 0;
            while (true) {
                if (boat[j].surplus_time < berth[i].round_trip_transport_time) {
                    if (j < boat_num - 1) {
                        j++;
                    }
                    else {
                        j = 0;
                    }
                    try_time++;
                    if (try_time == boat_num) {
                        berths_can_not_arrange_boat[i] = true;
                        break;
                    }
                }
                else {
                    boat[j].berths_to_go.push_front(i);
                    estimated_rounds_allocate_to_berths[i] = estimated_rounds_allocate_to_berths[i] - 1;
                    boat[j].surplus_time = boat[j].surplus_time - berth[i].round_trip_transport_time;
                    if (j < boat_num - 1) {
                        j++;
                    }
                    else {
                        j = 0;
                    }
                    break;
                }
            }

        }
        bool arrange_finished = true;
        for (int i = 0; i < berth_num; i++) {
            if (berths_can_not_arrange_boat[i]) {
                continue;
            }
            else {
                arrange_finished = false;
                break;
            }
        }
        if (arrange_finished) {
            break;
        }
    }

    //Step 4 给泊位统计还可搬运的货物
    for (int i = 0; i < boat_num; i++) {
        if (!boat[i].berths_to_go.empty()) {
            for (auto iter = boat[i].berths_to_go.begin(); iter != boat[i].berths_to_go.end(); iter++) {
                berth[*iter].cargo_capacity = berth[*iter].cargo_capacity + boat[i].capacity;
            }
        }
    }

    printf("OK\n");
    fflush(stdout);
}

int Input(list<Cargo>* Cargolist_Preliminary, int Node_movable_node[][4], int Node_is_berth[], int* item_id, bool* cargo_added, Robot robotsTest[])
{
    scanf("%d%d", &id, &money);
    fprintf(outputFile, "%d %d\n", id, money);
    int money1 = 0;
    for (int i = 0; i < berth_num; i++) {
        money1 = money1 + berth[i].money_total;
    }
    fprintf(money_statistic, "%d\n", money1);

    // 检查泊位状态，关闭货物达阈的泊位
    for (int i = 0; i < berth_num; i++) {
        if (berth[i].cargo_capacity <= 0) {
            berth[i].closed = true;
            for (int j = 0; j < n * n; j++) {
                if (Node_is_berth[j] == i) {
                    Node_is_berth[j] = -1;
                }
            }
        }
    }


    // 读入新增货物信息，将新增的cargo扩展到Cargolist中。
    int num;
    scanf("%d", &num);
    fprintf(outputFile, "%d\n", num);
    if (num >= 1) {
        *cargo_added = true;
    }
    for (int i = 1; i <= num; i++)
    {
        int x, y, val;
        scanf("%d%d%d", &x, &y, &val);
        fprintf(outputFile, "%d %d %d\n", x, y, val);
        *item_id = *item_id + 1;
        Cargo cargo = Cargo(x, y, val, *item_id, cargo_remain_time, 0);
        Cargolist_Preliminary->push_back(cargo);
    }
    for (int i = 0; i < robot_num; i++)
    {
        int sts;
        scanf("%d%d%d%d", &robot[i].goods, &robot[i].x, &robot[i].y, &sts);
        fprintf(outputFile, "%d %d %d %d\n", robot[i].goods, robot[i].x, robot[i].y, sts);
        robot[i].status = sts;
        if (id > 1) {
            //robot[i].x = robotsTest[i].x;
            //robot[i].y = robotsTest[i].y;
        }

    }
    for (int i = 0; i < 5; i++) {
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
    for (int i = 0; i < n * n; i++) {
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
    Init(Node_movable_directions, Node_movable_node, Berth_information, Node_is_berth, &Boat_capacity, Robot_init);

    /*
    * 创建货物信息表
    */
    auto Cargolist = new list<Cargo>;
    auto Cargolist_Preliminary = new list<Cargo>;
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
        if (zhen == 13856) {
            int a = 1;
        }
        if (zhen == 135) {
            int a = 1;
        }
        if (zhen == 14500) {
            int a = 1;
        }
        if (!Cargolist->empty()) {
            if (Cargolist->front().item_id == -1) {
                int a = 1;
            }
        }

        for (int i = 0; i < berth_num; i++) {
            if (berth[i].cargo_capacity < berth[i].goods.size()) {
                int a = 1;
            }
        }


        /*
        *  Step 1 : 处理输入的货物
        */
        bool cargo_added = false;
        int id = Input(Cargolist_Preliminary, Node_movable_node, Node_is_berth, &item_id, &cargo_added, robotTest);  //每一帧的输入
        if (!Cargolist_Preliminary->empty()) {
            auto iter = Cargolist_Preliminary->begin();
            int StartP = iter->x * n + iter->y;
            if (Node_movable_node[StartP][0] + Node_movable_node[StartP][1] + Node_movable_node[StartP][2] + Node_movable_node[StartP][3] != -4) {
                precalc_CargoToBerth(&*iter, berth, Node_movable_node, Node_is_berth);
                if (iter->CanBeGot) {
                    Cargolist->push_back(*iter);
                }
            }
            Cargolist_Preliminary->erase(iter);
        }

        /*
        * 检查是否仍有货物送往无效泊位
        */



        // 查错②
        if (zhen > 1) {
            for (int i = 0; i < robot_num; i++) {
                if (robot[i].x != robotTest[i].x || robot[i].y != robotTest[i].y) {
                    int a = 1;
                }
                if (!robot[i].path_total.empty()) {
                    bool A = true;
                }
                if (!robot[i].path_getting.empty()) {
                    if (robot[i].path_total.front() != robot[i].path_getting.front()) {
                        int a = 1;
                    }
                }
                else if (!robot[i].path_pulling.empty()) {
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
            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                if (iter->item_id == 3 && iter->status == 1) {
                    int a = 1;
                }
            }
        }

        for (int i = 0; i < berth_num; i++) {
            if (berth[i].closed) {
                int a = 1;
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
            auto ite2 = Cargolist_Preliminary->begin();
            while (ite2 != Cargolist_Preliminary->end()) {
                if (ite2->surplus_zheng == 0) {
                    //查错③
                    if (ite->status == 1) {
                        int a = 1;
                    }
                    ite2 = Cargolist_Preliminary->erase(ite2);
                }
                else if (ite2->status == 2) {
                    ite2 = Cargolist_Preliminary->erase(ite2);
                }
                else {
                    ite2++;
                }
            }
        }

        /*
        *  Step 2 : 刷新当前机器人的状态
        */
        StatusRefresh();


        /*
        *  Step 3 : 执行小车的路径规划
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
        if (cargo_unchoosen) {
            TaskArrange(*Cargolist, Node_movable_node);
        }

        /*
        *  Step 4 : 处理上一步规划好的路径
        */

        PlanProce()




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
            //optimize_task(robot, start_rearrange_robot_number, Cargolist, Node_movable_node, &task_arranged, Node_is_berth);
        }
        // }

         /*
         * 对路径即将走完的机器人预先规划后续路径
         */

        //path_Preplan(robot, limit_time, Cargolist, Node_movable_node, &task_arranged);


        /*
        * 检查路径冲突
        */


        //SolveDeadlock(robot, scan_range, Node_movable_node);

        /*
        * 对存储的路径处理格式并输出
        */

        /*
        * 规划轮船
        */
        //BoatPlan(boat, berth, robot, zhen);
        //for (int i = 0; i < n * n; i++) {
        //    if (Node_is_berth[i] != -1) {
        //        if (berth[Node_is_berth[i]].closed) {
        //            Node_is_berth[i] = -1;
        //        }
        //    }
        //}


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
                            berth[robot[i].target_berth].cargo_capacity--;
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