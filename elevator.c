#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <limits.h>
#include <math.h>

#define QUIT 'Q'
#define PAUSE 'W'
#define RESUME 'E'
#define RESTART 'R'
#define CALL 'A'
#define FLOOR 20
#define NUM_ELEVATORS 6
#define MAX_PEOPLE 15 // 엘리베이터 정원
#define MAX_TOTAL 150 // 점검 받아야하는 수

/* 요청 구조체 */
typedef struct _REQUEST
{
    int start_floor; //현재층(요청이 이루어지는)
    int dest_floor;  //목적층
    int num_people;  //몇 명이 타는지
} Request;

typedef struct _FLOORNODE
{
    struct _FLOORNODE *prev;
    struct _FLOORNODE *next;
    int floor;  // -1 : 점검
    int people; //+ 태운다, - 내린다
} F_node;

typedef struct _REQUESTNODE
{
    struct _REQUESTNODE *prev;
    struct _REQUESTNODE *next;
    Request req;
} R_node;

typedef struct _FLOORLIST
{
    F_node *head;
    F_node *tail;
} F_list;

typedef struct _REQUESTLIST
{
    R_node *head;
    R_node *tail;
} R_list;

/* 엘리베이터 구조체 */
typedef struct _ELEVATOR
{
    int current_floor;
    int next_dest;
    int current_people;
    int total_people;
    int fix;
    int fix_time;
    F_list pending;
} Elevator;

typedef struct _INPUT
{
    char *mode;
    int *req_current_floor;
    int *req_dest_floor;
    int *req_num_people;
} Input;

typedef struct _SIMUL
{
    Elevator **elevators;
    Input *input;
} Simul;

/* 함수 헤더 */
int getch(void);
void init(Input **input, Simul **simul, Elevator *elevators[6]);
void *input_f(void *data);
void *simul_f(void *data);
void print_UI(Elevator *elevators[6]);
void print_elevator_info(Elevator *elevators[6]);
void print_menu(char mode, Input *input);
void quit(Elevator *elevators[6]);
void simul_stop(char *mode);
void simul_restart(Simul *simul);
void get_request(Input *input);
void insert_into_queue(int current_floor, int dest_floor, int num_people);
Elevator *find_elevator(Elevator *elevators[6], Request *current);
F_node *find_ideal_location(Elevator *elevator, int start_floor, int dest_floor, int target);
int find_time(F_list list, F_node *target, int start, int end);
int find_min(int *arr, int n);
void move_elevator(Elevator *elevators[6]);
void fix_elevator(Elevator *elevator);
void R_list_insert(R_list list, int current_floor, int dest_floor, int num_people);
int R_list_size(R_list list);
Request *R_list_remove(R_list list);
void F_list_insert(F_list list, F_node *after, int floor, int people);
int F_list_size(F_list list);
void F_list_remove(F_list list);
F_node *F_list_peek(F_list list);
void print_F_list(F_list list);

/* 전역 변수 */
R_list reqs;
int flag = 0;

int main(void)
{
    Input *input;
    Simul *simul;
    Elevator *elevators[6];
    pthread_t input_thr;
    pthread_t simul_thr;
    int tid_input;
    int tid_simul;

    init(&input, &simul, elevators);

    //test
    flag = 1;
    insert_into_queue(1, 5, 3);
    /*
    flag = 1;
    insert_into_queue(11, 15, 5);
    flag = 1;
    insert_into_queue(5, 10, 4);
    flag = 1;
    insert_into_queue(9, 2, 7);
    flag = 1;
    insert_into_queue(5, 1, 8);
    flag = 1;
    insert_into_queue(3, 18, 2);
    flag = 1;
    insert_into_queue(17, 6, 6);
    flag = 1;
    insert_into_queue(9, 12, 4);
    */
    //test end

    tid_input = pthread_create(&input_thr, NULL, input_f, (void *)input);
    if (tid_input != 0)
    {
        perror("thread creation error: ");
        exit(0);
    }

    tid_simul = pthread_create(&simul_thr, NULL, simul_f, (void *)simul);
    if (tid_simul != 0)
    {
        perror("thread creation error: ");
        exit(0);
    }

    pthread_join(input_thr, NULL);
    pthread_join(simul_thr, NULL);

    return 0;
}

