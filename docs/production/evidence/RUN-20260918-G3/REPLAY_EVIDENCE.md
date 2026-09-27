# 학습 절차의 실제 재현 — 정식 검수 사본

선택 학습 자료를 제거해도 작업 완료의 실행 근거가 남도록 필요한 원본 결과를 이 폴더에 복사했다. [사본 목록과 SHA256](replay-evidence/index.json)의 모든 파일은 원본과 바이트가 같다. 새로운 실행이 아니며 기존 결과의 입력·시각·범위를 바꾸지 않았다. 수업과 직접 작성 절차는 선택 자료 `learning/P0/G3_INTEGRATION.md`에서 관리한다.

공통 출발8c6856d에서 G0→G1→G2를 통과한 뒤, G3 재현 폴더 `C:/Users/iam12/P0_lesson_replay_g3`는 G2의 detached HEAD `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`에 유지했다. 따라서 실행 JSON의 HEAD와 실제 파일 입력은 다르며 아래 manifest를 함께 확인한다. 완성 코드를 learn 브랜치에 병합하지 않았다.

| 단계 | 실제 입력·실행 근거 | 결과 |
|---|---|---|
| 초기 조립 | 역사0981 도구의66파일. 이전 실패·수정 이력은 SUMMARY와 REVIEW_FINDINGS에 보존 | 초기66개 해시 불변 확인 뒤 보충 진행 |
| 보충73개 | [e89 입력](replay-evidence/G3_REPLAY-supplement-inputs.json), 이전 변경6개 사본 보존·새7개 추가 | 전부 Git blob 일치 |
| 새 Editor·자동화 | [Editor](replay-evidence/G3_REPLAY-final-editor.json), [LD.P0](replay-evidence/G3_REPLAY-final-58.json) | Pass / 58 Success, 경고·Fail·NotRun0 |
| 실제 GPU PIE | [실행](replay-evidence/G3_REPLAY-final-pie.json), [수명·설정 복원](replay-evidence/G3_REPLAY-final-pie-proof.json) | 1Success, 종료 UI 재생성·구독/수거·서버 명령0 |
| Win64 패키징 | [compile/cook/archive](replay-evidence/G3_REPLAY-final-package.json) | Pass,104.45초. 이후 실제2프로세스는 [보충 검수](SUPPLEMENTS.md) |
| 마지막 테스트 보강 | [f64 최종73개](replay-evidence/G3_REPLAY-final-detail-inputs.json), [Editor](replay-evidence/G3_REPLAY-final-detail-editor.json), [Waves9종](replay-evidence/G3_REPLAY-final-detail-waves.json) | 전부 일치·무경고 Pass. 변경은 LDWaveTests.cpp197줄뿐 |

최종 제품·제공 검사 입력은 `f64cc671848560923595cc1955efe12620f326de`, 패키지 입력은 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`이다. 제품·검수 fixture·데이터·맵·cook·runner 변경 없이 테스트 기대값만 보강했으므로 해당9종을 재실행했다. 같은 등록9개를 전체58개에 다시 더해67종으로 세지 않는다.

PIE의 [host](replay-evidence/G3_REPLAY-final-host-terminal.png)·[client](replay-evidence/G3_REPLAY-final-client-terminal.png)는 실제546×720의 명시 종료 fixture다. 자연10웨이브·물리 입력·실제 청음·Android 결과로 확대하지 않는다.

실제로 재현한 것은66→73→테스트1파일 보충 경로다. 현재73개 일괄 조립 도구의 새 경로는 사전검사만 했으며 별도의 전체 재실행으로 주장하지 않는다. 문서화한 PC 범위만 참고 Verified이고 실제 학습자는 Planned다. G4·청음 NotRun으로 P0는 최종 미완료다.
