# G3 이번 구현·검수 요약

실행일2026-09-27~28. **PC 자동·화면 검수 통과, 실제 청음 대기**. G4 NotRun이며 P0 최종 미완료다. 실제 학습자는 Planned. [공통 작업 기록](../../P0_REFERENCE_RUN.md), [독립 기대값](REVIEW_PLAN.md), [결함·구조 리뷰](REVIEW_FINDINGS.md), [최종 독립 리뷰](FINAL_REVIEW.md).

| 실행 층 | 확인한 결과 | 한계·남은 범위 |
|---|---|---|
| 문서·데이터·스타일 | [마감 검사](final-checks.json): 데이터2689·기획8279검사, C++스타일83파일 오류0, 학습64문서704링크 오류0, 검사기 회귀25Pass | 문서 검사를 게임 실행으로 기록하지 않음 |
| Unreal 컴파일 | 최종 통합·새 재현 Editor Pass; A/B canonical 반영 후 각각45.02/58.54초 Pass | [A](final-A-editor.json), [B](final-B-editor.json), [입력 동일성](final-role-build-summary.json) |
| Unreal 자동화 | 새 재현 e89 전체58종 경고0/실패0. f64 테스트 전용197줄 추가 후 통합·재현 Waves9종 모두 무경고 Pass | NullRHI 실제 객체 검사이며 GPU·실제 RPC와 구별 |
| GPU PIE | 새 재현 listen/client2World, 소환·Running·종료 위젯 재생성·기존4위젯 수거·구독1·종료명령0·설정 복원1Success | 명시 종료 fixture. 자연10웨이브 결과는 패키지 증거 |
| Win64 패키지 빌드 | 최종 e89 입력73파일 compile/cook/archive104.45초 Pass | EXE 해시·입력은 [final-package-inputs](final-package-inputs.json) |
| Win64 자연 실행 | 기존 패키지5시드 host82/client72·실제RPC62검사, 같은 두 프로세스5매치/각4회 실제 결과복귀.300ms/3%→회복 한 판19/17 Pass | 모두 보스 시간초과. 자동 전술이며 사람 승률 아님. 이전 입력을 최신 패키지 실행으로 승계하지 않음 |
| 최종 패키지 보충 | 경계179/86, 동시재료27/14, 진입/늦은참가/복귀35/29 모두Pass. 세대 재사용 결함의 실제 새 매치 재전송도 거절 확인 | [독립 기대값·실제 결과·화면·fixture](SUPPLEMENTS.md). 무음,Engine/Slate입력 |
| 대표 부하·수명 | 기존 패키지20분,40유닛/99일반/2보스;69/33 Pass,2000실제사망/fallback0/GC2141 | [정확한 입력·실측 환경·한계](PERFORMANCE.md). 제한60FPS, 같은PC2프로세스 |
| 실제 소리 | cooked 거절 효과음 로드 Pass | 청음 NotRun, 사용자 실행 요청 중 |
| Android | SDK/NDK/JDK 준비, 최초 실제 빌드 exit6. 9/28 재확인도 엔진Android파일 없음/adb0 | APK·설치·물리터치·SafeArea·10웨이브·성능 NotRun |

G3 시작 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`, 최종 제품·검사 소스 `f64cc671848560923595cc1955efe12620f326de`. 패키지 소스는 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`이며 이후 diff는 LDWaveTests.cpp만197줄이다. [학습의 실제66→73→테스트1파일 조립](REPLAY_EVIDENCE.md)을 실행했고 최종73개 blob과 Editor·선택9회귀까지 확인하여 그 PC 수업 범위만 Verified다. 실행 JSON의 detached HEAD f735만으로 입력을 추정하지 않는다. learn3브랜치는 최초8c6856d에 유지했다.

자연5판/회복/20분의 이전 EXE SHA256은 `4339F4E3C166D244DC77D780C4E26FBC423E30E7BDB0923C1E39628F5B2B6B54`, C++0e473f4/Config98727f0/후처리0981이다. 마지막 패키지는 `349F4660D6A0729807EA4FFD04DEE7A6D732001D00D830488EF00C42C049B177`이다. 제품 변경은 World를 넘어 유지되는 연결 세대 발급기53af399이고 전투·렌더링·자연 전술·데이터·맵·음향·cook 설정 변경은 없다. 세대 변경은 새 패키지4매치/3복귀로 검증했다. 변경 근거 없는20분/5판 반복 대신 각 입력의 증거를 그대로 연결했다.