void init(Input **input, Simul **simul, Elevator *elevators[6])
{
    int i, j;
    reqs.head = (R_node *)malloc(sizeof(R_node));
    reqs.tail = (R_node *)malloc(sizeof(R_node));
    reqs.head->prev = NULL;
    reqs.head->next = reqs.tail;
    reqs.tail->prev = reqs.head;
    reqs.tail->next = NULL;

    *input = (Input *)malloc(sizeof(Input));
    (*input)->mode = (char *)malloc(sizeof(char));
    (*input)->req_current_floor = (int *)malloc(sizeof(int));
    (*input)->req_dest_floor = (int *)malloc(sizeof(int));
    (*input)->req_num_people = (int *)malloc(sizeof(int));

    *simul = (Simul *)malloc(sizeof(Simul));
    (*simul)->input = *input;

    for (i = 0; i < NUM_ELEVATORS; i++)
    {
        elevators[i] = (Elevator *)malloc(sizeof(Elevator));
    }

    //엘리베이터 : 1, 2 - 저층, 3, 4 - 전층, 5, 6 - 고층
    for (i = 0; i < NUM_ELEVATORS; i++)
    {
        elevators[i]->pending.head = (F_node *)malloc(sizeof(F_node));
        elevators[i]->pending.tail = (F_node *)malloc(sizeof(F_node));
        elevators[i]->pending.head->floor = 0;
        elevators[i]->pending.tail->floor = 0;
        elevators[i]->pending.head->prev = NULL;
        elevators[i]->pending.head->next = elevators[i]->pending.tail;
        elevators[i]->pending.tail->prev = elevators[i]->pending.head;
        elevators[i]->pending.tail->next = NULL;

        elevators[i]->current_floor = 1;
        elevators[i]->next_dest = 1;
        elevators[i]->current_people = 0;
        elevators[i]->total_people = 0;
        elevators[i]->fix = 0;
        elevators[i]->fix_time = 0;
    }

    // 고층 엘리베이터는 처음 11층에 멈춰있음
    elevators[4]->current_floor = 11;
    elevators[4]->next_dest = 11;
    elevators[5]->current_floor = 11;
    elevators[5]->next_dest = 11;

    (*simul)->elevators = elevators;
}

void *input_f(void *data)
{
    Input *input = (Input *)data;
    while (1)
    {
        if (*input->mode == QUIT)
        {
            break;
        }
        else if (*input->mode == CALL)
        {
            get_request(input);
        }
        else
        {
            scanf(" %c", input->mode);
        }
    }
}

void *simul_f(void *data)
{
    int i;
    Simul *simul = (Simul *)data;
    Elevator *response; // 요청에 응답하는 엘리베이터
    F_node *location;   // 요청이 들어가는 위치
    Request current;    // 처리할 요청

    // 1. 화면을 출력한다.
    // 2. 특수 모드가 입력되면 실행한다
    // 3. 엘리베이터 호출이 들어오면 호출에 응한다.
    // 3-1. 응답할 엘리베이터를 선택한다.
    // 3-2. 응답할 엘리베이터에 요청을 넣는다.
    // 4. 엘리베이터를 이동시킨다.

    /* 반복문 1회 반복시 1초 소요 */
    while (1)
    {
        system("clear");
        print_UI(simul->elevators);
        printf("\n");
        print_elevator_info(simul->elevators);
        printf("\n");
        print_menu(*simul->input->mode, simul->input);

        // 특수 모드 실행
        if (*simul->input->mode == QUIT)
        {
            quit(simul->elevators);
        }
        else if (*simul->input->mode == PAUSE)
        {
            simul_stop(simul->input->mode);
        }
        else if (*simul->input->mode == RESTART)
        {
            simul_restart(simul);
            continue;
        }

        // 요청 큐에 추가 & 건물 정보 업데이트
        insert_into_queue(*simul->input->req_current_floor, *simul->input->req_dest_floor, *simul->input->req_num_people);

        //점검 필요한 엘리베이터 있으면 점검 요청 넣기(맨 마지막에)
        for (i = 0; i < NUM_ELEVATORS; i++)
        {
            if (simul->elevators[i]->total_people >= MAX_TOTAL)
            {
                F_list_insert(simul->elevators[i]->pending, simul->elevators[i]->pending.tail, -1, 0);
                simul->elevators[i]->total_people = 0;
            }
        }

        if (R_list_size(reqs) != 0)
        {
            current = *R_list_remove(reqs);
            response = find_elevator(simul->elevators, &current);

            // 요청에 응답하는 엘리베이터에 정보 추가하기

            // 사람 태울 층 추가하기
            location = find_ideal_location(response, current.start_floor, current.dest_floor, current.start_floor);
            F_list_insert(response->pending, location, current.start_floor, current.num_people);

            // 사람 내릴 층 추가하기
            location = find_ideal_location(response, current.start_floor, current.dest_floor, current.dest_floor);
            F_list_insert(response->pending, location, current.dest_floor, current.num_people * -1);
        }

        // 엘리베이터 이동시키기
        move_elevator(simul->elevators);

        sleep(1);
    }
}

