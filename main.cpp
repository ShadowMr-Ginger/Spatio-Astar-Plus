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
    bool berthsCanBeReached[berth_num] = { false };
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

Robot robotTest[robot_num + 10];

struct Berth
{
    int x;
    int y;
    int transport_time;
    int round_trip_transport_time; // �����ڸò�λ��������һ������ĺ�ʱ
    int loading_speed;
    int cargo_capacity = 0;//��λ�������͵Ļ�����
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
    int capacity;// �ִ����ػ���

    list<int> berths_to_go;// �ִ����Ű��
    int final_berth = -1;// ����ǰ���Ĳ�λ
    int final_trip_start_time = -1; // ����ǰ���ò�λ��ʱ��

    // ״̬�Լ�ָ�
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



struct Cargo
{
    int x;
    int y;
    int point_number;
    int money;
    int surplus_zheng;
    int item_id;
    int status;  // ״̬��0���ã�1���ڱ�һ��������ѡ����2�Ѿ�����ȡ
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

    // ����һ�������Ԫ����ӵ��ϲ����������
    for (const auto& elem : list1) {
        merged_list.push_back(elem);
    }

    // ���ڶ��������Ԫ����ӵ��ϲ����������
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
        //Astar����
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

void precalc_CargoToBerth(Cargo* cargo, Berth berths[], int Node_movable_node[][4], int Node_is_berth[], bool* CanBeGot, int* BerthNumber, int* distance)
{ // Ԥ������ﵽ�����λ�ľ��롣����޷��ִﲴλ����Ϊ�û��ﲻ�ɻ�ȡ��
    int x_start = cargo->x;
    int y_start = cargo->y;
    int x_end[berth_num] = { 0 };
    int y_end[berth_num] = { 0 };
    for (int j = 0; j < berth_num; j++) {
        x_end[j] = berths[j].x;
        y_end[j] = berths[j].y;
    }
    //Astar����
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
    *distance = -1;
    int final_endPnumber;
    //��鲴λ�Ƿ��Ѿ��رգ����ǣ��򲻽��й滮
    bool all_closed = true;
    for (int i = 0; i < berth_num; i++) {
        if (!berth[i].closed) {
            all_closed = false;
            break;
        }
    }
    if (all_closed) {
        *CanBeGot = false;
    }
    else {
        while (true) {
            if (openlist.empty()) {
                break;
            }
            *distance = *distance + 1;
            if (*distance > 200) {
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
        //����·��
        if (*CanBeGot) {
            int last_Pnumber = final_endPnumber;
            while (closelist[last_Pnumber][1] != -1) {
                cargo->path_point.push_front(last_Pnumber);
                cargo->path_direction.push_front(closelist[last_Pnumber][2]);
                last_Pnumber = closelist[last_Pnumber][1];
            }
        }
    }
}



/*
********* ���ĺ��� ***********
* �����˵��Ⱥ���
* ѡ�����ż۾�ȵĻ��ﲢ·���滮
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
    if (!Cargolist_new.empty()) {
        //Astar����
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
                    // �޳��޷���ȡ�Ļ���
                    if (iter2->surplus_zheng - distance < 20) {
                        iter2 = Cargolist_new.erase(iter2);
                    }
                    // ����Ƿ����ȡ������
                    else if (iter->pointNumber == iter2->point_number) {
                        successful_plan = true;
                        //����۾��
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
        if (successful_plan) {
            *task_arranged = true;
            //����·��
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
            //�޸Ļ���״̬
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
********* ���ĺ��� ***********
* ����������������·���ع滮����
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
    //Astar_plus����
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
                        //�ж�����
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
                        //�жϳ�ײ
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
                //�ɵִ�
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
            // ���ɵִ�ڵ��Ƿ���Ҫ����Ϊ����
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
            // ��������ڵ��Ƿ��ܹ�����Ϊ�ɵִ�
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
        //����·��
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
            //����Ƿ������ײ·��
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
            //����Ƿ��������·��
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


    //·���ع滮
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
                // �ع滮robot1
                main_robot_number = iter->robot1;
                endP_number = iter->robot1_target;
                A_star_plus_path_rearrange(robots, main_robot_number, endP_number, robots_paths, Node_movable_node, &rearrange_path_points, &rearrange_path_directions, &no_solution);
                // �� robot1 �滮ʧ�ܣ������ع滮robot2
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
********* ���ĺ��� ***********
* ·��Ԥ�滮����
* �Դ洢·����������Ļ����˽���·��Ԥ�滮
*/
void path_Preplan(Robot robots[], int limit_time, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged, int max_scan_distance)
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
                if (!robots[i].path_total.empty()) {
                    startP_number = robots[i].path_total.back();
                }
                else {
                    startP_number = robots[i].x * n + robots[i].y;
                }
                list<Cargo> Cargolist_new;
                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                    if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber] && !berth[iter->nearestBerthNumber].closed)
                    {
                        Cargolist_new.push_back(*iter);
                    }
                }
                if (Cargolist_new.empty()) {
                    bool successful_plan = false;
                }
                rearranged = true;
                //Astar����
                list<openlist_unit> openlist;
                list<openlist_unit> openlist_new;
                int closelist[n * n][3] = { 0 };
                openlist_unit openP = openlist_unit(startP_number, -1, -1);
                openlist.push_back(openP);
                int distance = -1;
                double max_value_distance_ratio = 0;
                Cargo* chooseCargo = new Cargo();
                //int max_scan_distance = 300;
                int Node_is_cargo[n * n];
                for (int n1 = 0; n1 < n * n; n1++) {
                    Node_is_cargo[n1] = -1;
                }
                for (auto iter = Cargolist_new.begin(); iter != Cargolist_new.end(); iter++) {
                    Node_is_cargo[iter->point_number] = iter->item_id;
                }
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
                            if (Node_is_cargo[iter->pointNumber]!=-1) {
                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                    if (iter2->item_id != Node_is_cargo[iter->pointNumber]) {
                                        iter2++;
                                        continue;
                                    }
                                    scanRatio = true;
                                    successful_plan = true;
                                    //����۾��
                                    iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                                    if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                        chooseCargo->point_number = iter2->point_number;
                                        chooseCargo->item_id = iter2->item_id;
                                        chooseCargo->path_point = iter2->path_point;
                                        chooseCargo->path_direction = iter2->path_direction;
                                        chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                        chooseCargo->money = iter2->money;
                                        max_value_distance_ratio = iter2->value_distance_ratio;
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                    else {
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                }
                            }
                            if (scanRatio) {
                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                    // �޳��޷���ȡ�Ļ���
                                    if (iter2->surplus_zheng - distance < 20) {
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
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
                //����·��
                if (successful_plan) {
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
                    //�޸Ļ���״̬
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
                    // û�п��еĹ滮·��  ����һ������ȡ��·��

                    rearranged = true;
                    //Astar����
                    list<openlist_unit> openlist;
                    list<openlist_unit> openlist_new;
                    int closelist[n * n][3] = { 0 };
                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
                    openlist.push_back(openP);
                    int distance = -1;
                    Cargo* chooseCargo = new Cargo();
                    chooseCargo->money = 0;
                    int max_scan_distance = 75;
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
                    //����·��
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
                            else if (closelist[last_Pnumber][2] == 1) {
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
                        //�޸Ļ���״̬
                        robot[i].task_allocated = true;
                        if (robot[i].path_total.size() > limit_time) {
                            break;
                        }
                    }

                    break;
                }
            }
            if (rearranged) {
                break;
            }
        }
    }
}

