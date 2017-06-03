#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>

#define QUIT 'Q'
#define PAUSE 'W'
#define RESUME 'E'
#define RESTART 'R'
#define CALL 'A'
#define FLOOR 20
#define NUM_ELEVATORS 6

/* 요청 구조체 */
typedef struct _REQUEST {
    int start_floor; //현재층(요청이 이루어지는)
    int dest_floor; //목적층
    int num_people; //몇 명이 탄다
}Request;

typedef struct _PEOPLE{
    int number; //사람수
    int dest; //목적층
}People;

typedef struct _FLOORNODE {
    struct _FLOORNODE *prev;
    struct _FLOORNODE *next;
    int floor;
}F_node;

typedef struct _PEOPLENODE {
    struct _PEOPLENODE *prev;
    struct _PEOPLENODE *next;
    People p;
}P_node;

typedef struct _REQUESTNODE {
    struct _REQUESTNODE *prev;
    struct _REQUESTNODE *next;
    Request req;
}R_node;

typedef struct _FLOORLIST {
    F_node *head;
    F_node *tail;
}F_list;

typedef struct _PEOPLELIST {
    P_node *head;
    P_node *tail;
}P_list;

typedef struct _REQUESTLIST {
    R_node *head;
    R_node *tail;
}R_list;

/* 엘리베이터 구조체 */
typedef struct _ELEVATOR {
    int current_floor;
    int next_dest;
    int direction;
    int curr_people;
    int total_people;
    int fix;
    F_list pending;
}Elevator;

typedef struct _INPUT {
    char *mode;
    int *req_current_floor;
    int *req_dest_floor;
    int *req_num_people;
}Input;

typedef struct _SIMUL {
    Elevator **elevators;
    Input *input;
}Simul;

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
void simul_restart(int *time, Simul *simul);
void get_request(Input *input);
void insert_into_queue(int current_floor, int dest_floor, int num_people);
void accept_request(int start_floor, int dest_floor, int num_people);
Elevator *find_elevator(Elevator *elevators[6], R_list list);
int F_list_size(F_list list);
void move_elevator(Elevator *elevators[6]);
void R_list_insert(R_list list, int current_floor, int dest_floor, int num_people);
int R_list_size(R_list list);
Request *R_list_remove(R_list list);
void P_list_insert(P_list list, int dest_floor, int num_people);
int P_list_size(P_list list);
P_node *P_list_remove(P_list list);
void F_list_insert(F_list list);
int F_list_size(F_list list);
F_node *F_list_remove(F_list list);


/* 전역 변수 */
P_list building[20];
R_list reqs;
int flag = 0;

int main(void) {
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
    insert_into_queue(4, 11, 5);
    flag = 1;
    insert_into_queue(13, 18, 7);
    flag = 1;
    insert_into_queue(9, 2, 4);
    flag = 1;
    insert_into_queue(20, 12, 11);
    flag = 1;
    insert_into_queue(1, 5, 2);
    flag = 1;
    insert_into_queue(18, 7, 10);
    flag = 0;
    //test end

    tid_input = pthread_create(&input_thr, NULL, input_f, (void *)input);
    if(tid_input != 0) {
        perror("thread creation error: ");
        exit(0);
    }

    tid_simul = pthread_create(&simul_thr, NULL, simul_f, (void *)simul);
    if(tid_simul != 0) {
        perror("thread creation error: ");
        exit(0);
    }

    pthread_join(input_thr, NULL);
    pthread_join(simul_thr, NULL);

    return 0;
}

void init(Input **input, Simul **simul, Elevator *elevators[6]) {
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

    for(i = 0; i < FLOOR; i++) {
        building[i].head = (P_node *)malloc(sizeof(P_node));
        building[i].tail = (P_node *)malloc(sizeof(P_node));
        building[i].head->prev = NULL;
        building[i].head->next = building[i].tail;
        building[i].tail->prev = building[i].head;
        building[i].tail->next = NULL;
    }

    for(i = 0; i < NUM_ELEVATORS; i++) {
        elevators[i] = (Elevator *)malloc(sizeof(Elevator));
    }


    //엘리베이터 : 1, 2 - 저층, 3, 4 - 전층, 5, 6 - 고층
    for(i = 0; i < NUM_ELEVATORS; i++) {
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
        elevators[i]->direction = 0;
        elevators[i]->curr_people = 0;
        elevators[i]->total_people = 0;
        elevators[i]->fix = 0;
    }

    elevators[3]->current_floor = 11;
    elevators[3]->next_dest = 11;
    elevators[4]->current_floor = 11;
    elevators[4]->next_dest = 11;


    (*simul)->elevators = elevators;
}

