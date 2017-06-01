# 1	Introduction
## 1.1	Purpose
엘리베이터 시뮬레이션 시스템의 요구사항을 명세한 문서이다.  

## 1.2	Development environments
Programming Language: C
Editor/IDE: Cygwin Terminal, VI Editor
Compiler: GCC

# 2	Overall description
## 2.1	Product functions
Elevator Simulation System은 현재 건물의 엘리베이터가 어떻게 운행하는지를 보여준다. 엘리베이터는 상황에 따라 운행이 불가능할 수 있다.  
Elevator Simulation System은 현재 엘리베이터의 운행 여부를 보여주며, 운행 방향 및 운행 거리(출발 층에서 도착 층)를 보여준다.  
엘리베이터가 수리 중인 경우 수리에 대한 정보를 보여준다.  
Elevator Simulation System의 동작은 사용자의 입력을 통해서 이루어지며, 사용자는 시뮬레이션을 중단/재개/종료/재시작 할 수 있다.  
## 2.2	Design and Implementation Constraints  
건물은 1층에서 20층까지 총 20개의 층이 있다.  
엘리베이터는 총 6대이며, 운행하는 층의 범위가 다르다.(1층~10층 운행 2대, 1층, 11~20층 운행 2대, 1~20층 운행 2대) . 
엘리베이터는 초당 1층씩 움직일 수 있다.  
모든 엘리베이터의 정원은 15명이다. 정원이 다 찬 엘리베이터는 가장 가까운 목적 층까지 멈추지 않고 운행한다.  
건물의 각 층에서는 엘리베이터를 호출 할 때, 단순 위/아래 버튼을 누르는 것이 아닌 목적 층을 설정하는 것으로 호출한다. 이 또한 역시 Cygwin 에서의 키보드 입력으로 대체한다.  
## 2.3	Assumptions and Dependencies
모든 명령 및 입력은 Cygwin의 Command로 대체된다.
모든 출력은 cygwin의 화면으로 대체하며, 경보음은 시스템의 beep 음으로 한다.
기타 파일을 통하여 관리가 필요한 부분은 프로젝트 폴더에 파일 이름을 설정하여 저장한다.

# 3	Specific requirements
## 3.1	Interfaces
### 3.1.1	User Interfaces
입력: 키보드의 Q, W, E, R 키, 
출력: 화면(Console)
### 3.1.2	Software Interfaces

