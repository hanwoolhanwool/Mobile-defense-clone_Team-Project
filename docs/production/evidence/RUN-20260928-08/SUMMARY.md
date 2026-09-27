# P0 등급 팔레트 표시 후속

사용자 요청의 네 등급 식별성 개선을 PC에서 확인했다. 색상 원본은 [아트 명세](../../../art/ART_DIRECTION.md#p0-grade-palette)에서 관리한다. 시작 `f50710adbac9f813f88ef1d3be9cb6256bfd12f6` → 코드 완료 `65b7feebceb3b2aa94e1b191a010038706369d79`. 런타임 변경은 `Source/Mobile_defense_clone/Battle/LDUnitActor.cpp::InitializePrepared` 한 곳이다.

## 입력과 빌드

- A Editor: 최초 UBA 공유 메모리 생성 실패(exit9887)를 [원본 결과](editor-a-first.json)·[로그](editor-a-first.log)에 보존했다. `-NoUBA -MaxParallelActions=1`로 재시도하여 [Pass](editor-a-pass.json), 54.84초. 해당 실행은 코드 커밋 전이며 변경 파일 SHA256이 완료 커밋과 같다. UBA coordinator 표시는 남지만 각 작업은 NoUba로 실행되었다. 소스 컴파일 오류를 수정한 사례가 아니다.
- 통합 Editor: 사용자 저장·종료 확인 후 같은 코드로 빌드 [Pass](editor-integration.json), 28.41초. [로그](editor-integration.log).
- Win64 Development: compile/cook/stage/pak/archive [Pass](package.json), 169.33초. [로그](package.log). 기존 맵·재질·데이터를 새 패키지에 포함했다.
- 재현 경로 `C:/Users/iam12/P0_lesson_replay_g3`의 detached HEAD는 `f735b58`이며 실행 입력 SHA와 다르다. 앞선 f64 입력73파일의 해시를 확인하고 원래 actor를 Saved에 백업한 뒤 색상 파일만 교체했다. [73파일 manifest](input-manifest.json)의 모든 blob이 `65b7fee`와 일치한다. 과거 수업 증거와 시작점은 보존했다.

## 실제 PC 패키지 화면과 실행

실행: 2026-09-28 03:38:22~03:40:33 KST. [두 프로세스 기록](pair.json), [서버69검사](host.json)·[클라이언트33검사](client.json) 모두 Pass·실패0·정상 종료. pair의 Source는 재현 worktree HEAD이므로 위 manifest와 package 입력을 함께 확인한다. 실행 EXE SHA256: `E6245E9A04548795178571148C310806C2C04C341788DDA6015740D517187674`.

독립 관찰 기대는 네 등급이 서로 다른 색으로 보이고 서버·상대 화면에서 같은 팔레트를 쓰는 것이다. 원본540×1170 [서버 화면](host.png)과 [클라이언트 화면](client.png)에서 일반은 중성 회색, 희귀는 청록, 영웅은 밝은 보라, 전설은 노랑 계열 금색으로 구분되었다. 양쪽의 위·아래 보드에서 같은 색 조합을 확인했다. 재질 입력 HEX가 화면 픽셀과 같다는 주장은 하지 않는다. 캡처의 CsvProfiler 표시는 제공 검수 도구의 표시다.

기존 G3Load fixture로 16종·40유닛·99일반 적·2보스를 준비하고 `LoadSeconds=10`으로 화면을 확보했다. 이어지는 기존2000사망/25묶음 검사는 실제 공격으로 완료했고 강제 피해 사망0·중복 사망0, 종료 후 actor/전투 등록0을 확인했다. 무료 준비·높은 HP·고정 웨이브는 명시 검수 입력이다. 화면의 WAVE10은 자연 플레이로 도달했다는 뜻이 아니다.

재현 절차:

1. 완료 소스의 프로젝트를 사용한다. 새 worktree에서 Editor를 빌드하고 `package.json`의 UAT 인자에서 프로젝트/새 archive 경로를 맞춰 패키징한다. 기존 증거 폴더를 덮어쓰지 않는다.
2. 해당 프로젝트에서 `tools/Run-P0Load.ps1 -RunId <새이름> -GameExecutable <새패키지EXE> -LoadSeconds 10 -Port 18777 -RenderOffscreen`을 실행한다. 포트가 비어 있어야 하며 두 프로세스 종료까지 기다린다.
3. Saved/P0Runs의 pair/result JSON과 양쪽 representative-load.png를 위 기대와 대조한다. 화면 판정은 자동 검사 수에 포함되지 않는 직접 관찰이다. 원본 실행 로그·CSV는 `C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/P0-grade-palette-visual`에 보존했다.

사용할 새 패키지: `C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/P0-grade-palette-package/Package/Windows/Mobile_defense_clone.exe`. 통합 Editor 프로젝트: `C:/Users/iam12/P0_reference_integration/Mobile_defense_clone.uproject`.

## 변경 리뷰와 범위

구현 후 diff·호출 경로를 별도 리뷰 관점으로 대조했다. `InitializePrepared`의 기존 서버 권한 검사 안에서 색을 한 번 변환한다. 기존 `UnitColor` 복제→`RefreshPresentation`→동적 재질 Color 경로를 유지하고 보드·경제·공격 상태 원본에 접근을 추가하지 않는다. ARCH-01~06 관점에서 새 서비스 의존·구독·타이머·수명 소유자는 없다. 낮은 영향의 표시 매핑에 새 추상화를 추가하지 않았으며 수정이 필요한 중대한 결함은 발견하지 않았다. 독립 에이전트 리뷰로 표기하지 않는다.

문서·데이터·스타일·학습 링크는 [현재 검사](checks.json)로 구분한다. 이번에 PIE·전체 Unreal 자동화·자연 반복10웨이브·실제 청음·Android 실기기를 다시 실행하지 않았다. QA-VIS-02의 전체 화면비/터치 검수 통과를 뜻하지 않는다. G4·청음 대기와 P0 전체 미완료 상태는 유지한다. 추가 문양·돌기·색각별 식별성은 이번 색상 변경의 검증 범위 밖이다.

이번 짧은 실행의 프레임 p95는 서버17.680ms/클라이언트17.110ms, 측정 창 약12초다. Windows11/Ryzen5 7500F/RAM32GiB/RTX4060Ti/UE5.8.2, 같은 PC 두 GPU 프로세스, 각540×1170·60FPS 제한·VSync0·RenderOffscreen 조건이다. 프레임 제한·초기 렌더링·fixture 비용을 포함하므로 대표 성능/병목 개선 또는 회귀 판정에 쓰지 않는다. 이전20분 대표 부하와 Android 성능을 이 소스의 측정으로 대체하지 않는다.

[증거 해시](evidence-hashes.json)는 이 폴더에 복사한 원본 결과·로그·PNG의 무결성 확인용이다. 참고 자료 수정과 실제 학습자 Planned 상태를 분리한다.