void print_UI(Elevator *elevators[6])
{
    int i, j;
    for (i = 0; i < FLOOR; i++)
    {
        printf("    ------------------------------------------------------------------------- \n"); //윗칸
        printf("%2dF ", FLOOR - i);
        for (j = 0; j < NUM_ELEVATORS; j++)
        {
            if (elevators[j]->current_floor == FLOOR - i)
            {
                printf("|");
                if (elevators[j]->fix)
                {
                    printf(" 수리중");
                }
                else if (elevators[j]->current_floor == elevators[j]->next_dest)
                {
                    printf(" 대기중");
                }
                else if (elevators[j]->current_floor > elevators[j]->next_dest)
                {
                    printf(" ▼");
                    printf(" %2dF ", elevators[j]->next_dest);
                }
                else
                {
                    printf("▲ ");
                    printf(" %2dF ", elevators[j]->next_dest);
                }
                printf("%2d명", elevators[j]->current_people);
            }
            else
            {
                printf("|           ");
            }
        }
        printf("| ");

        printf("\n");
    }
    printf("    ------------------------------------------------------------------------- \n"); //아랫칸
    printf("       저층용 1    저층용 2    전층용 1    전층용 2    고층용 1    고층용 2 \n");
}

void print_elevator_info(Elevator *elevators[6])
{
    int i;
    R_node *curr = reqs.head->next;

    printf("현재 요청: ");
    while (curr != reqs.tail)
    {
        printf("%d층 -> %d층 %d명, ", curr->req.start_floor, curr->req.dest_floor, curr->req.num_people);
        curr = curr->next;
    }

    printf("\n");

    for (i = 0; i < NUM_ELEVATORS; i++)
    {
        printf("엘리베이터 %d | ", i + 1);
        if (elevators[i]->fix)
        {
            printf("수리 중 \n");
            continue;
        }
        else if (elevators[i]->current_floor == elevators[i]->next_dest)
        {
            printf("대기 중 | ");
        }
        else
        {
            printf("운행 중 | ");
        }
        printf("%2d명 탑승 중 | ", elevators[i]->current_people);
        printf("총 %3d명 탑승 | ", elevators[i]->total_people);
        printf("대기 요청 : ");
        print_F_list(elevators[i]->pending);
        printf("\n");
    }
}

void print_menu(char mode, Input *input)
{
    if (mode == CALL)
    {
        printf("\n엘리베이터 호출 모드\n");
        printf("현재 층 : %d, 목적 층 : %d, 사람 수 : %d\n", *input->req_current_floor, *input->req_dest_floor, *input->req_num_people);
        printf("h : 현재 층 증가, j : 목적 층 증가, k : 사람 수 증가 \n");
        printf("입력을 완료하려면 l를 누르십시오 \n");
    }
    else
    {
        printf("Q : 종료\t");
        printf("W : 정지\t");
        printf("E : 재개\t");
        printf("R : 재시작\t");
        printf("A : 호출\n");
        printf("메뉴 선택 : ");
    }
    fflush(stdout);
}