void *input_f(void *data) {
    Input *input = (Input *)data;
    while(1) {
        if(*input->mode == QUIT) {
            break;
        } else if(*input->mode == CALL) {
            get_request(input);
        } else {
            scanf(" %c", input->mode);
        }
    }
}

void *simul_f(void *data) {
    Simul *simul = (Simul *)data;
    int time = 0;

    while(1) {
        system("clear");
        print_UI(simul->elevators);
        printf("시뮬레이션 시작한 지 %d 초 경과 \n", time);
        print_elevator_info(simul->elevators);
        print_menu(*simul->input->mode, simul->input);

        if(*simul->input->mode == QUIT) {
            quit(simul->elevators);
        } else if(*simul->input->mode == PAUSE) {
            simul_stop(simul->input->mode);
        } else if(*simul->input->mode == RESTART) {
            simul_restart(&time, simul);
            continue;
        }

        //요청 큐에 추가 & 건물 정보 업데이트
        insert_into_queue(*simul->input->req_current_floor, *simul->input->req_dest_floor, *simul->input->req_num_people);

        //엘리베이터 찾기 및 이동
        if(R_list_size(reqs) != 0) {
            find_elevator(simul->elevators, reqs);
        }

        move_elevator(simul->elevators);

        time++;
        sleep(1);
    }
}

void print_UI(Elevator *elevators[6]) {
    int i, j;
    P_node *curr;
    for(i = 0; i < FLOOR; i++) {
        printf("    ------------------------------------------------------------ \n"); //윗칸
        printf("%2dF ", FLOOR - i);
        for(j = 0; j < NUM_ELEVATORS; j++) {
            if(elevators[j]->current_floor == FLOOR - i) {
                printf("|");
                if(elevators[j]->current_floor - elevators[j]->next_dest == 0) {
                    printf(" 대기중  ");
                } else if(elevators[j]->current_floor - elevators[j]->next_dest > 0) {
                    printf(" ▼");
                    printf(" %dF", elevators[j]->next_dest);
                    //TODO further implementation when moving
                } else {
                    printf("▲ ");
                    printf(" %dF ", elevators[j]->next_dest);
                }
            } else {
                printf("|         ");
            }
        }
        printf("| ");

        printf("대기인원: ");
        curr = building[FLOOR - i - 1].head->next;
        while(curr != building[FLOOR -i -1].tail) {
            printf("%d층으로 %d명, ", curr->p.dest, curr->p.number);
            curr = curr->next;
        }

        printf("\n");
    }
    printf("    ------------------------------------------------------------ \n"); //아랫칸
}

void print_elevator_info(Elevator *elevators[6]) {
    int i;
    R_node *curr = reqs.head->next;

    printf("현재 요청: ");
    while(curr != reqs.tail) {
        printf("%d층 -> %d층 %d명, ", curr->req.start_floor, curr->req.dest_floor, curr->req.num_people);
        curr = curr->next;
    }

    printf("\n");

    for(i = 0; i < NUM_ELEVATORS; i++) {
        printf("엘리베이터 %d : ", i + 1);
        if(elevators[i]->direction == 0) {
            printf("대기 중. ");
        } else {
            //TODO further implementataion regarding requests
            printf("%d명 탑승 중. ", elevators[i]->curr_people);
        }
        printf("누적 승객 수 : %d \n", elevators[i]->total_people);
    }
}

void print_menu(char mode, Input *input) {
    if(mode == CALL) {
        printf("\n엘리베이터 호출 모드\n");
        printf("현재 층 : %d, 목적 층 : %d, 사람 수 : %d \n", *input->req_current_floor, *input->req_dest_floor, *input->req_num_people);
        printf("h : 현재 층 증가, j : 목적 층 증가, k : 사람 수 증가 \n");
        printf("입력을 완료하려면 l를 누르십시오 \n");
    } else {
        printf("Q : 종료\t");
        printf("W : 정지\t");
        printf("E : 재개\t");
        printf("R : 재시작\t");
        printf("A : 호출\n");
        printf("메뉴 선택 : ");
    }
    fflush(stdout);
}

