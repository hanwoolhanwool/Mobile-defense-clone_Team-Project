# G1 A 역할 검증 증거

검증 코드/실행 HEAD는 `bce4b7b0abe7787e1efe54e9af8805612bdffe5d`, 출발점은 `4787bf1a3a0d866aa206d148586594b3c710f957`이다. 통합 담당자가 빌드·에디터를 직렬 실행했고 A가 각 Saved/P0Runs 결과 JSON과 로그를 직접 읽어 확인했다.

| 검증 범위 | 결과 | 증거 |
|---|---|---|
| A Editor compile/UHT/link | Pass,exit0,20.25초 | [결과](EDITOR_RESULT.json), [전체 빌드 로그](EDITOR_BUILD.log) |
| Unreal NullRHI `LD.P0.G1.Route` | 5Pass/0Fail/0NotRun,exit0 | [결과](TEST_RESULT.json), [이벤트 발췌](TEST_EVENTS.log) |
| 문서·데이터·C++서식 | Pass, exit0; 기획6651·데이터2689·서식24개 오류0 | [전체 검사 로그](PROJECT_CHECK.log) |
| 두 프로세스 화면·실제네트워크·18셀입력·화면비 | NotRun | 통합 G1 실행 예정 |
| Android실기기·게임성능·학습재현 | NotRun | 이 역할 실행에 포함하지 않음 |

환경: Windows, UE5.8, Win64 Development `Mobile_defense_cloneEditor`, MSVC14.50.35738, Windows SDK10.0.26100.0, 빌드 병렬4. 자동화는 `-unattended -NullRHI -nosound -nosplash -culture=en`. 전체 원본은 `C:/Users/iam12/P0_reference_a/Saved/P0Runs/G1-A-editor/`와 `G1-A-tests/`에 있으며 후자의 `engine.log`, `report/index.json`을 보존했다. 20.25초는 증분 컴파일 시간이며 게임 성능 측정이 아니다.

학습 폴더의 빌드 로그 사본은 엔진이 출력한 줄끝 공백만 제거했다. 원본 Saved 로그는 그대로 보존했으며 실행 내용·결과를 수정하지 않았다.

테스트 통과 범위는 코너/정확한1·2바퀴/6440cm의 독립 좌표, 잘못된 입력의 출력 불변, 100ms 표시 예측·250ms 상한·정지, transient world의 동일 actor/ID/RouteIndex·2바퀴/stop·종료 경계, local view의 canonical 원본 불변 및 simulated proxy 권한 거절이다. 실제 네트워크와 화면 정합은 이 결과로 통과 처리하지 않는다. 참고 수업 Draft/실제 학습 Planned를 유지한다.
