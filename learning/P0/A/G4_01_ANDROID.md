# Android 패키지·실기기 검수 — P0 / A / G4-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-MOB-01; [빌드 규약](../../../docs/technical/BUILD_RUN.md), [P0 검수](../../../docs/BACKLOG_QA.md), [공통 계약](../COMMON.md) |
| 참고 자료 제작 상태 | Draft — Android 엔진 구성 요소 및 실기기 대기 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | f64cc671848560923595cc1955efe12620f326de / 미생성 |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | B의 보드 좌표·터치·SafeZone 및 G3 PC 통합; Schema2/Rules0.3.0 |
| 제공 코드 / 직접 작성할 코드 | 제공: JSON·맵·기존 패키징 도구. 직접 작성/설정: 아래 모바일 설정과 실기기에서 발견한 P0 결함 수정. 미검증을 완료 코드로 취급하지 않음 |

## 이번에 만들 동작

ARM64 Android 기기에 데이터가 포함된 APK를 설치하고, 같은 네트워크의 PC와 호스트/IPv4 참가로 한 매치를 진행한다. 자기 보드18칸의 표시와 물리 터치, 소환·뭉치 이동·합성·판매, 보스와 결과 복귀를 확인한다. PC의 입력 주입은 손가락 터치나 모바일 SafeArea의 증거가 아니다.

## 코드 작성 순서

1. G3 코드와 데이터/맵/설정을 고정한다. 필수 런타임 입력은 Source/Content/Config이며 learning 폴더를 읽지 않는다.
2. `Config/DefaultEngine.ini`의 기존 Android 섹션과 `tools/Build-P0Package.ps1`의 SDK/JDK 경로를 대조한다. 같은 키를 별도 섹션에 중복 추가하지 않는다.
3. `UI/LDG1BoardWidget`, `LDGameplayWidget`, `LDBattleStatusWidget`, `LDResultWidget`, `LDEntryWidget`의 native SafeZone에서 실제 레이아웃을 확인한다. `LDPlayerController`의 화면 좌표→보드 CellId 경로를 PC와 공유한다.
4. 실제 기기에서 생긴 문제만 원인 파일과 재현 순서로 고친다. 입력/표시 수정이 서버 좌표·경제·확률 원본을 바꾸지 않도록 B와 리뷰한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | Launcher → UE5.8 → 옵션 | Android 선택 구성 요소 설치 | 현재 엔진의 Android 플랫폼 빌드 파일 확보 |
| 2 | Project Settings → Platforms → Android | SDK36 / BuildTools36.0.0 / NDK27.2.12479018 / JDK21.0.3+9 | 실제 UAT가 선택한 경로와 버전을 로그로 확인 |
| 3 | Android 앱/ABI | MinSDK26, TargetSDK36, ARM64 true, x86_64 false, Portrait, PackageDataInsideApk true | 세로 설치형 개발 APK. ID=`com.luckyworkshop.defense.prototype` |
| 4 | Android 렌더러·쿠킹 | ES3.1 true, Vulkan false, ETC2; 현행 Forward/HDR/MSAA2 | 설치 기기의 실제 RHI·HDR·샘플 수를 로그로 확인; 설정 파일만으로 적용 성공 판단 금지 |
| 5 | Maps & Modes | 기본 `L_P0Entry`, 전투 `L_P0`, GameInstance=`LDGameInstance` | 시작 화면→매치→결과 복귀 |
| 6 | 각 native UMG RebuildWidget | SafeZone→Canvas→기존 v2 배치, 표시/입력 같은 실제 가용 영역 | 노치·시스템 바 아래로 핵심 버튼/보드가 가려지지 않음 |

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| SDK/NDK/JDK 준비 후 Android 빌드 | APK 생성 | UE Android 플랫폼 구성 요소 누락으로 exit6; APK 없음 | [공통 실행 기록](../../../docs/production/P0_REFERENCE_RUN.md); SDK 준비는 패키징 Pass가 아님 |
| 2026-09-28 02:23 KST 도구 재확인 | 엔진 플랫폼 파일/adb device | Android 플랫폼 디렉터리 없음, adb 연결0; [실제 점검](../../../docs/production/evidence/RUN-20260918-G3/android-latest-readiness.json) | 실기기 미실행 |
| 물리 터치·SafeArea·10웨이브 | 양쪽 상태 일치·조작/표시 정상 | NotRun | 기기 모델/OS/실측 뷰포트/이미지 미확보 |
| 초기30FPS·메모리·복귀 | 측정값과 정상/실패 원인 기록 | NotRun | PC 수치를 모바일 결과로 대체하지 않음 |

준비 후 통합 폴더에서 `tools/Build-P0Package.ps1 -Platform Android -RunId <새 이름>`을 실행하고 전체 로그와 새 APK SHA256을 보관한다. `adb devices -l`의 `device` 상태를 확인하고, 대상 기기를 명시하여 해당 APK를 설치한다. 기존 앱 데이터 삭제·기기 설정 초기화는 이 절차에 포함하지 않는다.

PC 호스트는 패키지 시작 화면에서 호스트를 누르고, Android는 같은 네트워크의 PC IPv4로 참가한다. 기기의 `127.0.0.1`은 PC 주소가 아니다. 두 보드 상태, 첫 구매·실패 구매·합성·판매·보충·드래그, 상대 보드 거절, 보스 시간과 결과를 기록한다. 물리 터치는 사용자가 수행하며 화면 캡처/녹화·입력 순서·기기 모델을 증거에 연결한다. 백그라운드/복귀 시 P0의 종료/복귀 정책을 확인하고 P1/P2 재접속 기능을 추가하지 않는다.

측정에는 APK/소스/데이터 SHA, 기기/OS/GPU, 전원·발열·화면밝기·화면비/노치, 실제 RHI, FPS 제한30과 워밍업/측정 길이를 남긴다. 프레임 평균/p95/최대, Game/Render/GPU, 메모리와 열 상태를 구분한다. 아직 실행되지 않았으므로 예상 화면은 기존 PC v2 배치를 유지하는 세로 화면이며 실제 이미지나 성능 수치는 없다.

## 상대에게 전달하고 통합하기

A는 APK·빌드 로그·기기별 재현/성능을, B는 실제 터치 좌표/선택/실패 UI 원인을 함께 검토한다. 수정 커밋이 생기면 통합 Editor/관련 회귀와 새 APK를 다시 검증한다. APK만 생성됐거나 자동 입력만 통과했을 때 G4 또는 이 수업을 Verified로 표시하지 않는다.

## 이해 확인

- SDK 파일 존재, APK 생성, 설치 성공, 물리 터치 완주는 각각 무엇을 증명하는가?
- SafeArea가 줄어들 때 서버 CellId를 바꾸면 왜 두 참가자 상태가 달라지는가?
- 작은 변형: 시스템 내비게이션 표시 방식을 바꾸고 동일 셀의 터치/투영 결과를 비교하라. 기기 설정은 검수 후 사용자의 원래 값으로 복원한다.

## 단계 완료

- [ ] Android 빌드·설치·실행과 완료 SHA/APK 해시를 연결했다.
- [ ] 실제 기기의 물리 터치·SafeArea·두 참가자·10웨이브·결과/복귀를 확인했다.
- [ ] 초기 성능·측정 조건·실패/수정 과정을 기록했다.
- [ ] 문서 시작점에서 재현한 후 참고 상태만 Verified로 변경했다.

외부 의존성은 UE Android 구성 요소 설치와 USB 디버깅을 허용한 실제 기기다. G4 필수 검수가 남는 동안 P0 최종 완료와 P1 진입을 보류한다.
