# G3 새 시작점 재현 증거

참고 제작은 아래 완료한 PC 재현 범위 Verified, 학습자 Planned. 실제 청음·Android·선택 변형은 미검증이다. G2 기준 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`의 새 detached `C:/Users/iam12/P0_lesson_replay_g3`에 역할 수업 순서로 파일을 조립했다. [수업 절차](../../G3_INTEGRATION.md).

- [최초65파일 조립](assembly.json): 런타임 기준 `de6e2f62f94660161f5863013ea3353f7a3a1e92`.
- [최종66파일 대조](final-inputs.json): 검증 입력 `5359cda03b241537e0561c16b5026e18061181aa`. Source/Content/Config는 최초 조립과 동일하다.
- [패키징 도구 수정](package-tool-amendment.json): 첫 패키징이 기존 다른 에디터의 Live Coding mutex로 exit6. 사용자 에디터를 보존하고 제공 도구에 UBT `-NoHotReloadFromIDE -MaxParallelActions=4`를 지정했다. 읽기 전용 CSV 분석기도 제공 입력으로 추가했다. 현재 재현 도구는 이 두 파일을 포함한66개를 처음부터 조립한다.
- [Editor](editor.json), [56종 자동화](automation.json), [실제 PIE](pie.json), [PIE 수명 증거](pie-proof.json), [G2 조작 회귀](g2-regression.json): Pass. 자동화56종 경고0, PIE1종/원설정 복원, G2 실제 host213/client57검사 통과.

재현 HEAD는 출발점에 유지되므로 각 실행 JSON의 HEAD만으로 최종 소스를 판정하지 않는다. 위 최종 입력 manifest가 실행 파일의 코드/에셋 입력을 식별한다. 빌드·NullRHI 자동화·GPU PIE·별도 프로세스는 서로 다른 증거다.

PIE에서 양쪽546×720 화면의 WAVE1/20초/일반2와 gold80/pop1, 자기 보드 아래를 직접 확인했다. [host](host-running.png) · [client](client-running.png). 창 테두리를 포함한 실제 크기이며 요청한540×1170 그대로라고 기록하지 않는다.

위 초기 단계의 Pass는 당시 패키지·반복 자연 규칙 플레이·지연/회복·20분 부하·Android를 포함하지 않았다. 전체 로그/PNG는 재현 폴더 Saved/P0Runs/Replay-G3-*에 보존한다. 과거 패키징 실패 로그는 `Replay-G3-package`, 수정 후 첫 빌드 시도는 `Replay-G3-package-fix1`이다. 이후 실제 결과는 아래에 별도로 연결한다.

후속 cooked 실행에서 Entry의 임시 스타일 수명과 효과음 cook 누락을 실제 발견했다. [Entry 수정](entry-style-amendment.json) 후 [66파일 대조](entry-fixed-inputs.json), [Editor](entry-fixed-editor.json) 및 [관련3자동화](entry-fixed-automation.json) Pass. [효과음 cook 수정](audio-cook-amendment.json)까지 포함한 최종 입력은 `98727f04e7c563a854a103ad26152cff5ed652a6`이며 [정식66파일 manifest](../../../../docs/production/evidence/RUN-20260918-G3/package-final-inputs.json)와 [패키지 빌드](../../../../docs/production/evidence/RUN-20260918-G3/package-audio-fixed-build.json)에 연결한다. 기존 입력·실패 JSON은 덮어쓰지 않았다. C++는0e473f4의 B 전체57무경고 검수와 동일하며, 이 재현 폴더는56개 전체 검사 뒤 변경된 Entry3개를 별도로 실행했다.

이 패키지의 [5시드2인](../../../../docs/production/evidence/RUN-20260918-G3/package-five-seeds-summary.json), [300ms/3% 손실 후 회복](../../../../docs/production/evidence/RUN-20260918-G3/package-recovery-summary.json), 실제 중복/종료 RPC와 양쪽 화면은 [정식 검수 요약](../../../../docs/production/evidence/RUN-20260918-G3/SUMMARY.md)에 모았다. 학습자는 Planned이며 자연 규칙의 자동 플레이를 학습자의 직접 플레이로 기록하지 않는다.20분 부하와 G4는 각각 별도 판정한다.

후처리 검사 보강 후 [최종66파일 대조](audited-final-inputs.json)의 SourceSha는 `0981d07307112857dfdf0e91c79bcecdbcbc291b`다. 패키징 이후 바뀐 것은 `Test-P0G3Evidence.ps1`과 `Analyze-P0Load.py` 두 읽기 분석 도구뿐이며 런타임 C++·데이터·맵·Config는 위 패키지와 같다. 강화한 RPC 검사는 실제 두 run에서 통과했고 종료 후 응답을 제거한 로그 사본을 실패로 판정했다. Source0981의 새 조립 사전검사도50변경경로/66명시파일을 확인했으며 새 디렉터리를 만들지 않았다.

## 2026-09-28 최종 재현

역사66파일 입력을 보존한 뒤 [73파일 조립](supplement-inputs.json)으로 e89a1fabaf5ef5e3a1d03a09397806814551ec20을 대조했다. [Editor](final-editor.json), [58종 무경고 자동화](final-58.json), [실제 GPU PIE](final-pie.json)·[위젯 수명](final-pie-proof.json), [Win64 Development compile/cook/archive](final-package.json)가 모두 Pass다. 패키징은104.45초였고 APK 검사는 아니다. [host 종료 재생성](final-host-terminal.png)·[client 종료 재생성](final-client-terminal.png)은546×720 실제 관찰이며 의도한 InitFailure 종료 fixture다.

이 패키지의 실제 별도2프로세스 경계·동시 요청·늦은 참가/복귀도 전부 Pass였다. [정식 요약](../../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md)은 EXE 해시, 양쪽 검사, 실제 화면 및 fixture 한계를 구분한다. 초기 패키지와 실패 실행을 덮어쓰지 않았다.

마지막에는 LDWaveTests.cpp만 f64cc671848560923595cc1955efe12620f326de로 바꿨다. 이전73파일이 e89와 일치함을 먼저 확인하고 기존 파일은 Saved/P0Runs/Replay-G3-final-expectation-amendment/LDWaveTests.before.cpp에 보존했다. [최종73개 blob](final-detail-inputs.json), [Editor](final-detail-editor.json), [Waves9종 무경고 Pass](final-detail-waves.json)로 문서의 마지막 보충 절차를 확인했다. 패키지 제품·검수 fixture·설정·데이터·맵은 변경0이므로 이유 없이 재패키징/전체58검사를 반복하지 않았다.

Verified는 역사 도구0981의66파일 조립→e89의73파일 보충→f64의한 테스트 파일 보충 경로와 위 실제 실행에 한정한다. 현재 Replay-P0G3.ps1로73개를 처음부터 조립하는 경로는 사전검사만 했으며 별도의 새 전체 실행 증거로 주장하지 않는다. 실제 학습자의 작성/진도/SHA는 미생성이다.