## 3.2	Functional requirements
### 3.2.1	화면 표시
화면은 항목 3.1.2 의 그림과 같이 건물 전체의 view를 보여주고, 엘리베이터의 움직임을 글로도 표시한다.
엘리베이터는 목적 층과 방향을 함께 출력한다.
### 3.2.2	Elevator Simulation System의 시작, 정지, 및 종료
#### 3.2.2.1	Elevator Simulation System의 시작과 종료
Elevator Simulation System을 실행하면 시뮬레이션은 자동으로 시작된다.
사용자가 종료 버튼을 누를 경우, 엘리베이터 시뮬레이션이 종료된다.
종료 버튼은 키보드의 Q 버튼으로 한다. 
엘리베이터의 운행 기록은 다음 시작 시 초기화된다.
#### 3.2.2.2	Elevator Simulation System의 정지
사용자가 정지 버튼을 누를 경우, 엘리베이터 시뮬레이션이 정지된다.
정지 버튼은 키보드의 W 버튼으로 한다.
모든 엘리베이터는 현재 층에서 멈춘다.
정지 상태에서는, 재개 버튼이나 종료 버튼이 눌릴 때 까지 아무런 변화도 일어나지 않는다.
엘리베이터의 운행 기록은 초기화되지 않는다.
#### 3.2.2.3	Elevator Simulation System의 재개
사용자가 재개 버튼을 누를 경우, 정지 상태의 엘리베이터 시뮬레이션이 다시 동작한다.
재개 버튼은 키보드의 E 버튼으로 한다.
정지 상태가 아닐 때 재개 버튼을 눌러도 아무 일도 일어나지 않는다.
엘리베이터 호출 요청 및 운행 기록은 초기화되지 않는다.
#### 3.2.2.4	Elevator Simulation System의 재시작
사용자가 재시작 버튼을 누를 경우, 엘리베이터 시뮬레이션이 처음부터 다시 시작한다.
재시작 버튼은 키보드의 R 버튼으로 한다.
모든 들어와 있는 요청 및 운행 기록은 초기화된다.
### 3.2.3	Elevator의 움직임
#### 3.2.3.1	엘리베이터 운행 범위
엘리베이터는 총 6대가 있으며, 각각 운행 범위가 다르다.
저층용 엘리베이터 2대는 1층에서 10층까지만 운행한다.
고층용 엘리베이터 2대는 11층에서 20층까지만 운행한다.
전층용 엘리베이터는 1층에서 20층까지 모두 운행한다.
#### 3.2.3.2	엘리베이터의 소요 시간
엘리베이터는 1초당 1층씩 움직일 수 있다.
엘리베이터가 승객을 태우거나 내리는 데 걸리는 시간은 승객 3명당 1초이다. (예시: 5명 탑승 시 2초 소요, 10명 하차 시 4초 소요)
엘리베이터가 승객을 태우려고 시도하였으나 정원이 초과되어 못 태운 경우, 소요시간은 (현재 엘리베이터가 수용 가능한 인원 수를 태우는 데 걸리는 시간 + 1초) 이다.
#### 3.2.3.3	엘리베이터의 정원
모든 엘리베이터의 정원은 15명이다.
#### 3.2.3.4	엘리베이터의 운행 방침
엘리베이터는, 운행 시간 및 승객 수용 시간을 고려하여, 목적 층에 도달하는 시간이 최소가 되도록 운행한다.
엘리베이터가 해당 층의 사람을 전부 태울 수 없는 경우, 수용 가능한 최대 인원만 태운다. 남아있는 사람은 다음 엘리베이터를 이용한다.
### 3.2.4	엘리베이터 호출 모드
#### 3.2.4.1	엘리베이터 호출 모드 돌입
사용자는 엘리베이터 호출 버튼을 눌러서 호출 모드로 돌입할 수 있다.
호출 버튼은 키보드의 A 버튼으로 한다.
#### 3.2.4.2	현재 층 입력
엘리베이터 호출 모드 돌입 시, 사용자는 현재 층을 먼저 입력해야 한다.
사용자는 키보드로 숫자 1에서 20을 입력하여 현재 층을 입력할 수 있다.
잘못된 입력이 들어올 경우, 에러 메시지를 적절하게 출력하고, 유효한 입력이 들어올 때 까지 입력을 계속 받는다.
#### 3.2.4.3	목적 층 및 사람 수 입력
사용자가 현재 층을 입력하고 난 후, 시스템은 목적 층 및 사람 수를 공백으로 구분하여 입력하도록 요구한다.
사용자는 키보드로 숫자를 입력하여 목적 층 및 사람 수를 입력할 수 있다.
사용자가 입력하는 형식은 (목적 층, 사람 수) 이다. 다른 경우는 생각하지 않는다.
잘못된 입력이 들어올 경우, 에러 메시지를 적절하게 출력하고, 유효한 입력이 들어올 때 까지 입력을 계속 받는다.
입력을 완료하면 다시 목적 층 및 사람 수를 입력하는 화면으로 전환된다. 사용자는 다시 엘리베이터 호출 버튼을 눌러서 호출을 종료할 때 까지 현재 층에서의 호출을 무제한으로 생성할 수 있다.
#### 3.2.4.4	엘리베이터 호출 완료
엘리베이터 호출 모드에서, 사용자가 다시 엘리베이터 호출 버튼을 눌러서 해당 층에서의 엘리베이터 호출을 완료할 수 있다.
### 3.2.5	엘리베이터 운행 일지 기록
#### 3.2.5.1	
### 3.2.6	엘리베이터 점검 모드
#### 3.2.6.1	엘리베이터의 점검 기준
모든 엘리베이터는 태운 승객의 수가 150명이 넘어가는 순간에 점검이 필요하다. 
#### 3.2.6.2	엘리베이터의 점검 시작
엘리베이터가 점검이 필요하면, 더 이상의 추가 호출에 응하지 않는다.
엘리베이터는 현재 수용 중인 승객이 다 내리고, 마지막으로 정지한 층에서 점검에 들어간다.
#### 3.2.6.3	엘리베이터 점검 시간
엘리베이터가 점검을 완료하는 시간은 30초이다.
#### 3.2.6.4	화면 출력
엘리베이터가 점검 중인 경우, 화면에 FIX 라고 표시한다.

