#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

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

/* 요청 큐 노드 */
typedef struct _NODE {
    struct _NODE *prev;
    struct _NODE *next; 
    Request req; //요청 정보
}Node;

/* 요청 큐 */
typedef struct _QUEUE {
    Node *front;
    Node *rear;
}Queue;

/* 엘리베이터 구조체 */
typedef struct _ELEVATOR {
    int floor[FLOOR]; //각 층에 멈추는지 여부
    int is_moving; //운행 중인지 여부
    int current_floor; //현재 위치해 있는 층
    int curr_people; //현재 타고있는 사람 수
    int total_people; //현재까지 총 태운 사람 수
    int fix; //수리 중인지 여부
    Queue reqs; //현재 들어와있는 요청
}Elevator;

/* input_t thread에서 필요한 변수 */
typedef struct _INPUT {
    char *menu_input;
    int *floor_input;
    int *dest_input;
    int *people_input;
}Input;

/* simul_t thread에서 필요한 변수 */
typedef struct _SIMUL {
    Input *input;
    Elevator *elevators[6];
}Simul;

/* 함수 헤더 */
void gotoxy(int x, int y);
void *input_f(void *data); //input_t thread에서 실행되는 함수
void *simul_f(void *data); //simul_t thread에서 실행되는 함수
void allocate(Input **input, Simul **simul); //동적할당 처리
void init(Elevator *elevators[6]);
void print_UI(Elevator *elevators[6]);
void print_elevator_info(Elevator *elevators[6]);
void print_menu(char mode);
char get_menu_input();
void get_request(Input *input);
void quit(Elevator *elevators[6]);
void simul_stop(char *mode);
void simul_restart(int *time, Simul *simul);
Elevator *call_elevator(Elevator *elevators[6]); //움직여야 할 엘리베이터 찾는 함수

int main(void) {
    Input *input;
    Simul *simul;
    pthread_t input_t;
    pthread_t simul_t;
    int tid_input;
    int tid_simul;

    allocate(&input, &simul);

    tid_input = pthread_create(&input_t, NULL, input_f, (void *)input);
    if(tid_input != 0) {
        perror("thread creation error: ");
    }

    tid_simul = pthread_create(&simul_t, NULL, simul_f, (void *)simul);
    if(tid_simul != 0) {
        perror("thread creation error: ");
    }

    pthread_join(input_t, NULL);
    pthread_join(simul_t, NULL);

    free(simul);
    free(input->people_input);
    free(input->dest_input);
    free(input->floor_input);
    free(input->menu_input);
    free(input);

    return 0;
}

void allocate(Input **input, Simul **simul) {
    *input = (Input *)malloc(sizeof(Input));
    (*input)->menu_input = (char *)malloc(sizeof(char));
    (*input)->floor_input = (int *)malloc(sizeof(int));
    (*input)->dest_input = (int *)malloc(sizeof(int));
    (*input)->people_input = (int *)malloc(sizeof(int));
    *simul = (Simul *)malloc(sizeof(Simul));
    (*simul)->input = *input;
}

void *input_f(void *data) {
    Input *input = (Input *)data;
    while(1) {
        if(*input->menu_input == QUIT) {
            break;
        } else if(*input->menu_input == CALL) {
            get_request(input);
        } else {
            *input->menu_input = get_menu_input();
        }
    }
}

void *simul_f(void *data) {
    int time = 0;
    Elevator *to_move = NULL;
    Simul *simul = (Simul *)data;
    init(simul->elevators);
    while(1) {
        //gotoxy(0, 0);
        system("clear");
        print_UI(simul->elevators);
        printf("시뮬레이션 시작한 지 %d 초 경과 \n", time);
        printf("현재 요청 : %d층->%d층 %d명 \n", *simul->input->floor_input, *simul->input->dest_input, *simul->input->people_input);
        print_elevator_info(simul->elevators);
        print_menu(*simul->input->menu_input);

        if(*simul->input->menu_input == QUIT) {
            quit(simul->elevators);
        } else if(*simul->input->menu_input == PAUSE) {
            simul_stop(simul->input->menu_input);
        } else if(*simul->input->menu_input == RESTART) {
            simul_restart(&time, simul);
            continue;
        }

        //Elevator call
        to_move = call_elevator(simul->elevators);
        if(to_move != NULL) {
        }

        time++;
        sleep(1);
    }
}

void init(Elevator *elevators[6]) {
    int i, j;
    for(i = 0; i < NUM_ELEVATORS; i++) {
        elevators[i] = (Elevator *)malloc(sizeof(Elevator));
    }
    for(i = 0; i < NUM_ELEVATORS; i++) {
        //노드 동적할당
        elevators[i]->reqs.front = (Node *)malloc(sizeof(Node));
        elevators[i]->reqs.rear = (Node *)malloc(sizeof(Node));

        //엘리베이터 별로 다르게 층 초기화
        if(i < 2) {
            for(j = 0; j < 10; j++) {
                elevators[i]->floor[j] = 1;
            }
        } else if(i < 4) {
            for(j = 10; j < FLOOR; j++) {
                elevators[i]->floor[j] = 1;
            }
        } else {
            for(j = 0; j < FLOOR; j++) {
                elevators[i]->floor[j] = 1;
            }
        }

        //다른 변수들 초기값 설정
        elevators[i]->current_floor = 1;
        elevators[i]->is_moving = 0;
        elevators[i]->curr_people = 0;
        elevators[i]->total_people = 0;
        elevators[i]->fix = 0;
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
                if(elevators[j]->is_moving == 0) {
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

    for(i = 0; i < NUM_ELEVATORS; i++) {
        printf("엘리베이터 %d : ", i + 1);
        if(elevators[i]->is_moving == 0) {
            printf("대기 중. ");
        } else {
            //TODO further implementataion regarding requests
            printf("%d명 탑승 중. ", elevators[i]->curr_people);
        }
        printf("누적 승객 수 : %d \n", elevators[i]->total_people);
    }
}

void print_menu(char mode) {
    if(mode == CALL) {
        printf("---엘리베이터 호출 모드--- \n");
        printf("현재 층, 목적 층, 사람 수 순서대로 : ");
    } else {
        printf("-----------MENU------------ \n");
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

char get_menu_input() {
    char input;
    scanf(" %c", &input);
    return input;
}

void get_request(Input *input) {
    scanf(" %d %d %d", input->floor_input, input->dest_input, input->people_input);
    *input->menu_input = 0;
}

void quit(Elevator *elevators[6]) {
    int i;

    printf("엘리베이터 시뮬레이션 시스템을 종료합니다. \n");
    for(i = NUM_ELEVATORS - 1; i >= 0; i--) {
        free(elevators[i]->reqs.rear);
        free(elevators[i]->reqs.front);
    }

    for(i = NUM_ELEVATORS - 1; i >= 0; i--) {
        free(elevators[i]);
    }

    //TODO modification
    exit(0);
}

//TODO implementation of stop, resume, and restart
void simul_stop(char *mode) {
    while(*mode != RESUME && *mode != QUIT && *mode != RESTART) {
    }
}

void simul_restart(int *time, Simul *simul) {
    int i;
    for(i = 0; i < NUM_ELEVATORS; i++) {
        simul->elevators[i]->is_moving = 0;
        simul->elevators[i]->curr_people = 0;
        simul->elevators[i]->total_people = 0;
        simul->elevators[i]->fix = 0;
    }

    *time = 0;
    *simul->input->menu_input = 0;
}

void gotoxy(int x, int y) {
    printf("\033[%d;%df", y, x);
    fflush(stdout);
}