// 모든 메모리 해제하는 수정 필요 - pthread_exit 활용해서 정리함수 구현 필요
void quit(Elevator *elevators[6])
{
    int i;

    printf("\n엘리베이터 시뮬레이션 시스템을 종료합니다. \n");
    for (i = NUM_ELEVATORS - 1; i >= 0; i--)
    {
        free(elevators[i]->pending.tail);
        free(elevators[i]->pending.head);
    }

    for (i = NUM_ELEVATORS - 1; i >= 0; i--)
    {
        free(elevators[i]);
    }

    //TODO modification
    exit(0);
}

//pthread_mutex 를 이용해서 좀 더 깨끗하게??
void simul_stop(char *mode)
{
    while (*mode != RESUME && *mode != QUIT && *mode != RESTART)
    {
    }
}

void simul_restart(Simul *simul)
{
    int i;
    for (i = 0; i < NUM_ELEVATORS; i++)
    {
        simul->elevators[i]->current_floor = 1;
        simul->elevators[i]->next_dest = 1;
        simul->elevators[i]->current_people = 0;
        simul->elevators[i]->total_people = 0;
        simul->elevators[i]->fix = 0;
    }

    // 고층 엘리베이터는 시작 층이 11층
    simul->elevators[4]->current_floor = 11;
    simul->elevators[4]->next_dest = 11;
    simul->elevators[5]->current_floor = 11;
    simul->elevators[5]->next_dest = 11;

    *simul->input->mode = 0;

    //요청 목록 초기화
    //빌딩 정보 초기화
}

// scanf 를 이용해서 구현 불가능?
void get_request(Input *input)
{
    char key;

    *input->req_current_floor = 1;
    *input->req_dest_floor = 1;
    *input->req_num_people = 1;

    while (1)
    {
        key = getch();
        if (key == 'l')
        {
            *input->mode = 0;
            flag = 1;
            break;
        }
        if (key == 'h')
        {
            (*input->req_current_floor)++;
            if (*input->req_current_floor > FLOOR)
            {
                *input->req_current_floor = 1;
            }
        }
        if (key == 'j')
        {
            (*input->req_dest_floor)++;
            if (*input->req_dest_floor > FLOOR)
            {
                *input->req_dest_floor = 1;
            }
        }
        if (key == 'k')
        {
            (*input->req_num_people)++;
            if (*input->req_num_people > MAX_PEOPLE)
            {
                *input->req_num_people = 1;
            }
        }
    }

    flag = 1;
}

void insert_into_queue(int current_floor, int dest_floor, int num_people)
{
    //예외처리: flag = 입력을 완료했는가(scanf로 입력 구현하면 불필요)
    if (flag == 0)
    {
        return;
    }

    if (current_floor == dest_floor)
    {
        return;
    }

    if (current_floor > FLOOR || current_floor < 1 || dest_floor > FLOOR || dest_floor < 1)
    {
        return;
    }

    R_list_insert(reqs, current_floor, dest_floor, num_people);

    flag = 0;
}