/*
********* ���ĺ��� ***********
* �Ż�����Ŀ�꺯��
* ������а�������Ļ����ˣ�����δЯ�����ɨ���������ﲢ���Ǹ������ŵİ���Ŀ��
*/
void optimize_task(Robot robots[], int start_rearrange_robot_number, list<Cargo>* Cargolist, int Node_movable_node[][4], bool* task_arranged, int node_is_berth[], int max_scan_distance)
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
    // Step1 : �����ع滮�������·���Ļ�����
    bool rearranged = false;
    int rearranged_robot_number = -1;
    for (int m = 0; m < robot_num; m++) {
        if (rearranged) {
            break;
        }
        int i = rearrange_robot_list[m];
        if (robot[i].active){
            if (robot[i].choose_item_id == -1) {
                int startP_number;
                startP_number = robots[i].x * n + robots[i].y;
                list<Cargo> Cargolist_new;
                for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                    if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber] && !berth[iter->nearestBerthNumber].closed)
                    {
                        Cargolist_new.push_back(*iter);
                    }
                }
                if (Cargolist_new.empty()) {
                    break;
                }
                rearranged = true;
                //Astar����
                list<openlist_unit> openlist;
                list<openlist_unit> openlist_new;
                int closelist[n * n][3] = { 0 };
                openlist_unit openP = openlist_unit(startP_number, -1, -1);
                openlist.push_back(openP);
                int distance = -1;
                double max_value_distance_ratio = 0;
                Cargo* chooseCargo = new Cargo();
                //int max_scan_distance = 300;
                bool successful_plan = false;
                int Node_is_cargo[n * n];
                for (int n1 = 0; n1 < n * n; n1++) {
                    Node_is_cargo[n1] = -1;
                }
                for (auto iter = Cargolist_new.begin(); iter != Cargolist_new.end(); iter++) {
                    Node_is_cargo[iter->point_number] = iter->item_id;
                }
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
                        if (Node_is_cargo[iter->pointNumber] != -1) {
                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                if (iter2->item_id != Node_is_cargo[iter->pointNumber]) {
                                    iter2++;
                                    continue;
                                }
                                scanRatio = true;
                                successful_plan = true;
                                //����۾��
                                iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                                if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                    chooseCargo->point_number = iter2->point_number;
                                    chooseCargo->item_id = iter2->item_id;
                                    chooseCargo->path_point = iter2->path_point;
                                    chooseCargo->path_direction = iter2->path_direction;
                                    chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                    chooseCargo->money = iter2->money;
                                    max_value_distance_ratio = iter2->value_distance_ratio;
                                    iter2 = Cargolist_new.erase(iter2);
                                }
                                else {
                                    iter2 = Cargolist_new.erase(iter2);
                                }
                            }
                        }
                        if (scanRatio) {
                            for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                // �޳��޷���ȡ�Ļ���
                                if (iter2->surplus_zheng - distance < 20) {
                                    iter2 = Cargolist_new.erase(iter2);
                                }
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
                //����·��
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

                    robot[i].path_getting = path_getting_alternative;
                    robot[i].path_pulling = path_pulling_alternative;
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
                    //�޸Ļ���״̬
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
    }

    // Step2: ����Ƿ���ڻ������޷���ʱȡ�������У����¹滮ȡ��·��
    for (int m = 0; m < robot_num; m++){
        if (rearranged) {
            break;
        }
        int i = rearrange_robot_list[m];
        if (robot[i].active){
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
                        if (iter->status == 0 && robots[i].berthsCanBeReached[iter->nearestBerthNumber] && !berth[iter->nearestBerthNumber].closed)
                        {
                            Cargolist_new.push_back(*iter);
                        }
                    }
                    if (Cargolist_new.empty()) {
                        break;
                    }
                    rearranged = true;
                    //Astar����
                    list<openlist_unit> openlist;
                    list<openlist_unit> openlist_new;
                    int closelist[n * n][3] = { 0 };
                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
                    openlist.push_back(openP);
                    int distance = -1;
                    double max_value_distance_ratio = 0;
                    Cargo* chooseCargo = new Cargo();
                    //int max_scan_distance = 300;
                    bool successful_plan = false;
                    int Node_is_cargo[n * n];
                    for (int n1 = 0; n1 < n * n; n1++) {
                        Node_is_cargo[n1] = -1;
                    }
                    for (auto iter = Cargolist_new.begin(); iter != Cargolist_new.end(); iter++) {
                        Node_is_cargo[iter->point_number] = iter->item_id;
                    }
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
                            if (Node_is_cargo[iter->pointNumber] != -1) {
                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                    if (iter2->item_id != Node_is_cargo[iter->pointNumber]) {
                                        iter2++;
                                        continue;
                                    }
                                    scanRatio = true;
                                    successful_plan = true;
                                    //����۾��
                                    iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                                    if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                        chooseCargo->point_number = iter2->point_number;
                                        chooseCargo->item_id = iter2->item_id;
                                        chooseCargo->path_point = iter2->path_point;
                                        chooseCargo->path_direction = iter2->path_direction;
                                        chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                        chooseCargo->money = iter2->money;
                                        max_value_distance_ratio = iter2->value_distance_ratio;
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                    else {
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
                                }
                            }
                            if (scanRatio) {
                                for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                    // �޳��޷���ȡ�Ļ���
                                    if (iter2->surplus_zheng - distance < 20) {
                                        iter2 = Cargolist_new.erase(iter2);
                                    }
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
                    //����·��
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
                        //�޸Ļ���״̬
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
    }

    //Step3: �Լ����رյĲ�λ�����г������������ƵĻ����������ͣ�������������С����ȥ��
    
    int node_is_berth_new[n * n];
    bool shutdown_berth[berth_num] = { false };
    for (int i = 0; i < n * n; i++) {
        node_is_berth_new[i] = node_is_berth[i];
    }
    for (int i = 0; i < berth_num; i++) {
        if (rearranged) {
            break;
        }
        int surplus_berth_capacity;
        if (!berth[i].closed) {
            surplus_berth_capacity = berth[i].cargo_capacity;
        }
        else {
            surplus_berth_capacity = 0;
        }
        
        int cargo_on_transporting = 0;
        int cargo_price[robot_num] = { 0 };
        for (int j = 0; j < robot_num; j++) {
            cargo_price[j] = INT_MAX;
        }
        for (int j = 0; j < robot_num; j++) {
            if (robot[j].active && robot[j].target_berth == i) {
                cargo_on_transporting++;
                cargo_price[j] = robot[j].carried_money;
            }
        }
        if (cargo_on_transporting > surplus_berth_capacity) {
            shutdown_berth[i] = true;
            for (int k = 0; k < n * n; k++) {
                if (node_is_berth_new[k] == i) {
                    node_is_berth_new[k] = -1;
                }
            }
            for (int t = 0; t < cargo_on_transporting - surplus_berth_capacity; t++) {
                int min_price_cargo = INT_MAX;
                int rearrange_robot_number = -1;
                for (int j = 0; j < robot_num; j++) {
                    if (cargo_price[j] < min_price_cargo) {
                        min_price_cargo = cargo_price[j];
                        rearranged_robot_number = j;
                    }
                }
                // ���������ֵ��С�Ļ���С��
                bool rearrange_successful = false;
                if (!robot[rearranged_robot_number].carried) {
                    list<Cargo> Cargolist_new;
                    for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                        if (iter->status == 0 && !shutdown_berth[iter->nearestBerthNumber] && !berth[iter->nearestBerthNumber].closed) {
                            Cargolist_new.push_back(*iter);
                        }
                    }
                    if (!Cargolist_new.empty()) {
                        //Astar����
                        int startP_number = robot[rearranged_robot_number].x * n + robot[rearranged_robot_number].y;
                        list<openlist_unit> openlist;
                        list<openlist_unit> openlist_new;
                        int closelist[n * n][3] = { 0 };
                        openlist_unit openP = openlist_unit(startP_number, -1, -1);
                        openlist.push_back(openP);
                        int distance = -1;
                        double max_value_distance_ratio = 0;
                        Cargo* chooseCargo = new Cargo();
                        //int max_scan_distance = 300;
                        bool successful_plan = false;
                        int Node_is_cargo[n * n];
                        for (int n1 = 0; n1 < n * n; n1++) {
                            Node_is_cargo[n1] = -1;
                        }
                        for (auto iter = Cargolist_new.begin(); iter != Cargolist_new.end(); iter++) {
                            Node_is_cargo[iter->point_number] = iter->item_id;
                        }
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
                                if (Node_is_cargo[iter->pointNumber] != -1) {
                                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                        if (iter2->item_id != Node_is_cargo[iter->pointNumber]) {
                                            iter2++;
                                            continue;
                                        }
                                        scanRatio = true;
                                        successful_plan = true;
                                        //����۾��
                                        iter2->value_distance_ratio = (double)iter2->money / (iter2->DistanceToBerth + distance);
                                        if (iter2->value_distance_ratio > max_value_distance_ratio) {
                                            chooseCargo->point_number = iter2->point_number;
                                            chooseCargo->item_id = iter2->item_id;
                                            chooseCargo->path_point = iter2->path_point;
                                            chooseCargo->path_direction = iter2->path_direction;
                                            chooseCargo->nearestBerthNumber = iter2->nearestBerthNumber;
                                            chooseCargo->money = iter2->money;
                                            max_value_distance_ratio = iter2->value_distance_ratio;
                                            iter2 = Cargolist_new.erase(iter2);
                                        }
                                        else {
                                            iter2 = Cargolist_new.erase(iter2);
                                        }
                                    }
                                }
                                if (scanRatio) {
                                    for (auto iter2 = Cargolist_new.begin(); iter2 != Cargolist_new.end();) {
                                        // �޳��޷���ȡ�Ļ���
                                        if (iter2->surplus_zheng - distance < 20) {
                                            iter2 = Cargolist_new.erase(iter2);
                                        }
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
                        //����·��
                        rearranged = true;
                        if (successful_plan) {
                            rearrange_successful = true;
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
                            if (!robot[rearranged_robot_number].path_getting.empty()) {
                                auto iter2 = robot[rearranged_robot_number].path_total.begin();
                                auto iter3 = robot[rearranged_robot_number].path_direction.begin();
                                for (auto iter = robot[rearranged_robot_number].path_getting.begin(); iter != robot[rearranged_robot_number].path_getting.end();) {
                                    iter = robot[rearranged_robot_number].path_getting.erase(iter);
                                    iter2 = robot[rearranged_robot_number].path_total.erase(iter2);
                                    iter3 = robot[rearranged_robot_number].path_direction.erase(iter3);
                                }
                            }
                            if (!robot[rearranged_robot_number].path_pulling.empty()) {
                                auto iter2 = robot[rearranged_robot_number].path_total.begin();
                                auto iter3 = robot[rearranged_robot_number].path_direction.begin();
                                for (auto iter = robot[rearranged_robot_number].path_pulling.begin(); iter != robot[rearranged_robot_number].path_pulling.end();) {
                                    iter = robot[rearranged_robot_number].path_pulling.erase(iter);
                                    iter2 = robot[rearranged_robot_number].path_total.erase(iter2);
                                    iter3 = robot[rearranged_robot_number].path_direction.erase(iter3);
                                }
                            }

                            robots[rearranged_robot_number].path_getting = path_getting_alternative;
                            robots[rearranged_robot_number].path_pulling = path_pulling_alternative;
                            robot[rearranged_robot_number].path_total = path_total_alternative;
                            robot[rearranged_robot_number].path_direction = direction_total_alternative;
                            robot[rearranged_robot_number].path_getting_next.clear();
                            robot[rearranged_robot_number].path_pulling_next.clear();
                            for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
                                if (robot[rearranged_robot_number].choose_item_id == iter2->item_id) {
                                    iter2->status = 0;
                                }
                                for (auto iter = robot[rearranged_robot_number].choose_item_id_next.begin(); iter != robot[rearranged_robot_number].choose_item_id_next.end(); iter++) {
                                    if (*iter == iter2->item_id) {
                                        iter2->status = 0;
                                    }
                                }
                            }
                            robot[rearranged_robot_number].choose_item_id_next.clear();
                            robot[rearranged_robot_number].target_berth = chooseCargo->nearestBerthNumber;
                            robot[rearranged_robot_number].carried_money = chooseCargo->money;
                            robot[rearranged_robot_number].carried = false;
                            //�޸Ļ���״̬
                            int item_id_origin = robot[rearranged_robot_number].choose_item_id;
                            for (auto iter = Cargolist->begin(); iter != Cargolist->end(); iter++) {
                                if (iter->item_id == item_id_origin) {
                                    iter->status = 0;
                                }
                                if (iter->item_id == chooseCargo->item_id) {
                                    iter->status = 1;
                                }
                            }
                            robot[rearranged_robot_number].task_allocated = true;
                            robot[rearranged_robot_number].choose_item_id = chooseCargo->item_id;
                            break;
                        }
                    }
                }
                else {
                    //Astar����
                    int startP_number = robot[rearranged_robot_number].x * n + robot[rearranged_robot_number].y;
                    list<openlist_unit> openlist;
                    list<openlist_unit> openlist_new;
                    int closelist[n * n][3] = { 0 };
                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
                    openlist.push_back(openP);
                    int final_endPnumber;
                    //��鲴λ�Ƿ��Ѿ��رգ����ǣ��򲻽��й滮
                    bool all_closed = true;
                    for (int i = 0; i < berth_num; i++) {
                        if (!berth[i].closed) {
                            all_closed = false;
                            break;
                        }
                    }
                    if (all_closed) {
                    }
                    else {
                        bool CanBeGot = false;
                        while (true) {
                            if (openlist.empty()) {
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
                                if (node_is_berth_new[iter->pointNumber] != -1) {
                                    // ������λ
                                    robot[rearranged_robot_number].target_berth = node_is_berth_new[iter->pointNumber];
                                    CanBeGot = true;
                                    final_endPnumber = iter->pointNumber;
                                    break;
                                }

                            }
                            if (CanBeGot) {
                                break;
                            };
                            openlist = openlist_new;
                            openlist_new.clear();
                        }
                        rearranged = true;
                        //����·��
                        if (CanBeGot) {
                            rearrange_successful = true;
                            list<int> new_path;
                            list<int> new_direction;
                            int last_Pnumber = final_endPnumber;
                            while (closelist[last_Pnumber][1] != -1) {
                                new_path.push_front(last_Pnumber);
                                new_direction.push_front(closelist[last_Pnumber][2]);
                                last_Pnumber = closelist[last_Pnumber][1];
                            }
                            robot[rearranged_robot_number].path_pulling = new_path;
                            robot[rearranged_robot_number].path_direction = new_direction;
                            robot[rearranged_robot_number].path_total = new_path;
                            robot[rearranged_robot_number].path_getting_next.clear();
                            robot[rearranged_robot_number].path_pulling_next.clear();
                            for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
                                for (auto iter = robot[rearranged_robot_number].choose_item_id_next.begin(); iter != robot[rearranged_robot_number].choose_item_id_next.end(); iter++) {
                                    if (*iter == iter2->item_id) {
                                        iter2->status = 0;
                                    }
                                }
                            }
                            robot[rearranged_robot_number].choose_item_id_next.clear();
                        }
                    }
                }
                if (!robot[rearranged_robot_number].carried && !rearrange_successful && !robot[rearranged_robot_number].path_getting.empty())
                {
                    //Astar����
                    int startP_number = robot[rearranged_robot_number].path_getting.back();
                    list<openlist_unit> openlist;
                    list<openlist_unit> openlist_new;
                    int closelist[n * n][3] = { 0 };
                    openlist_unit openP = openlist_unit(startP_number, -1, -1);
                    openlist.push_back(openP);
                    int final_endPnumber;
                    //��鲴λ�Ƿ��Ѿ��رգ����ǣ��򲻽��й滮
                    bool all_closed = true;
                    for (int i = 0; i < berth_num; i++) {
                        if (!berth[i].closed) {
                            all_closed = false;
                            break;
                        }
                    }
                    if (all_closed) {
                    }
                    else {
                        bool CanBeGot = false;
                        while (true) {
                            if (openlist.empty()) {
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
                                if (node_is_berth_new[iter->pointNumber] != -1) {
                                    // ������λ
                                    robot[rearranged_robot_number].target_berth = node_is_berth_new[iter->pointNumber];
                                    CanBeGot = true;
                                    final_endPnumber = iter->pointNumber;
                                    break;
                                }

                            }
                            if (CanBeGot) {
                                break;
                            };
                            openlist = openlist_new;
                            openlist_new.clear();
                        }
                        //����·��
                        rearranged = true;
                        if (CanBeGot) {
                            rearrange_successful = true;
                            list<int> new_path;
                            list<int> new_direction;
                            int last_Pnumber = final_endPnumber;
                            while (closelist[last_Pnumber][1] != -1) {
                                new_path.push_front(last_Pnumber);
                                new_direction.push_front(closelist[last_Pnumber][2]);
                                last_Pnumber = closelist[last_Pnumber][1];
                            }
                            robot[rearranged_robot_number].path_pulling = new_path;
                            list<int> direction_getting;
                            auto iter2 = robot[rearranged_robot_number].path_direction.begin();
                            for (auto iter = robot[rearranged_robot_number].path_getting.begin(); iter != robot[rearranged_robot_number].path_getting.end(); iter++) {
                                direction_getting.push_back(*iter2);
                                iter2++;
                            }
                            robot[rearranged_robot_number].path_direction = merge_in_order(direction_getting, new_direction);
                            robot[rearranged_robot_number].path_total = merge_in_order(robot[rearranged_robot_number].path_getting, new_path);
                            robot[rearranged_robot_number].path_getting_next.clear();
                            robot[rearranged_robot_number].path_pulling_next.clear();
                            for (auto iter2 = Cargolist->begin(); iter2 != Cargolist->end(); iter2++) {
                                for (auto iter = robot[rearranged_robot_number].choose_item_id_next.begin(); iter != robot[rearranged_robot_number].choose_item_id_next.end(); iter++) {
                                    if (*iter == iter2->item_id) {
                                        iter2->status = 0;
                                    }
                                }
                            }
                            robot[rearranged_robot_number].choose_item_id_next.clear();
                        }
                    }
                }
            }
            berth[i].closed = true;
        }
    }


    // Step4: �Ż�Ŀǰ��������Ļ�����

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
    //            //Astar����
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
    //                        // �޳��޷���ȡ�Ļ���
    //                        if (iter2->surplus_zheng - distance < 10) {
    //                            iter2 = Cargolist_new.erase(iter2);
    //                        }
    //                        // ����Ƿ����ȡ������
    //                        else if (iter->pointNumber == iter2->point_number) {
    //                            //����۾��
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
    //            //����·��
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

    //                //�޸Ļ���״̬
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
* ��������滮����
*/
void BoatPlan(Boat boats[], Berth berths[], Robot robots[], int zhen_now)
{
    for (int i = 0; i < boat_num; i++) {
        if (zhen_now < boat[i].surplus_time) {
            continue;
        }
        //Step 1 ��ǰ֡��������������ƶ�����λ
        if (boat[i].status == 1 && boat[i].pos == -1) {
            boat[i].cargo_number = 0;
            if (!boat[i].berths_to_go.empty() && boat[i].berths_to_go.size() > 1) {
                // �����һ�غ�
                auto target_berth_iter = boat[i].berths_to_go.begin();
                boat[i].need_to_ship = true;
                boat[i].shipping_target = *target_berth_iter;
                boat[i].berths_to_go.erase(target_berth_iter);
            }
            else
            {
                // ���һ�غϵȵ�ʱ���ٿ�ʼ����
                if (zhen_now >= boat[i].final_trip_start_time) {
                    auto target_berth_iter = boat[i].berths_to_go.begin();
                    boat[i].need_to_ship = true;
                    boat[i].shipping_target = *target_berth_iter;
                    boat[i].berths_to_go.erase(target_berth_iter);
                }
            }
        }
        //Step 2 ��ǰ֡�������ƶ�����λ����ʼװ��������ͣ��ʱ��
        if (boat[i].status != 0 && boat[i].pos != -1 && !boat[i].at_berth) {
            boat[i].at_berth = true;
            int target_berth = boat[i].shipping_target;
            boat[i].shipping_target = -1;
            boat[i].now_berth = target_berth;
            boat[i].remaining_time = boat[i].capacity / berth[target_berth].loading_speed + 1;
            // ���
            if (boat[i].remaining_time < -2) {
                int a = 1;
            }
        }

        //Step 3 ��ǰ֡����������װ��������ͣ��ʱ��
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
        //Step 4 ��ǰ֡������װ�ػ����������ʼ����
        if (boat[i].at_berth && boat[i].remaining_time == 0) {
            boat[i].at_berth = false;
            boat[i].need_to_go = true;
            // ����capacity
            int leaving_berth = boat[i].now_berth;
            int now_capacity = 0;
            for (int j = 0; j < boat_num; j++) {
                if (boat[j].shipping_target == leaving_berth) {
                    now_capacity = now_capacity + boat[j].capacity;
                }
                if (!boat[j].berths_to_go.empty()) {
                    for (auto iter = boat[j].berths_to_go.begin(); iter != boat[j].berths_to_go.end(); iter++) {
                        if (*iter == leaving_berth) {
                            now_capacity = now_capacity + boat[j].capacity;
                        }
                    }
                }
            }
            if (now_capacity < berth[leaving_berth].cargo_capacity) {
                berth[leaving_berth].cargo_capacity = now_capacity;
            }
            // ����Ƿ���Ҫ�رոۿ�
            
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
* �����ض�����
*/
void cargo_reFind_berth(list<Cargo> Cargolist, int Node_movable_directions[][4], int Node_movable_node[][4], int Node_is_berth[],bool *cargo_added)
{
    for (int t = 0; t < 1; t++) {
        for (auto iter = Cargolist.begin(); iter != Cargolist.end(); iter++) {
            if (berth[iter->nearestBerthNumber].closed && iter->status == 0) {

                Cargo cargo = Cargo(iter->x, iter->y, iter->item_id, iter->item_id, iter->surplus_zheng, 0);
                int StartP = iter->x * n + iter->y;
                Cargolist.erase(iter);
                if (Node_movable_node[StartP][0] + Node_movable_node[StartP][1] + Node_movable_node[StartP][2] + Node_movable_node[StartP][3] != -4) {
                    *cargo_added = true;
                    precalc_CargoToBerth(&cargo, berth, Node_movable_node, Node_is_berth, &cargo.CanBeGot, &cargo.nearestBerthNumber, &cargo.DistanceToBerth);
                    if (cargo.CanBeGot) {
                        Cargolist.push_back(cargo);
                    }
                }
                break;
            }
        }
    }
}

//��ʼ������     ��ȡ��Ԥ�����ͼ��Ϣ��
FILE* outputFile;
FILE* money_statistic;
void Init(int Node_movable_directions[][4], int Node_movable_node[][4], int Berth_information[][5], int Node_is_berth[], int* Boat_capacity, Robot robot_init[])
{
    // ��ȡ��ͼ��Ϣ���洢��Node_movable_directions��
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
                // �ټ�¼��ʼ�����˵�λ����Ϣ
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
    // ��ȡ��λ��Ϣ���洢��Berth_information��    
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
    * �������˿��Եִ�����в�λ
    * ����û����˲��ɵִ��κβ�λ����û������޷�ִ�������ж���Ϊ��Ч�����ˡ�
    */
    scanBerthsCanBeReached(robot_init, berth, Node_movable_node);
    for (int i = 0; i < robot_num; i++) {
        for (int j = 0; j < berth_num; j++) {
            if (robot_init[i].berthsCanBeReached[j]) {
                robot_init[i].active = true;
            }
        }
    }

    // ��¼��������
    scanf("%d", &boat_capacity);
    fprintf(outputFile, "%d\n", boat_capacity);
    *Boat_capacity = boat_capacity;
    // �����ִ������в�λ��������һ��������ܺ�ʱ
    for (int i = 0; i < berth_num; i++) {
        berth[i].round_trip_transport_time = berth[i].transport_time * 2 + (boat_capacity / berth[i].loading_speed + 1) + 2; // Ԥ����֡�ݴ�
    }

    char okk[100];
    scanf("%s", okk);
    fprintf(outputFile, "%s\n", okk);
    //////////////////////////////////////////////// �������////////////////////////////////////////////////////////////////////////////////
    /*
    * �ִ��Ű࿪ʼ
    */
    for (int i = 0; i < boat_num; i++) {
        boat[i].capacity = boat_capacity;
    }

    /*
    * �������ʵ�飺
    * ������� rand_cargo_numbe �������ڿɵִ�Ľڵ��ϣ�������Щ���ﵽ���в�λ�ľ��롣����޷��ִ��400���㡣
    * ͳ�Ƶִ����в�λ��ʱ�����
    * ͳ�Ʊ�ѡ��Ϊ��һ��λ�Ĵ���
    * �����Ż��ִ����Ű�
    */
    const int rand_cargo_number = 100;

    //ִ����ɴλ���ʵ��
    int berth_be_choosen_times[berth_num] = { 0 };
    int distance_to_berth_test[berth_num][rand_cargo_number];
    for (int i = 0; i < berth_num; i++) {
        for (int j = 0; j < rand_cargo_number; j++) {
            distance_to_berth_test[i][j] = n * 2;
        }
    }
    //srand((unsigned)time(NULL));
    for (int num = 0; num < rand_cargo_number; num++) {
        while (true) {
            int point_number = rand() % (n * n);
            if (Node_movable_node[point_number][0] + Node_movable_node[point_number][1] + Node_movable_node[point_number][2] + Node_movable_node[point_number][3] != -4) {
                bool first_reached = false;
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
                                if (!first_reached) {
                                    first_reached = true;
                                    berth_be_choosen_times[i]++;
                                }
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
        for (int j = 0; j < rand_cargo_number; j++) {
            test_score_berths[i] = test_score_berths[i] + distance_to_berth_test[i][j];
        }
    }
    for (int i = 0; i < berth_num; i++) {
        test_score_berths[i] = test_score_berths[i] / rand_cargo_number;
        test_score_berths[i] = test_score_berths[i] + berth[i].transport_time;
    }


    // ���/////////////////////////////////////////////////////////////////////////////
    //int berth_be_choosen_times[berth_num] = { 32, 20, 25, 29, 15, 13, 14, 14, 18, 20 };
    //double test_score_berths[berth_num] = { 922.62, 1202.44, 1105.23, 917.65, 1204.21, 1085.7, 1239.87, 918.28, 1170.54, 1345.39 };

    //ð������
    int IDX_test_score_berths[berth_num] = { 0 };
    for (int i = 0; i < berth_num; i++) {
        IDX_test_score_berths[i] = i;
    }
    for (int i = 0; i < berth_num; i++)//���ѭ���ǱȽϵ���������������10��������ô��Ӧ�ñȽ�10-1=9��
    {
        int temp;
        int IDX_temp;
        for (int j = 0; j < berth_num - i - 1; j++)//�ڲ�ѭ���Ƚϵ��ǵ�ǰһ�ֵıȽϴ��������磺��һ�ֱȽ�9-1=8�Σ��ڶ��ֱȽ�9-2=7��
        {
            if (test_score_berths[j] > test_score_berths[j + 1])//������������������򽻻�λ��
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

    // ͳ�Ƶ�ͼ�ϵġ���������
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

    //������ÿ���������һ�����մ�ֻ
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
                    //ѡ�����ղ�λ
                    boat[i].final_berth = choose_berth;
                    boat[i].berths_to_go.push_front(choose_berth);
                    //����ʣ��ɷ���ʱ��
                    boat[i].final_trip_start_time = total_zhen - berth[choose_berth].round_trip_transport_time - 50;//Ԥ��50֡ʱ���ݴ����ֹ��֡Ӱ��
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
                //ѡ�����ղ�λ
                boat[i].final_berth = choose_berth;
                boat[i].berths_to_go.push_front(choose_berth);
                //����ʣ��ɷ���ʱ��
                boat[i].final_trip_start_time = total_zhen - berth[choose_berth].round_trip_transport_time - 50;//Ԥ��50֡ʱ���ݴ����ֹ��֡Ӱ��
                berths_choosed[choose_berth] = true;
                break;
            }
        }
    }

    // ��ʣ��ɷ���ʱ������Ű�

    // Step1 ���Լ����ܹ����Ż������ܰ��
    int mean_round_trip_time = 0;
    for (int i = 0; i < berth_num; i++) {
        mean_round_trip_time = mean_round_trip_time + berth[i].round_trip_transport_time;
    }
    mean_round_trip_time = mean_round_trip_time / berth_num;

    int estimated_rounds = boat_num;
    for (int i = 0; i < boat_num; i++) {
        estimated_rounds = estimated_rounds + (boat[i].final_trip_start_time - 1000) / mean_round_trip_time;
    }

    double estimated_rounds_allocate_to_berths[berth_num] = { 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5 };;
    for (int i = 0; i < berth_num; i++) {
        estimated_rounds_allocate_to_berths[i] = (double)estimated_rounds / rand_cargo_number * berth_be_choosen_times[i];
    }
    


    // Step2 ���в����ò�λ
    //for (int i = 0; i < berth_num; i++) {
    //    if (estimated_rounds_allocate_to_berths[i] < 0.5) {
    //        berth[i].closed = true;
    //    }
    //}

    // Step3 Ϊÿ����ֻ�ƶ��Ű෽��
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

    //Step 4 ����λͳ�ƻ��ɰ��˵Ļ���
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

int Input(list<Cargo>* Cargolist, int Node_movable_node[][4], int Node_is_berth[], int* item_id, bool* cargo_added, Robot robotsTest[])
{
    scanf("%d%d", &id, &money);
    fprintf(outputFile, "%d %d\n", id, money);
    int money1 = 0;
    for (int i = 0; i < berth_num; i++) {
        money1 = money1 + berth[i].money_total;
    }
    fprintf(money_statistic, "%d\n", money1);

    // ��鲴λ״̬���رջ�����еĲ�λ
    for (int i = 0; i < berth_num; i++) {
        if (berth[i].cargo_capacity <= 0) {
            berth[i].closed = true;
        }
        if (berth[i].closed){
            for (int j = 0; j < n * n; j++) {
                if (Node_is_berth[j] == i) {
                    Node_is_berth[j] = -1;
                }
            }
        }
    }


    // ��������������Ϣ����������cargo��չ��Cargolist�С�
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
        int StartP = x * n + y;
        if (Node_movable_node[StartP][0] + Node_movable_node[StartP][1] + Node_movable_node[StartP][2] + Node_movable_node[StartP][3] != -4) {
            precalc_CargoToBerth(&cargo, berth, Node_movable_node, Node_is_berth, &cargo.CanBeGot, &cargo.nearestBerthNumber, &cargo.DistanceToBerth);
            if (cargo.CanBeGot) {
                Cargolist->push_back(cargo);
            }
        }
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
    /////////////////////////////////////////////////���г�ʼ��///////////////////////////////////////////////////////
    /*
    *  �����ڵ�ͨ�����һ���ڵ�һ��,һ��40000�У������Ͻǿ�ʼ��������˳���ţ����м�����š�
    *  ÿһ��Ϊ4��0-1�������ֱ��ʾ�Ƿ��ܹ�����/��/��/���ƶ�
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
    *  ������λ��Ϣ���һ����λһ��,һ��10�У�������ֵ��š�
    *  ÿһ��Ϊ5��������[��λ���, ��λ����x, y, ����ʱ��, װ��ʱ��]
    */
    int Berth_information[berth_num][5];
    for (int i = 0; i < berth_num; i++) {
        for (int j = 0; j < 5; j++) {
            Berth_information[i][j] = 0;
        }
    }

    /*
    *  ����װ����
    */
    int Boat_capacity;

    /*
    * ����һ����ʼ�Ļ�����Ⱥ���Ա��ڿ�ʼ����ǰ��ȷ���������Ƿ�������С�
    */
    Robot Robot_init[robot_num];

    /*
    * ��ʼ��
    */
    Init(Node_movable_directions, Node_movable_node, Berth_information, Node_is_berth, &Boat_capacity, Robot_init);

    /*
    * ����������Ϣ��
    */
    auto Cargolist = new list<Cargo>;
    int item_id = 0;

    /*
    * �㷨������
    */
    int start_rearrange_robot_number = 0;
    int limit_time = 40; //ʣ��·������limit_time�Ļ����˽��滮δ��·��
    int scan_range = 15; // ���·����ͻ�ķ�Χ;
    int max_scan_distance = 250;

    /////////////////////////////////////////////////��ʼ��֡����///////////////////////////////////////////////////////
    for (int zhen = 1; zhen <= 15000; zhen++)
    {
        // ����
        if (zhen == 9820) {
            int a = 1;
        }
        if (zhen == 1878) {
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
            if (berth[i].cargo_capacity < 0) {
                int a = 1;
            }
        }

        bool cargo_added = false;
        /*
        * ��������Ļ���
        */
        int id = Input(Cargolist, Node_movable_node, Node_is_berth, &item_id, &cargo_added, robotTest);  //ÿһ֡������




        // ����
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
        * ���֣��ҵ���Ч�����˲�ȡ����·���滮
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
        * ���»�����л����ʣ��ʱ��(surplus_zheng -= 1)
        * ɾ�������˵Ļ���(status = 2)�Լ������������ʱ��Ļ���(surplus_zheng = 0)
        */

        if (!Cargolist->empty())
        {
            for (auto i = Cargolist->begin(); i != Cargolist->end(); i++) {
                i->surplus_zheng -= 1;
            }
            auto ite = Cargolist->begin();
            while (ite != Cargolist->end()) {
                if (ite->surplus_zheng == 0) {
                    //����
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
        * ���ݵ�ǰ״̬��ִ��һ�ΰ�������Ĺ滮
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
        * ����������Ż�
        */
        //if (!task_arranged){
        if (start_rearrange_robot_number == robot_num) {
            start_rearrange_robot_number = 0;
        }
        else {
            start_rearrange_robot_number++;
        }
        //if (!cargo_added && cargo_unchoosen) {
            optimize_task(robot, start_rearrange_robot_number, Cargolist, Node_movable_node, &task_arranged, Node_is_berth, max_scan_distance);
        //}
        // }
        /*
        * ����Ƿ����л���������Ч��λ
        */
        if (!cargo_added) {
            cargo_reFind_berth(*Cargolist, Node_movable_directions, Node_movable_node, Node_is_berth, &cargo_added);
        }
        /*
        * ��·����������Ļ�����Ԥ�ȹ滮����·��
        */
        //if (!cargo_added){
        path_Preplan(robot, limit_time, Cargolist, Node_movable_node, &task_arranged, max_scan_distance);
        //}

        /*
        * ���·����ͻ
        */


        SolveDeadlock(robot, scan_range, Node_movable_node);

        /*
        * �Դ洢��·�������ʽ�����
        */

        /*
        * �滮�ִ�
        */
        BoatPlan(boat, berth, robot, zhen);
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

        // ִ�д���ָ��
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