void quit(Elevator *elevators[6]) {
    int i;

    printf("엘리베이터 시뮬레이션 시스템을 종료합니다. \n");
    for(i = NUM_ELEVATORS - 1; i >= 0; i--) {
        free(elevators[i]->pending.tail);
        free(elevators[i]->pending.head);
    }

    for(i = NUM_ELEVATORS - 1; i >= 0; i--) {
        free(elevators[i]);
    }

    //TODO modification
    exit(0);
}

void simul_stop(char *mode) {
    while(*mode != RESUME && *mode != QUIT && *mode != RESTART) {
    }
}

void simul_restart(int *time, Simul *simul) {
    int i;
    for(i = 0; i < NUM_ELEVATORS; i++) {
        simul->elevators[i]->current_floor = 1;
        simul->elevators[i]->next_dest = 1;
        simul->elevators[i]->direction = 0;
        simul->elevators[i]->curr_people = 0;
        simul->elevators[i]->total_people = 0;
        simul->elevators[i]->fix = 0;
    }

    simul->elevators[3]->current_floor = 11;
    simul->elevators[3]->next_dest = 11;
    simul->elevators[4]->current_floor = 11;
    simul->elevators[4]->next_dest = 11;

    *time = 0;
    *simul->input->mode = 0;

    //요청 목록 초기화
    //빌딩 정보 초기화
}

void get_request(Input *input) {
    char key;

    *input->req_current_floor = 1;
    *input->req_dest_floor = 1;
    *input->req_num_people = 1;

    while(1) {
        key = getch();
        if(key == 'l') {
            *input->mode = 0;
            flag = 1;
            break;
        }
        if(key == 'h') {
            (*input->req_current_floor)++;
            if(*input->req_current_floor > FLOOR) {
                *input->req_current_floor = 1;
            }
        }
        if(key == 'j') {
            (*input->req_dest_floor)++;
            if(*input->req_dest_floor > FLOOR) {
                *input->req_dest_floor = 1;
            }
        }
        if(key == 'k') {
            (*input->req_num_people)++;
            if(*input->req_num_people == 60) {
                *input->req_num_people = 1;
            }
        }
    }

    flag = 1;
}

void insert_into_queue(int current_floor, int dest_floor, int num_people) {
    if(flag == 0) {
        return;
    }

    if(current_floor == dest_floor) {
        return;
    }

    if(current_floor > FLOOR || current_floor < 1 || dest_floor > FLOOR || dest_floor < 1) {
        return;
    }

    R_list_insert(reqs, current_floor, dest_floor, num_people);
    accept_request(current_floor, dest_floor, num_people);

    flag = 0;
}

void R_list_insert(R_list list, int current_floor, int dest_floor, int num_people) {
    R_node *new_node = (R_node *)malloc(sizeof(R_node));
    new_node->prev = list.tail->prev;
    new_node->next = list.tail;
    new_node->prev->next = new_node;
    new_node->next->prev = new_node;
    new_node->req.start_floor = current_floor;
    new_node->req.dest_floor = dest_floor;
    new_node->req.num_people = num_people;
}

int R_list_size(R_list list) {
    int size = 0;
    R_node *curr = list.head->next;
    while(curr != list.tail) {
        size++;
        curr = curr->next;
    }

    return size;
}

Request *R_list_remove(R_list list) {
    R_node *to_remove = list.head->next;
    Request *ret = &to_remove->req;

    to_remove->prev->next = to_remove->next;
    to_remove->next->prev = to_remove->prev;
    to_remove->next = NULL;
    to_remove->prev = NULL;

    free(to_remove);
    return ret;
}

void accept_request(int start_floor, int dest_floor, int num_people) {
    P_list_insert(building[start_floor - 1], dest_floor, num_people);
}

void P_list_insert(P_list list, int dest_floor, int num_people) {
    P_node *new_node = (P_node *)malloc(sizeof(P_node));
    new_node->next = list.tail;
    new_node->prev = new_node->next->prev;
    new_node->prev->next = new_node;
    new_node->next->prev = new_node;
    new_node->p.number = num_people;
    new_node->p.dest = dest_floor;
}

Elevator *find_elevator(Elevator *elevators[6], R_list list) {
    //1. 큐에 요청을 뺀다
    //2. 각각에 가상의 스케쥴링을 실행한다
    //3. 시간을 구한다
    //4. 최소 시간 걸리는 엘리베이터 리턴
}

int F_list_size(F_list list) {
    F_node *curr = list.head->next;
    int num = 0;

    while(curr != list.tail) {
        num++;
        curr = curr->next;
    }

    return num;
}

void move_elevator(Elevator *elevators[6]) {

}

int getch(void) {
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
