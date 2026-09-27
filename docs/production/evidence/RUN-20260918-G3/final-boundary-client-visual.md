# 최종 G3Boundary client 화면 독립 검토

2026-09-28. **아래 원본 9장의 화면과 이번 패키지 실행 범위는 확인됨.** 구현자와 별도로 `view_image`로 모두 직접 열고 client result의86개 checks/4개 case, pair 정상 종료를 대조했다. 새 게임·빌드·포트 실행, 화면 생성/편집, 제품 코드 변경은 하지 않았다. P0 전체·Android·청음의 완료 판정은 아니다.

원본 경로: `C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/Replay-final-G3Boundary/client`. 코드 입력은 실행 담당이 고정한 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`; pair의 HEAD `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`는 조립 출발점이다. exe SHA256 `349F4660D6A0729807EA4FFD04DEE7A6D732001D00D830488EF00C42C049B177`. 실제 GPU Win64 Development,540×1170,60FPS 제한,RenderOffscreen,무음·자동 Slate/Controller 조작이다. HP1/예정 타격·단계 건너뛰기/추가 적은 명시 경계 fixture이며 자연 플레이가 아니다.

## 화면별 대조

모든 화면에 개인6×3 보드가 아래에 있고, 상단 상대 보드와 중앙 같은 방향 화살표가 보인다. 보드·핵심 수치·문구·하단 버튼은 화면 안에 들어오며 잘림/겹침을 발견하지 못했다. 소유자1의 실제 unit2/C03/cell35·인구1/20·보드 revision2는 결과 JSON과 대조했다. 각 결과 패널 뒤 전장은 어둡게 가려지며, 반환 버튼과 하단 자원은 읽을 수 있다.

| 원본 파일 | 직접 본 상태·JSON 대응 |
|---|---|
| case0-ready.png | WAVE10/10,01:00,2인 협동,N0/100,보스1·2 각1/6000. 골드80/별0/인구1/20,소환22골드 표시. 합성/판매 회색 |
| case0-result.png | 승리/양쪽 보스와 모든 일반 적 처치,WAVE10/10,00:00,전투 종료,N0,보스0/0. 골드280/별4,소환 비활성·전투 종료 안내·시작 화면으로 버튼. 결과 시각=D |
| case1-ready.png | WAVE10/10,01:00,N0,보스1/1,골드80/별0,소환22골드. 이전 판 결과/재화가 초기 화면에 남지 않음 |
| case1-result.png | 패배/보스 제한 시간 초과,00:00,N0,보스1은0/6000·보스2는1/6000. 골드180/별2,행동 버튼 비활성·반환 표시. D+.001 타격 미반영과 일치 |
| case2-ready.png | WAVE10/10,01:00,N1/100,보스1/1,골드80/별0. 추가 일반1은 별도 fixture 입력 |
| case2-normal-wait.png | WAVE10/10,00:00이지만 **2인 협동 유지**,N1/100,보스0/0,결과 패널 없음. 골드280/별4,소환22골드 정상 표시. 두 보스 사망 뒤 일반1 때문에 계속 Running인 JSON 관찰과 일치 |
| case2-result.png | 일반 정리 후 승리/양쪽 보스와 모든 일반 적 처치,N0,보스0/0,골드281/별4.00:00·전투 종료·비활성 소환·반환 표시. 결과=D+2 |
| case3-ready.png | WAVE1/10,00:20,N99/100이 주황 경고색. 보스HP 행 없음,골드80/별0,소환22골드. 아직 결과 패널 없음 |
| case3-result.png | 패배/일반 적 수 한도 도달,WAVE1/10,00:00,N100/100 주황색. 골드80/별0 유지,합성/판매/소환 문구 정상·비활성,반환 표시 |

기본 상세의 다중 이미지 미리보기에서 case2-normal-wait의 소환 글자와 case3-result의 합성/판매 글자가 비어 보였다. **동일 해시의 파일을 `detail: original`로 단독 다시 열어 모든 글자 존재를 확인했다.** 따라서 제품 글자 누락 결함이라는 초기 추정은 철회한다. 원본 파일이나 화면을 편집하지 않았다. 이 두 항목을 닫힌 제품 수정으로 기록해서도 안 된다.

## 실행 결과와 한계

[원본 pair](C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/Replay-final-G3Boundary/pair.json): Pass/PairStateConsistent=true,host179/client86 checks 전부 true,양쪽4 cases/3회 실제 반환/9 PNG,exit0,CriticalLog=false. client PID54412,실행254.724초이며 `engine.log`의 `LogExit: Exiting.`을 확인했다. 각 판 Battle/owner board/economy/실제 actor HP 비교·종료 상태 고정·입력 잠금·같은 유료 unit 유지 검사가 모두 true다. 다음3판에서 이전 payload를 새 Controller로 보낸 실제 `P0WIRE CLIENT code=5`와 새 보드·경제·cache·RNG 불변도 확인했다. 이는 이전 Actor 채널 패킷을 재현했다는 뜻은 아니다.

결과 MatchId 순서: `20F697F9418880E7970CE79420A6BD21`, `3B6923474D853D1D763A54849EBD70DF`, `8F27B2C44279E5425A8C70918E058F71`, `9978F4CE4D0F5CAEB0FF9E91DD4B6273`. case0 D=69.941100299,case1 D=65.024003100,case2 D=65.024410199/결과67.024410199,case3 한도 결과5.528733697초가 양쪽 기록과 일치한다. client의 authority hit 배열/host hitch 수치는 비어 있거나0인 것이 정상이며 서버 측 실제 타격 증거로 대신 사용하지 않았다.

추가 화면 표시 결함은 발견하지 못했다. 정지 PNG만으로 터치 성공·모든 프레임의 깜박임·모든 화면비·99개 개별 적 가시성·청음·Android를 입증하지 않는다. 같은 위치의 적이 겹치는 한도 fixture는 숫자와 실제 고유ID 검사로 확인했다. 버튼의 기능/반환은 이미지 색상만이 아니라 실제 검사/로그와 함께 판단했다. host9장은 통합 담당의 별도 검토 범위다.

## 파일 SHA256

| 파일 | SHA256 |
|---|---|
| case0-ready.png | `1B9B94D0C2CC934CE4FB160A7E7EA60CDDDB68AB49A5CD51EF8EEDB7263324B0` |
| case0-result.png | `A9E46E15C35382E0860D2707E44C50946DAAD77D8A992D342BE048C3087A4A24` |
| case1-ready.png | `C5BF908D6F7607D9C8C23839B75AA476C9E0FF5BA7C5AE980DCB5272D3C37444` |
| case1-result.png | `E046D91EB54732477BC7841F07D66FA332EB4C40F33E5ED67F09997FF2E2BD1B` |
| case2-ready.png | `9C29151772D2C460CD76DFA293CB3DA80A50A05B74966AA00154C58984CAC98F` |
| case2-normal-wait.png | `FDC29A6FA270F41D470284E0127F3C775A5C2CAB8B8DE1BDA9B0F885CF895B48` |
| case2-result.png | `80F045533BFA8094D0195D0EA08597E861DDC057198C36FDD50A8709AFAC669B` |
| case3-ready.png | `DA1DA69B8C30BEEDE7A53D7E8CC3681A43F52DD1E42013570247E9FB91B27961` |
| case3-result.png | `15E9FDB2A0D611C8D4CF71610E609C124358AC91AAA3BCCBDFC366AE8FCF2017` |
| client/result.json | `A6B34A253CF8E3BD727E62F394E3802B80E66D72B30D1FA970D28BD5872EC88D` |
| pair.json | `346F17DBFA1CBD0986D5BFD67DBE0C4CC6B02F13DF80C2639F1041273766C78A` |

R15의 실행층 해석: [원래 기대표](REVIEW_PLAN.md)는 정상 cooked 데이터/에셋의 실제 PKG 로드·화면·청음과 **UE 실패주입**을 요구한다. 따라서 정상 패키지 로드 + Unreal 데이터누락/부분생성 실패 근거를 연결하며, 별도 손상 pak 제작을 추가 필수 요구로 만들지 않는다. R01의 실제 실패 시작은 Entry의 LoadingTimeout 검수와 구분해 연결한다. 청음은 무음 로딩 검사로 대체하지 않는다.