Elevator *find_elevator(Elevator *elevators[6], Request *current)
{
    F_node **ideal;
    int *time_required;
    int ideal_index;
    int i;
    int size;
    int s = 0;

    // 1. 큐에 요청을 뺀다
    // 2. 각각에 가상의 스케쥴링을 실행한다
    // 2-1. 점검 요청이 들어와 있으면 소요시간을 최대로 한다
    // 3. 각각의 소요시간을 구한다
    // 4. 최소 시간 걸리는 엘리베이터 리턴

    if ((current->start_floor > 10 && current->dest_floor <= 10) || (current->start_floor <= 10 && current->dest_floor > 10))
    {
        s = 2;
        size = 2;
        ideal = (F_node **)malloc(sizeof(F_node *) * size);
        time_required = (int *)malloc(sizeof(int) * size);
        for (i = 0; i < 2; i++)
        {
            if (elevators[i + s]->pending.tail->prev->floor == -1 || elevators[i + s]->fix == 1)
            {
                time_required[i] = INT_MAX;
                continue;
            }
            ideal[i] = find_ideal_location(elevators[i + s], current->start_floor, current->dest_floor, current->start_floor);
            time_required[i] = find_time(elevators[i + s]->pending, ideal[i], elevators[i + s]->current_floor, current->start_floor);
            printf("%d번째 엘리베이터 소요시간: %d초 \n", i + s + 1, time_required[i]);
        }
    }
    else if (current->start_floor > 10 || current->dest_floor > 10)
    {
        s = 2;
        size = 4;
        ideal = (F_node **)malloc(sizeof(F_node *) * size);
        time_required = (int *)malloc(sizeof(int) * size);
        for (i = 0; i < 4; i++)
        {
            if (elevators[i + s]->pending.tail->prev->floor == -1 || elevators[i + s]->fix == 1)
            {
                time_required[i] = INT_MAX;
                continue;
            }
            ideal[i] = find_ideal_location(elevators[i + s], current->start_floor, current->dest_floor, current->start_floor);
            time_required[i] = find_time(elevators[i + s]->pending, ideal[i], elevators[i + s]->current_floor, current->start_floor);
            printf("%d번째 엘리베이터 소요시간: %d초 \n", i + s + 1, time_required[i]);
        }
    }
    else
    {
        size = 4;
        ideal = (F_node **)malloc(sizeof(F_node *) * size);
        time_required = (int *)malloc(sizeof(int) * size);
        for (i = 0; i < 4; i++)
        {
            if (elevators[i + s]->pending.tail->prev->floor == -1 || elevators[i + s]->fix == 1)
            {
                time_required[i] = INT_MAX;
                continue;
            }
            ideal[i] = find_ideal_location(elevators[i + s], current->start_floor, current->dest_floor, current->start_floor);
            time_required[i] = find_time(elevators[i + s]->pending, ideal[i], elevators[i + s]->current_floor, current->start_floor);
            printf("%d번째 엘리베이터 소요시간: %d초 \n", i + s + 1, time_required[i]);
        }
    }

    ideal_index = find_min(time_required, size);
    free(time_required);
    free(ideal);
    return elevators[ideal_index + s];
}

F_node *find_scheduled_place(F_node *start, F_node *end, int start_floor, int dest_floor, int target)
{
    F_node *current = start;
    int call_direction = dest_floor - start_floor;
    if (call_direction > 0)
    {
        while (1)
        {
            if (target <= current->floor)
            {
                return current;
            }
            current = current->next;
            if (current == end->next)
            {
                return current;
            }
        }
    }
    else
    {
        while (1)
        {
            if (target >= current->floor)
            {
                return current;
            }
            current = current->next;
            if (current == end->next)
            {
                return current;
            }
        }
    }
}

F_node *find_direction_change_location(F_node *current, int current_direction)
{
    F_node *target = current;
    if (current->next == NULL)
    {
        return current;
    }

    int new_direction;
    while (1)
    {
        target = target->next;
        if (target->next == NULL)
        {
            return target;
        }
        new_direction = target->floor - current->floor;
        if (new_direction * current_direction < 0)
        {
            return target;
        }
    }
}

// 예외처리 고려 필요
F_node *find_ideal_location(Elevator *elevator, int start_floor, int dest_floor, int target)
{
    int elevator_direction = 0;
    int call_direction = 0;
    F_list list = elevator->pending;
    F_node *start = list.head->next;
    F_node *end;

    if (F_list_size(list) == 0)
    {
        return start;
    }

    if (F_list_size(list) == 1)
    {
        return start->next;
    }

    elevator_direction = start->floor - elevator->current_floor;
    call_direction = dest_floor - start_floor;

    if (elevator_direction == 0)
    {
        elevator_direction = start->next->floor - start->floor;
    }

    if (call_direction > 0)
    {
        if (elevator_direction > 0)
        {
            if (target >= elevator->current_floor)
            {
                end = find_direction_change_location(start, elevator_direction);
                return find_scheduled_place(start, end, start_floor, dest_floor, target);
            }
            else
            {
                // 다음 방향 같아질 때 까지 찾아야 함!
                start = find_direction_change_location(start, elevator_direction);
                elevator_direction *= -1;
                start = find_direction_change_location(start, elevator_direction);
                elevator_direction *= -1;
                end = find_direction_change_location(start, elevator_direction);
                return find_scheduled_place(start, end, start_floor, dest_floor, target);
            }
        }
        else
        {
            // 다음 방향 바뀔 때 까지 찾아야 함!
            start = find_direction_change_location(start, elevator_direction);
            elevator_direction *= -1;
            end = find_direction_change_location(start, elevator_direction);
            return find_scheduled_place(start, end, start_floor, dest_floor, target);
        }
    }
    else
    {
        if (elevator_direction < 0)
        {
            if (target <= elevator->current_floor)
            {
                end = find_direction_change_location(start, elevator_direction);
                return find_scheduled_place(start, end, start_floor, dest_floor, target);
            }
            else
            {
                // 다음 방향 같아질 때 까지 찾아야 함!
                start = find_direction_change_location(start, elevator_direction);
                elevator_direction *= -1;
                start = find_direction_change_location(start, elevator_direction);
                elevator_direction *= -1;
                end = find_direction_change_location(start, elevator_direction);
                return find_scheduled_place(start, end, start_floor, dest_floor, target);
            }
        }
        else
        {
            // 다음 방향 바뀔 때 까지 찾아야 함!
            start = find_direction_change_location(start, elevator_direction);
            elevator_direction *= -1;
            end = find_direction_change_location(start, elevator_direction);
            return find_scheduled_place(start, end, start_floor, dest_floor, target);
        }
    }
}

