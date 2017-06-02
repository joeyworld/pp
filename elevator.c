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

/* 전역 변수 */
R_list up;
R_list down;
int flag = 0;

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
void R_list_insert(R_list list, int current_floor, int dest_floor, int num_people);

int main(void) {
    Input *input;
    Simul *simul;
    Elevator *elevators[6];
    pthread_t input_thr;
    pthread_t simul_thr;
    int tid_input;
    int tid_simul;

    init(&input, &simul, elevators);

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
    up.head = (R_node *)malloc(sizeof(R_node));
    up.tail = (R_node *)malloc(sizeof(R_node));
    down.head = (R_node *)malloc(sizeof(R_node));
    down.tail = (R_node *)malloc(sizeof(R_node));
    up.head->next = up.tail;
    up.tail->prev = up.head;
    down.head->next = down.tail;
    down.tail->prev = down.head;

    *input = (Input *)malloc(sizeof(Input));
    (*input)->mode = (char *)malloc(sizeof(char));
    (*input)->req_current_floor = (int *)malloc(sizeof(int));
    (*input)->req_dest_floor = (int *)malloc(sizeof(int));
    (*input)->req_num_people = (int *)malloc(sizeof(int));

    *simul = (Simul *)malloc(sizeof(Simul));
    (*simul)->input = *input;

    for(i = 0; i < NUM_ELEVATORS; i++) {
        elevators[i] = (Elevator *)malloc(sizeof(Elevator));
    }

    for(i = 0; i < NUM_ELEVATORS; i++) {
        elevators[i]->pending.head = (F_node *)malloc(sizeof(F_node));
        elevators[i]->pending.tail = (F_node *)malloc(sizeof(F_node));
        elevators[i]->pending.head->floor = 0;
        elevators[i]->pending.tail->floor = 0;
        elevators[i]->pending.head->next = elevators[i]->pending.tail;
        elevators[i]->pending.tail->prev = elevators[i]->pending.head;

        elevators[i]->current_floor = 1;
        elevators[i]->next_dest = 1;
        elevators[i]->direction = 0;
        elevators[i]->curr_people = 0;
        elevators[i]->total_people = 0;
        elevators[i]->fix = 0;
    }

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

        //요청 큐에 추가
        insert_into_queue(*simul->input->req_current_floor, *simul->input->req_dest_floor, *simul->input->req_num_people);
        
        //엘리베이터 이동시키기

        time++;
        sleep(1);
    }
}

void print_UI(Elevator *elevators[6]) {
    int i, j;
    for(i = 0; i < FLOOR; i++) {
        printf("    ------------------------------------------------------ \n"); //윗칸
        printf("%2dF ", FLOOR - i);
        for(j = 0; j < NUM_ELEVATORS; j++) {
            if(elevators[j]->current_floor == FLOOR - i) {
                printf("|");
                if(elevators[j]->direction == 0) {
                    printf(" 대기중 ");
                } else {
                    //TODO further implementation when moving
                }
            } else {
                printf("|        ");
            }
        }
        printf("| \n");
    }
    printf("    ------------------------------------------------------ \n"); //아랫칸
}

void print_elevator_info(Elevator *elevators[6]) {
    int i;
    R_node *curr1 = up.head->next;
    R_node *curr2 = down.head->next;

    printf("상행 요청: ");
    while(curr1 != up.tail) {
        printf("%d층 -> %d층 %d명, ", curr1->req.start_floor, curr1->req.dest_floor, curr1->req.num_people);
        curr1 = curr1->next;
    }

    printf("\n");

    printf("하행 요청: ");
    while(curr2 != down.tail) {
        printf("%d층 -> %d층 %d명, ", curr2->req.start_floor, curr2->req.dest_floor, curr2->req.num_people);
        curr2 = curr2->next;
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
        printf("\n------엘리베이터 호출 모드------ \n");
        printf("현재 층 : %d, 목적 층 : %d, 사람 수 : %d \n", *input->req_current_floor, *input->req_dest_floor, *input->req_num_people);
        printf("h : 현재 층 증가, j : 목적 층 증가, k : 사람 수 증가 \n");
        printf("입력을 완료하려면 l를 누르십시오 \n");
        printf("-------------------------------- \n");
    } else {
        printf("\n-----------MENU------------ \n");
        printf("Q : 종료       \t");
        printf("W : 정지       \n");
        printf("E : 재개       \t");
        printf("R : 재시작     \n");
        printf("A : 호출       \n");
        printf("--------------------------- \n");
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
        simul->elevators[i]->direction = 0;
        simul->elevators[i]->curr_people = 0;
        simul->elevators[i]->total_people = 0;
        simul->elevators[i]->fix = 0;
    }

    *time = 0;
    *simul->input->mode = 0;
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

    if(dest_floor - current_floor > 0) {
        R_list_insert(up, current_floor, dest_floor, num_people);
    } else if(dest_floor - current_floor < 0) {
        R_list_insert(down, current_floor, dest_floor, num_people);
    }

    flag = 0;
}

void R_list_insert(R_list list, int current_floor, int dest_floor, int num_people) {
    R_node *new_node = (R_node *)malloc(sizeof(R_node));
    new_node->prev = list.head;
    new_node->next = list.head->next;
    new_node->prev->next = new_node;
    new_node->next->prev = new_node;
    new_node->req.start_floor = current_floor;
    new_node->req.dest_floor = dest_floor;
    new_node->req.num_people = num_people;
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