실제 결함(종료 타격·보스HP 게시·Entry 스타일 수명·음향cook·세대 재사용), fixture/도구 결함(조립 LiveCoding·수명GC·Unityhelper·초기Slate레이아웃·비동기응답관찰)과 수정 전 실패 기록을 [리뷰](REVIEW_FINDINGS.md)에 보존한다. 전체 로그는 작업 폴더 Saved/P0Runs, 정식 작은 결과/선택 화면은 현재 폴더, 재현별 manifest는 learning/P0/evidence에 둔다. 필수 런타임은 Source/Content/Config에 있으며 learning을 로드하지 않는다.

## 최종 패키지 자연 규칙·네트워크 관찰

[5시드 결과](package-five-seeds-summary.json), [실행 조건·해시·실제 채택 시드](package-five-seeds-pair.json), [실제 RPC 전체 비교](package-five-seeds-wire.json). 왕복150ms를 요청하려고 endpoint별 송신75ms·손실1%를 설정했다. 실제 접속 게임 시간은 host1250.021초/client1252.044초로 각각10분 이상이다. 모든 판은10웨이브 보스 시간초과 패배였고 일반 잔여0, 양쪽 공용 상태·개체·재화는 서버와 일치했다. 전승은 P0 기준이 아니며 수치는 조정하지 않았다.

| 시드 | 최종 보스HP 0/1 | client 실제 echo 평균/p95(ms) |
|---|---|---|
|1776|435 / 3975|196.9 / 216.9|
|42|0 / 3332|198.0 / 216.6|
|1729|975 / 4114|197.7 / 217.1|
|2026|1659 / 3764|197.3 / 216.5|
|9001|153 / 3087|186.4 / 200.1|

첫 합성 World 시각·유효 피해·명령·재화·HUD 재생성은 결과 JSON에 판별로 보존했다. 자동 전술은 합성 가능한 뭉치를 먼저 합성하고 가용 골드를 소모하며, 매치마다 실제 판매/이동을 한 번 수행한다. 첫 진입은 양쪽 Entry의 실제 Slate 버튼을 눌렀고 네 번의 결과 복귀도 실제 버튼을 눌렀다. 이후 시드를 바꾸는 host는 `OpenLevel(listen?P0Seed=...)`, client는 `ClientTravel`로 다시 참가하므로 후속 참가 버튼을 네 번 더 누른 증거로 기록하지 않는다. RNG 시드 입력 외에 재화·HP·웨이브 시계는 변경하지 않았다. 사람의 숙련도·원작 밸런스 재현을 뜻하지 않는다. host/client의 최초 소환 응답은 각23/24회 실제 수신했고 매번 전체 응답 signature가 같았다. Result와 보드 변경 뒤 재전송도 원응답을 유지했다. [의도적으로 종료 후 SERVER 응답10개를 제거한 사본](wire-negative-check.json)은 이전 횟수 검사는 통과하면서 새 종료 검사를 정확히10개 실패시켰다. 원본 실행 로그는 변경하지 않았다.

[300ms/3% 회복 결과](package-recovery-summary.json), [실행 조건](package-recovery-pair.json), [실제 RPC 검증](package-recovery-wire.json). 각 송신150ms·손실3%를 적용한 후 host120.002초/client120.013초에 emulation을 해제했고, 약250초 게임을 완료하여 최종 상태 일치를 확인했다. client Pending 재시도17회. echo244표본 전체 평균161.5ms/p95349.6ms에는 손실 구간과 회복 구간이 함께 포함되며 시각 없는 개별 표본에서 구간별 p95를 추정하지 않는다. 호스트의 로컬 요청은 같은 지연을 거치지 않는다.

실제 확인 화면: [host 10웨이브](package-host-wave10.png), [client 10웨이브](package-client-wave10.png), [host 결과](package-host-result.png), [client 결과](package-client-result.png). 양쪽 개인 보드가 아래에 있고 웨이브/보스HP/패배 사유가 일치하며 소환·합성·판매는 결과에서 비활성이다. 무음 실행의 효과음 로드 Pass와 실제 청취 NotRun을 구분한다.