int find_time(F_list list, F_node *target, int start, int end)
{
    int time = 0;
    F_node *temp = target;
    int curr = start;

    if (temp == list.tail)
    {
        return abs(end - start);
    }

    while (temp != target)
    {
        time += abs(temp->floor - curr);
        curr = temp->floor;
        temp = temp->next;
    }

    time += abs(end - curr);

    return time;
}

int find_min(int *arr, int n)
{
    // 우선순위 규칙
    // 1. 시간이 최소로 걸리는 엘리베이터
    // 2. 운행 범위가 좁은 엘리베이터(저층 > 전층)
    // 3. 현재 사람 수가 적은 엘리베이터(이미 만원인 엘리베이터는 제외)
    // 4. 인덱스가 적은 엘리베이터

    int i;
    int min = 0;
    for (i = 1; i < n; i++)
    {
        if (arr[i] < arr[min])
        {
            min = i;
        }
    }
    //추가 우선순위에 대한 고려 필요
    return min;
}

void move_elevator(Elevator *elevators[6])
{
    int i;
    int to_ride = 0; // 태워야 할 사람 수
    int available;   // 정원이 초과될 시 최대로 태울수 있는 사람 수
    int leftover;    // 못 타고 남아있는 사람 수
    F_node *next_floor;

    // 1. 다음 목적지를 구한다(있으면).
    // 2. 현재 층과 비교햐여,
    // 3. 높으면 현재층 증가, 낮으면 감소, 같으면 사람을 태운다.
    // 4. 사람을 다 못 태우면 최대 수용 가능 인원만 태운다.
    // 4-1. 다 못 타고 남은 인원은 다시 엘리베이터를 호출한다.

    // (step 4 버그있음ㅜ)

    for (i = 0; i < NUM_ELEVATORS; i++)
    {
        if (F_list_size(elevators[i]->pending) > 0)
        {
            next_floor = F_list_peek(elevators[i]->pending);
            if (elevators[i]->fix)
            {
                fix_elevator(elevators[i]);
                continue;
            }
            else
            {
                if (elevators[i]->next_dest > elevators[i]->current_floor)
                {
                    (elevators[i]->current_floor)++;
                }
                else if (elevators[i]->next_dest < elevators[i]->current_floor)
                {
                    (elevators[i]->current_floor)--;
                }
                else
                {
                    elevators[i]->next_dest = next_floor->floor;
                    if (elevators[i]->current_floor == next_floor->floor)
                    {
                        elevators[i]->current_people += next_floor->people;
                        if (next_floor->people > 0)
                        {
                            elevators[i]->total_people += next_floor->people;
                        }
                        F_list_remove(elevators[i]->pending);
                    }
                }
            }
        }
    }

    /*
    for (i = 0; i < NUM_ELEVATORS; i++)
    {
        if (elevators[i]->fix)
        {
            fix_elevator(elevators[i]);
            continue;
        }

        if (elevators[i]->next_dest > elevators[i]->current_floor && elevators[i]->fix != 1)
        {
            (elevators[i]->current_floor)++;
        }
        else if (elevators[i]->next_dest < elevators[i]->current_floor && elevators[i]->fix != 1)
        {
            (elevators[i]->current_floor)--;
        }
        else
        {
            if (F_list_size(elevators[i]->pending) > 1)
            {
                next_floor = F_list_peek(elevators[i]->pending);
                available = MAX_PEOPLE - elevators[i]->current_people;

                if (available < next_floor->people)
                {
                    elevators[i]->current_people += available;
                    elevators[i]->total_people += available;
                    leftover = next_floor->people - available;

                    F_list_remove(elevators[i]->pending);
                    next_floor = F_list_peek(elevators[i]->pending);
                    next_floor->people = available * -1;
                    elevators[i]->next_dest = next_floor->floor;
                    R_list_insert(reqs, elevators[i]->current_floor, next_floor->floor, leftover);
                }
                else
                {
                    elevators[i]->current_people += next_floor->people;
                    if (next_floor->people > 0)
                    {
                        elevators[i]->total_people += next_floor->people;
                    }
                    elevators[i]->next_dest = next_floor->floor;
                    F_list_remove(elevators[i]->pending);
                    next_floor = F_list_peek(elevators[i]->pending);
                    if (next_floor->floor == -1)
                    {
                        F_list_remove(elevators[i]->pending);
                        elevators[i]->fix = 1;
                        fix_elevator(elevators[i]);
                        continue;
                    }
                }
            }
            else if (F_list_size(elevators[i]->pending) == 1)
            {
                next_floor = F_list_peek(elevators[i]->pending);
                elevators[i]->current_people += next_floor->people;
                F_list_remove(elevators[i]->pending);
            }
        }
    }
    */
}

