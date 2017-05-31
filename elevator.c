#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLOOR 20

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
    int num_people; //현재 타고있는 사람 수
    int fix; //수리 중인지 여부
    Queue reqs; //현재 들어와있는 요청
}Elevator;

/* 함수 헤더 */
void print_menu();

int main() {
    Elevator low1, low2;
    Elevator high1, high2;
    Elevator all1, all2;

    low1.reqs.front = (Node *)malloc(sizeof(Node));
    low1.reqs.rear = (Node *)malloc(sizeof(Node));
    low2.reqs.front = (Node *)malloc(sizeof(Node));
    low2.reqs.rear = (Node *)malloc(sizeof(Node));
    high1.reqs.front = (Node *)malloc(sizeof(Node));
    high1.reqs.rear = (Node *)malloc(sizeof(Node));
    high2.reqs.front = (Node *)malloc(sizeof(Node));
    high2.reqs.rear = (Node *)malloc(sizeof(Node));
    all1.reqs.front = (Node *)malloc(sizeof(Node));
    all1.reqs.rear = (Node *)malloc(sizeof(Node));
    all2.reqs.front = (Node *)malloc(sizeof(Node));
    all2.reqs.rear = (Node *)malloc(sizeof(Node));

    print_menu();

    return 0;
}

void print_menu() {
    printf("-----MENU----- \n");
    printf("Q : 종료       \n");
    printf("W : 정지       \n");
    printf("E : 재개       \n");
    printf("R : 재시작     \n");
    printf("A : 호출       \n");
    printf("-------------- \n");
}
