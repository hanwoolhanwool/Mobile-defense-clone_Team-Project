# G0 B 소유 RPC와 canonical 종료 재현

2026-09-27의 **실제 GPU Editor PIE Pass**다. [B-01](../../B/G0-01-foundation.md)·[B-02](../../B/G0-02-commands.md)의 Verified는 아래 **기존 TopDown 맵 + native LDGameMode** 경로로 한정한다. 실제 학습자는 Planned이며 `learn/*` 브랜치에 참고 코드를 병합하지 않는다. 이 기록은 제공 검사의 작성 당시 README에 남은 NotRun 이후의 실행 결과다.

## 입력과 실행 순서

- B: 공통 출발 `8c6856d235de87cc28c12b49ca775bd0937334a5`에 제공 기반 `81ba665adff6c60ef95f79319b3115050937a1c1`과 독립 제품 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`를 조립했다. 원래 [27파일 manifest](b-assembly.json) 중26개가 그대로이며 Build.cs만 아래 Editor 의존을 추가했다.
- canonical: 별도 detached `649c1dedd6832c41089a76b59bc76518cd262296`에서 같은 검사를 실행했다. 독립 B의 GameMode에 종료 API를 추가하지 않았다.
- 제공 검사: `5f1f08606f87e264b5ff0a12d957dbc7720f7e6c`의 [LDG0PieTests.cpp](../../../tools/G0Replay/B/LDG0PieTests.cpp). 두 재현본과 제공 원본 SHA256은 `BC2181B6BFCE2D27BE5C79F32DEEE3C4D0361848FEA405C3A0A92A70345274FD`로 같다. 제품 작성 과제와 이 검사를 구별한다.

[공통 G0 설치·실행](../../../tools/G0Replay/README.md)과 [B 제공 검사 설치](../../../tools/G0Replay/B/README.md)를 따른다. B 수업 소스를 먼저 모두 조립한 뒤 제공 cpp를 대상 `Source/Mobile_defense_clone/Tests`에 복사한다. Build.cs의 `if (Target.bBuildEditor)` 안에 `UnrealEd`, `SlateCore`만 추가한다. canonical에서만 같은 블록에 `PrivateDefinitions.Add("LD_G0_CANONICAL_PIE=1");`을 추가한다. 제품 Core/Data/Network·에셋·규칙은 바꾸지 않는다.

Editor 빌드 후 제공 `learning/tools/Test-P0G0PIE.ps1`에 아래 대상·필터·새 RunId를 전달한다. 기존 결과 폴더는 덮어쓰지 않는다. GPU Editor가 필요하며 NullRHI로 실행하지 않는다. 빌드/Editor/포트는 다른 게임·성능 검사와 직렬로 사용한다.

| 대상 | 실제 ProjectRoot | 필터 | 실제 실행 RunId |
|---|---|---|---|
| B 독립 | `C:/Users/iam12/P0_lesson_replay_b` | `LD.PIE.G0.B.OwnedRPC` | `G0-actual-network-replay-B-pie` |
| canonical | `C:/Users/iam12/P0_lesson_replay_g0_integration` | `LD.PIE.G0.Canonical.TerminalCache` | `G0-actual-network-replay-canonical-pie` |

검사는 `/Game/TopDown/Lvl_TopDown`과 `Request.GameModeOverride=ALDGameMode::StaticClass()`를 지정한다. PIE는 ListenServer·2인·RunUnderOneProcess=true·요청 창540×720이다. 기존 맵의 World Settings나 Blueprint를 저장 변경하지 않는다. native 선택을 BP GameMode 에셋으로 바꾸는 실습은 **선택 과제/미검증**이다. G0 게임 HUD가 없으므로 판정 근거는 화면 완성도가 아니라 실제 두 World·서버/클라이언트 객체·RPC·종료 기록이다.

## 실제 결과와 독립 기대

[간결한 합계·경고·입력 해시](B-network-summary.json), [B Editor](B-native-editor.json)·[B PIE](B-native-pie.json)·[B 원본 proof](B-native-proof.json), [canonical Editor](canonical-native-editor.json)·[canonical PIE](canonical-native-pie.json)·[canonical 원본 proof](canonical-native-proof.json)를 함께 읽는다.

| 항목 | B 독립 실제 결과 | canonical 실제 결과 |
|---|---|---|
| Editor | 44.19초 Pass | 54.72초 Pass |
| PIE 자동화 | 1Success/0Fail, 경고 이벤트1개 | 1Success/0Fail, 경고 이벤트2개 |
| 준비·신원 | 서로 다른 PIE World, 같은 MatchId, Preparing, index0/1·epoch1/2·owner 문맥 복제 | 동일 기대 Pass |
| 최초 번호1 + 즉시 Retry | 실제 서버 요청2·클라이언트 PhaseNotAllowed2, 확정 완료1·cache1 | 동일 기대 Pass |
| 소유자 문맥 불일치 | 자기 RPC에 host epoch를 싣자 InvalidEpoch, 완료/캐시 불변; 비로컬 PC 의도 거절 | 동일 기대 Pass |
| 전송 유실/지연 응답 | 해당 client NetDriver만 outgoing PktLoss100 약0.4초, 번호2 pending 유지. 서버 RPC로 주입한 old-match701/old-epoch702는 무시하고 일치 Pending703만 수용 | 동일 기대 Pass |
| 손실 복원 후 Retry | 같은 번호2 실제 dispatch3, 새 확정 완료1·총cache2 | 동일 기대 Pass |
| 별도 응답 예산 고갈 | 번호2 재전송32회+새 번호3 실제33 dispatch, 서버 프레임1251~1252/0.013084초. 번호3 cache3 확정이지만 송신 시도0·pending 유지 | 프레임1402~1403/0.010569초, 같은 cache3·번호3 송신0 |
| 충전 후 번호3 Retry | 처음 저장한 PhaseNotAllowed 전달, cache3 유지·확정 완료 총3회 | 동일 기대 Pass |
| Mode 종료 후 실제 raw RPC | 독립 B의 범위 밖 | AbortMatch가 양쪽 Aborted로 복제. 같은 번호3 원응답 전체 동일, 변경 내용3은 RequestIdConflict, 새4는 PhaseNotAllowed. cache4·확정 완료3 |
| 전체 관측 개수 | 서버 요청40·클라이언트 응답19·서버 송신 시도19 | 서버 요청43·클라이언트 응답22·서버 송신 시도22 |
| 정리 | PIE World0, Play/해당 packet 설정/event/send observer 모두 복원 | 동일 기대 Pass |

응답 예산은 실제 서버 수신·캐시 확정 후 **송신 시도0**을 확인했다. 단순히 응답이 늦거나32회를 호출했다는 이유로 성공 처리하지 않았다. 전송 손실 단계와 응답 제한 단계는 별개다. old-match/epoch/Pending 응답은 서버의 실제 Client RPC를 사용하는 명시 fixture이고 제품 G0가 성공·Pending을 생성한 것이 아니다. 직접 `_Implementation` 호출은 사용하지 않았다. G0는 Board/Economy Stub 때문에 모든 정상 명령이 PhaseNotAllowed이며 소환 성공/재화 변경을 주장하지 않는다.

두 실행은 무경고 성공이 아니다. B Automation의 경고1개는 CrowdFollowing의 RecastNavMesh 미발견이다. canonical에는 기존 TopDown 직렬화 maxTiles128/실제48 불일치로 NavMesh를 재생성한 경고와 같은 CrowdFollowing 경고가 있다. 원본 보고서에는 각각1/2개 이벤트가 남아 있다. runner의 `Warnings=1`은 각 실행의 **경고가 있는 테스트 수**이며 이벤트 개수가 아니다. 게임 요구에 없는 맵 재저장이나 내비게이션 변경을 이 네트워크 검사에 섞지 않았다.

원본 전체 로그/보고서는 각 ProjectRoot의 `Saved/P0Runs/<실행 접두>-editor/{result.json,build.log}`, `-pie/{result.json,engine.log,report/index.json}`, `-pie-proof/pie-proof.json`에 있다. 위 JSON 복사와 원본 report/proof 해시는 요약에 보존한다. B의 실행 HEAD는 공통 출발점이며 실제 제품 입력은 조립 manifest의03acb67이다. 두 값을 혼동하지 않는다.

## 검증 한계와 다음 단계

Verified는 위 native G0 조립·Editor·기존 값 자동화와 실제 소유 RPC/종료 수명의 재현 범위다. 별도 OS 프로세스 패키지·cook·실제 보드/경제·게임 HUD·물리 터치·Android·대표 성능 검사는 아니다. 선택 Blueprint GameMode 연결도 미검증이다. 응답 큐/진행 중 중복의 서버 비동기 작업은 G0에 없으며 주입 Pending 수신과 실제 비동기 executor 검증을 구별한다. 다음 제품 단계는 G1의 좌표·두 화면·전체 셀 입력 검수다.