void fix_elevator(Elevator *elevator)
{
    (elevator->fix_time)++;
    if (elevator->fix_time == 30)
    {
        elevator->fix = 0;
        elevator->fix_time = 0;
    }
}

void R_list_insert(R_list list, int current_floor, int dest_floor, int num_people)
{
    R_node *new_node = (R_node *)malloc(sizeof(R_node));
    new_node->next = list.tail;
    new_node->prev = new_node->next->prev;
    new_node->prev->next = new_node;
    new_node->next->prev = new_node;
    new_node->req.start_floor = current_floor;
    new_node->req.dest_floor = dest_floor;
    new_node->req.num_people = num_people;
}

int R_list_size(R_list list)
{
    int size = 0;
    R_node *curr = list.head->next;
    while (curr != list.tail)
    {
        size++;
        curr = curr->next;
    }

    return size;
}

Request *R_list_remove(R_list list)
{
    R_node *to_remove = list.head->next;
    Request *ret = &to_remove->req;

    to_remove->prev->next = to_remove->next;
    to_remove->next->prev = to_remove->prev;
    to_remove->next = NULL;
    to_remove->prev = NULL;

    free(to_remove);
    return ret;
}

void F_list_insert(F_list list, F_node *after, int floor, int people)
{
    F_node *new_node = (F_node *)malloc(sizeof(F_node));
    new_node->next = after;
    new_node->prev = new_node->next->prev;
    new_node->next->prev = new_node;
    new_node->prev->next = new_node;
    new_node->floor = floor;
    new_node->people = people;
}

int F_list_size(F_list list)
{
    F_node *curr = list.head->next;
    int num = 0;

    while (curr != list.tail)
    {
        num++;
        curr = curr->next;
    }

    return num;
}

void F_list_remove(F_list list)
{
    F_node *to_remove = list.head->next;

    to_remove->prev->next = to_remove->next;
    to_remove->next->prev = to_remove->prev;
    to_remove->next = NULL;
    to_remove->prev = NULL;

    free(to_remove);
}

F_node *F_list_peek(F_list list)
{
    return list.head->next;
}

void print_F_list(F_list list)
{
    F_node *curr = list.head->next;
    while (curr != list.tail)
    {
        printf("(%dF %d명) ", curr->floor, curr->people);
        curr = curr->next;
    }
}

int getch(void)
{
    int ch;
    struct termios buf, save;
    tcgetattr(0, &save);
    buf = save;
    buf.c_lflag &= ~(ICANON | ECHO);
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &buf);
    ch = getchar();
    tcsetattr(0, TCSAFLUSH, &save);
    return ch;
}