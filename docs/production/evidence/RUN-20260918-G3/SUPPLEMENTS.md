# 최종 PC 패키지 보충 검수 — 2026-09-28

세 실행 모두 **Pass**. 실제 GPU를 사용하는 별도 Win64 Development 패키지 두 프로세스로 실행했고 양쪽 정상 종료·오류0·상태 일치를 확인했다. 무음·화면 밖 렌더링·Engine/Slate 입력 주입이므로 실제 청음·사람의 입력·Android 검수는 포함하지 않는다. 자연5판과20분 부하의 정확한 이전 입력/결과는 [SUMMARY](SUMMARY.md), [PERFORMANCE](PERFORMANCE.md)에 유지한다.

패키지 입력은 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`, EXE SHA256은 `349F4660D6A0729807EA4FFD04DEE7A6D732001D00D830488EF00C42C049B177`이다. [입력 매핑](final-package-inputs.json)의73파일과 새 재현 Editor·58자동화·실제 PIE·compile/cook/archive를 거쳤다. 마지막 `f64cc671848560923595cc1955efe12620f326de`는 `Tests/LDWaveTests.cpp` 기대 사례197줄만 추가했고 제품·fixture·맵·데이터·설정·실행 도구 변경은0이다. 통합과 재현에서 해당 Waves9종을 다시 실행해 모두 무경고 Pass였다. 이 테스트 소스를 앞선 패키지 소스로 소급하지 않는다.

## 독립 기대값과 실제 관찰

| 실제 실행 | 기대값과 확인 결과 | 증거 |
|---|---|---|
| G3Boundary, 540×1170, 4매치 | host179/client86 검사, GUID4개, 실제 결과 복귀3회씩. D−0.001/D 타격은 보상 후 승리, D+0.001 타격은 시간초과로 취소. 일반1이면 보스둘 사망 뒤에도 Running, 마지막 일반 사망 즉시 승리. N99→100 첫 증가에서 즉시 패배 | [pair](final-G3Boundary-pair.json), [host](final-G3Boundary-host.json), [client](final-G3Boundary-client.json) |
| G3NetConflict, 540×1170 | host27/client14 검사. 실제 소유 client RPC 두 개가 같은 재료3개를 요청: Success1/StaleBoard1, population3→1, Rare R02/ID4, RNG1회만 진행, 재화·소환횟수 불변, 캐시2/게시1 | [pair](final-G3NetConflict-pair.json), [host](final-G3NetConflict-host.json), [client](final-G3NetConflict-client.json) |
| G3Entry, 360×780 | host35/client29 검사. Entry35초 대기는 매치 timeout 아님. 호스트 매치 Loading30초는 Aborted, 늦은 참가자는 유효 문맥/epoch를 얻지 못하고 owner1 골드100/빈 보드/캐시0 유지. host 실제 복귀 버튼2회+중복 의도에도 단일 복귀, client 네트워크 종료 후 Entry 오류 안내 | [pair](final-G3Entry-pair.json), [host](final-G3Entry-host.json), [client](final-G3Entry-client.json) |

경계 실행의 결과는 순서대로 `(승리, gold280/stars4/N0/boss0,0)`, `(시간초과, gold180/stars2/N0/boss0,1)`, `(승리, gold281/stars4/N0/boss0,0)`, `(한도패배, gold80/stars0/N100)`이며 두 참가자가 일치했다. 마감 직전 host200ms 실제 정지를 넣어도 예약 타격의 사건 시각을 보존했다. 새 매치마다 이전 판의 성공 소환 payload를 새 Controller로 보내 실제 InvalidEpoch 응답과 보드/경제/RNG/캐시 불변을 확인하고 새 요청은 정상 처리했다. NET-LIFE01의 패키지 재전송 검증을 닫는다.

경계 fixture는 준비·1~9웨이브를 생략하고 보스/일반 HP를1로 낮추며 타격 예정 시각을 배치한다. 적 한도 사례는 일반 적을 추가하고 예약 일반 생성을 억제한다. 결과·보상·실제 월드 시각은 직접 지정하지 않는다. 동시 요청은 무료 고정 재료를 제공하고, 종료는 정리용 Abort다. Entry는 정상 진입 경로를 사용하며 기기 입력은 주입한다. 이 결과는 자연 플레이의 승률·평균 클리어 시간을 뜻하지 않는다. host179에는 peer 준비 대기 중 반복된 baseline 검사도 포함되므로175였던 Editor 실행과 고유 규칙 수로 비교하지 않는다.

## 화면과 수명

경계 양쪽 원본9장씩을 관찰했다. 자기 보드 아래, 보스/잔여수/종료 사유, 결과 버튼과 조작 비활성 상태가 값과 일치한다. 정상 대기 화면의00초/N1/보스0,0은 승리 대기 규칙과 맞는다. [host 화면 해시](final-boundary-host-visual.json), [독립 client 화면 리뷰](final-boundary-client-visual.md). 축소 미리보기에서 누락처럼 보였던 버튼 글자는 원본 크기에서 정상으로 확인했고 파일 해시도 같았다. 표시 결함으로 등록하지 않았다.

[진입 시간초과](final-entry-loading-timeout.png), [host 복귀](final-entry-host-returned.png), [client 복귀](final-entry-client-returned.png)도 확인했다. 늦은 client의 종료 화면은 참가자 미등록으로 보드가 없으며 결과/복귀 UI만 표시한다. old World/Controller/Result의 표준 GC 수거·관찰자 해제·복귀 뒤3초 안정 상태를 확인했다. 별도 실제 PIE는 종료 위젯을 제거·재생성해 이전 위젯4개 수거·새 구독1개씩·서버 명령0을 재확인했다.

## 추가 Unreal 기대 사례

[통합 Editor](final-detail-editor.json)와 [Waves9종](final-detail-waves.json), [새 재현 결과](REPLAY_EVIDENCE.md)를 연결한다. 모두 실제 Unreal 객체를 사용한 NullRHI 검사이며 패키지 RPC의 정확 도착 시각을 증명하지 않는다.

- 준비9.999초 구매22/판매환급12 뒤 gold70/유닛1/소환2/보드·경제 revision3. 기존 Actor/ID와 명령 전후 공격 예약0.25 유지. Running 진입 후 만료된 타이머까지 동결해야 한다는 잘못된 기대는 두지 않는다.
- wave9에서 생성된 ID321/322, route0/1, 시각170, HP112의 같은 Actor가 wave10 시각190에도 유지된다. 일반2+보스2이며 명시 처치358회로 양쪽 gold458. 직접 시각/처치 fixture이므로 자연 전략 결과가 아니다.
- 첫 소환20/판매11 후 gold91, N99에서 World11.04에 실제 Controller 명령을 제출하면 앞선11초 생성으로 N100/EnemyLimit을 먼저 확정하고 새 명령은 PhaseNotAllowed. 기존 성공 원응답 전체는 재전송에서 유지한다.
- 같은9종에 준비29.999/30.0 허용·30.001 거절과 N101 방어도 포함된다. N101은 공개 카운터를100으로 변조한 실패 fixture이며 실제101 Actor 부하로 기록하지 않는다.

전체 로그와 원본 PNG: `C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/Replay-final-G3Boundary`, `Replay-final-G3NetConflict`, `Replay-final-G3Entry`. 마지막 회귀는 같은 폴더의 `Replay-G3-final-detail-editor`, `Replay-G3-final-detail-waves`, 통합 폴더의 `G3-final-detail-*`다. 모든 이전 실패 실행은 보존했다.

남은 필수 검수는 실제 거절 효과음 청음과 Android 구성 설치·APK·실기기 터치/SafeArea/10웨이브/성능이다. G3 자동·화면 검수 통과와 P0 최종 완료를 구분한다.
