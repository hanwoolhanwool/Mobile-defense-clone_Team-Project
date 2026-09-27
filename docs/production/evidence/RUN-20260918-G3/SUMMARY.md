# G3 이번 구현·검수 요약

실행일2026-09-27. G3 **InProgress**, P0 최종 미완료. 실제 학습자는 Planned. [공통 작업 기록](../../P0_REFERENCE_RUN.md), [독립 기대값](REVIEW_PLAN.md), [결함·구조 리뷰](REVIEW_FINDINGS.md), [새 조립 입력](replay-inputs.json).

| 실행 층 | 확인한 결과 | 남은 범위 |
|---|---|---|
| 문서·데이터·스타일 | 최근검사Pass,스타일77파일/오류0,학습57문서479링크/오류0 | 후속 수정 뒤 관련 검사 갱신 |
| Unreal 컴파일 | 통합·새 재현 Editor,최종 A/B 역할 Editor Pass | 최종 실행 입력 일치 확인 |
| Unreal 자동화 | 0e473f4 전체57개/경고0/실패0,실제 객체의 시각·소유권·종료·스타일 수명 | 실제 패키지·네트워크·물리 입력과 별개 |
| GPU PIE | 새 재현 listen/client2World,각 소환1·gold80,Running,종료 후 World0·설정 복원 Pass | 실제10웨이브 결과 검수 아님 |
| Editor-game2프로세스 | G2 회귀host213/client57. 부하smoke host69/client33 및 실제2000사망·fallback0·GC2141 Pass |20분 대표 패키지 성능 아님 |
| Win64 패키지 빌드 | 최초210.94초, Entry 수정132.82초, 필수 음향 cook 수정34.77초 compile/cook/archive Pass | 최종 패키지 해시를 각 실행 증거에 고정 |
| Win64 패키지 실행 | 5시드 host82/client72·실제RPC로그62 Pass. 같은 두 프로세스에서 각4회 결과 복귀·5개의 새 매치. 300ms/3% 손실→120초 회복 한 판 host19/client17 및 RPC로그 Pass | 대표20분 부하 진행. 첫 시작화면 크래시와 다음 효과음 cook 누락 Fail은 보존, 수정 후 닫음 |
| Android | 도구 준비, 실제 첫 빌드 exit6 | UE Android 선택 구성 요소와 실기기 필요, 설치/물리 터치/10웨이브/성능 NotRun |

입력: G2 시작 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`. 새 재현은 이 HEAD에 원본 파일들을 조립했으므로 실행 JSON의 HEAD만으로 런타임 소스를 추정하지 않는다. 최초65파일de6e2f6→제공 도구 포함66파일5359cda→Entry 수명 수정0e473f4→필수 효과음 cook 설정98727f0를 별도 manifest/수정 기록으로 연결한다. [최종 패키지 입력66개](package-final-inputs.json)의 C++는0e473f4, Config는98727f0다. 기존 조립·패키지·실패 입력은 덮어쓰지 않는다.

주요 수정은 종료 관찰 중 다음 공격 차단(A01), 이미 반영된 보스HP를 종료 snapshot에 반영(A02), 검수 fixture의 두 참가자 준비 대기, 실제 World에서 증거 경로 확보, cooked Entry의 임시 스타일 참조 제거(PKG01)다. 기대값·원인·수정·실패/후속 증거는 리뷰 문서가 원본이다.

전체 실행 로그는 각 작업 폴더 Saved/P0Runs에, 선택한 작은 결과·manifest·화면은 정식 검수와 학습 재현 증거 폴더에 보존한다. 부하 fixture와 정상 규칙 자동 플레이를 구분하고 실제 학습자/사람의 플레이로 기록하지 않는다. 측정 환경은 공통 기록을 따른다. 최종 패키지 성능 수치는 아직 없다.

## 최종 패키지 자연 규칙·네트워크 관찰

[5시드 결과](package-five-seeds-summary.json), [실행 조건·해시·실제 채택 시드](package-five-seeds-pair.json), [실제 RPC 전체 비교](package-five-seeds-wire.json). 왕복150ms를 요청하려고 endpoint별 송신75ms·손실1%를 설정했다. 실제 접속 게임 시간은 host1250.021초/client1252.044초로 각각10분 이상이다. 모든 판은10웨이브 보스 시간초과 패배였고 일반 잔여0, 양쪽 공용 상태·개체·재화는 서버와 일치했다. 전승은 P0 기준이 아니며 수치는 조정하지 않았다.

| 시드 | 최종 보스HP 0/1 | client 실제 echo 평균/p95(ms) |
|---|---|---|
|1776|435 / 3975|196.9 / 216.9|
|42|0 / 3332|198.0 / 216.6|
|1729|975 / 4114|197.7 / 217.1|
|2026|1659 / 3764|197.3 / 216.5|
|9001|153 / 3087|186.4 / 200.1|

첫 합성 World 시각·유효 피해·명령·재화·HUD 재생성은 결과 JSON에 판별로 보존했다. 자동 전술은 합성 가능한 뭉치를 먼저 합성하고 가용 골드를 소모하며, 매치마다 실제 판매/이동을 한 번 수행한다. 사람의 숙련도·원작 밸런스 재현을 뜻하지 않는다. host/client의 최초 소환 응답은 각23/24회 실제 수신했고 매번 전체 응답 signature가 같았다. Result와 보드 변경 뒤 재전송도 원응답을 유지했다. [의도적으로 종료 후 SERVER 응답10개를 제거한 사본](wire-negative-check.json)은 이전 횟수 검사는 통과하면서 새 종료 검사를 정확히10개 실패시켰다. 원본 실행 로그는 변경하지 않았다.

[300ms/3% 회복 결과](package-recovery-summary.json), [실행 조건](package-recovery-pair.json), [실제 RPC 검증](package-recovery-wire.json). 각 송신150ms·손실3%를 적용한 후 host120.002초/client120.013초에 emulation을 해제했고, 약250초 게임을 완료하여 최종 상태 일치를 확인했다. client Pending 재시도17회. echo244표본 전체 평균161.5ms/p95349.6ms에는 손실 구간과 회복 구간이 함께 포함되며 시각 없는 개별 표본에서 구간별 p95를 추정하지 않는다. 호스트의 로컬 요청은 같은 지연을 거치지 않는다.

실제 확인 화면: [host 10웨이브](package-host-wave10.png), [client 10웨이브](package-client-wave10.png), [host 결과](package-host-result.png), [client 결과](package-client-result.png). 양쪽 개인 보드가 아래에 있고 웨이브/보스HP/패배 사유가 일치하며 소환·합성·판매는 결과에서 비활성이다. 무음 실행의 효과음 로드 Pass와 실제 청취 NotRun을 구분한다.
