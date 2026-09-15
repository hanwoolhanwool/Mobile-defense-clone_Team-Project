---
id: TECH-CHECKLIST
version: 0.1.0
status: Draft
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-01
applies_to: UE 5.8.2 후보 / 플랫폼별 준비 현황
verified: 2026-09-13
verified_run: RUN-20260913-03
verification_scope: 기존 Windows 기본 빌드·실행 기록 대조; Android 제외
---

# 개발 시작 전 사전 준비·세팅 한눈에 보기

기준일: **2026-09-13** · [전체 과정·현재 상황](../production/PROJECT_STATUS.md) · [세부 환경 안내](DEVELOPMENT_SETUP.md) · [빌드 절차](BUILD_RUN.md)

**Windows 기본 개발·빌드·실행 준비는 완료했다. Android는 목표 설정을 정했으며 실제 도구 조합 적용·패키징·실기기 확인이 남아 있다.**

이 문서는 현재 개발 PC의 준비 현황과 새 환경에서 따라갈 순서를 요약한다. 버전은 기존 결정의 기준이며 자동으로 최신 버전을 뜻하지 않는다. 환경·설정의 원본은 개발 환경 안내, 실제 성공·실패의 원본은 [검수 기록](../production/TEST_RUNS.md)이다. 새 PC에서는 아래 완료 표시를 인계하지 않고 다시 확인한다.

## 1. 준비 상태 요약

| 준비 항목 | 상태 | 확인한 결과 또는 남은 일 |
|---|---|---|
| 프로젝트 출발점 | 완료 | 기존 프로젝트·모듈 유지, 템플릿 보존 |
| 작업 입력 보존 | 완료 | 입력 606개 복사·SHA-256 확인, 소스·콘텐츠 533개 불변 확인 |
| Windows 개발 도구 | 완료 | 선택한 VS·MSVC·SDK 적용 및 실제 UBT 선택 확인 |
| Windows 빌드·실행 | 완료 | Editor·패키징, 기본 맵의 클릭 이동·정상 종료 확인 |
| Android 개발 도구 | 일부 준비 | SDK 36 설치 확인. Studio·Build Tools·NDK·JDK는 목표 조합 적용·선택 검증 필요 |
| Android 프로젝트 설정 | 기준 결정 | 모바일 렌더링·화면·ABI·앱 설정 적용 및 결과 검증 대기 |
| Android APK | 대기 | 컴파일·쿠킹·패키징·APK 내용 검사 미실행 |
| Android 실기기 | 미정 | 모델·OS 미정, 마지막 확인 시 승인된 adb 연결 기기 0대 |
| 엔진 최종 버전 고정 | 대기 | UE 5.8.2 후보. PC·Android 빌드 모두 통과한 근거 필요 |
| 기능 구현 전 설계 | 일부 확인 필요 | 원작 보드·이동·합성·경제 규칙을 해당 구현 전에 확정 |

## 2. 도구 버전 표

| 도구 | 선택한 기준 | 현재 적용·관찰 및 검증 상태 |
|---|---|---|
| Unreal Engine | 5.8.2 후보 | 5.8.2 / CL 56702186, Windows 검증 완료 |
| Visual Studio | 2026 18.10.0 Stable | Community 18.10.12201.205 설치 완료, 재부팅 요구 없음 |
| MSVC | 14.50 계열, 실제 컴파일러 14.50.35723 이상 | 디렉터리 14.50.35717 / 실제 컴파일러 14.50.35738.0, UBT 사용 확인 |
| Windows SDK | 10.0.26100.0 | 설치·UBT 선택 확인 |
| .NET SDK | 10.0 계열 | 시스템 10.0.201 / UE 동봉 10.0.203, UBT는 동봉 SDK 사용 |
| PowerShell | 빌드 스크립트 실행에 7.2 이상 필요 | 현재 Windows 빌드 실행 완료. Windows PowerShell 5.1과 구분 |
| Android Studio | Koala 2024.1.2 Patch 1 | 현재 AI-252.27397.103.2522.14617522 관찰, 목표 조합 검증 대기 |
| Android SDK Platform | API 36 | android-36 설치 확인, UE 선택·빌드 검증 대기 |
| Android Build Tools | 36.0.0 | 현재 36.1.0 관찰, 목표 버전 적용 대기 |
| Android NDK | r27c / 27.2.12479018 | 현재 28.0.13004108·29.0.14206865 관찰, 목표 버전 적용 대기 |
| JDK | OpenJDK 21.0.3 | 현재 Studio 동봉 21.0.8 관찰, 목표 버전 적용 대기 |

선택 근거는 [DEC-009~011](../DECISIONS.md)과 [세부 환경 안내](DEVELOPMENT_SETUP.md)를 따른다. MSVC 디렉터리 이름과 실제 컴파일러 패치 번호가 다른 것은 확인된 정상 관찰이다. 기존 설치 항목의 존재만으로 프로젝트가 그 버전을 사용한다고 판정하지 않는다.

## 3. 프로젝트·플랫폼 설정 표

| 항목 | 적용 기준 | 현재 상태 |
|---|---|---|
| 프로젝트 | 기존 Mobile_defense_clone.uproject와 동일 이름 C++ 모듈 사용 | 유지 확인 |
| 코드·콘텐츠 구조 | 기존 모듈 안에 기능별 코드, 전용 콘텐츠는 Content/LD/ 아래 구성 | 기존 입력 보존. 디펜스 전용 구조는 후속 구현 |
| 기본 실행 맵 | 환경 검증은 /Game/TopDown/Lvl_TopDown | Windows 실행 확인. 디펜스 전장으로 전환은 후속 작업 |
| Windows 컴파일러 | Compiler=VisualStudio2026, CompilerVersion=14.50.35717 | Config 적용·실제 빌드 확인 |
| Windows SDK 선택 | WindowsSDKVersion=10.0.26100.0 | Config 적용·실제 빌드 확인 |
| 모바일 렌더링 | Mobile Forward·베이크 조명, Lumen·Nanite·VSM 사용 안 함 | 목표 결정, 적용·모바일 검증 대기 |
| 화면·입력 | 세로, UI 설계 기준 1080×1920, DPI·SafeArea 대응, 터치 | 목표 결정, 실제 화면·입력 검증 대기 |
| 개발 패키지 | Development APK, 게임 데이터 포함 | 목표 결정, 패키징 대기 |
| ABI·텍스처 | ARM64 단일 ABI, ETC2 단일 쿠킹 | 목표 결정, 패키징 결과 검사 대기 |
| 그래픽 | OpenGL ES 3.2, Vulkan Off, Mobile HDR On, MSAA 2x | 목표 결정, 실제 적용 검증 대기 |
| Android SDK 값 | 최소 API 26, Target API 36 | 목표 결정, 생성 APK 검사 대기 |
| 앱 ID | com.luckyworkshop.defense.prototype | 개발용 기준 결정, APK 검사 대기 |
| 앱 표시 이름 | 행운공방 디펜스 (개발) | 개발용 기준 결정, 기기 표시 검사 대기 |

모바일 항목은 선택한 설정이다. 현재 Windows 검증이 해당 설정의 적용·기기 호환성을 보장하지 않는다. 구체적인 Config 키·병합 방법과 검사 명령은 [빌드 절차](BUILD_RUN.md)에 있다.

## 4. 새 환경에서 준비하는 순서

1. **입력 확보:** 프로젝트 파일, Source·Config·Content 및 필요한 Build·Plugins를 확보한다. 미커밋·미추적·Git에서 제외된 필수 에셋도 확인하고 복사·해시 증거를 남긴다.
2. **Windows 도구 설치:** 위 기준의 UE와 VS C++ 데스크톱·게임 개발 구성 요소, MSVC·Windows SDK를 준비한다. 실제 버전과 설치 경로를 기록한다.
3. **Windows 설정·빌드:** Config의 컴파일러·SDK 값을 맞추고 아래 스크립트로 프로젝트 생성·Editor 빌드·Windows 패키징을 수행한다.
4. **기본 실행 확인:** Editor의 PIE 시작·클릭 이동·중지, 패키징 게임의 표시·클릭 이동·종료를 직접 확인한다.
5. **Android 도구·설정:** 선택한 SDK·NDK·JDK 경로와 버전을 맞추고 위 모바일 설정을 적용한다. 실제 UE 로그의 선택값을 확인한다.
6. **Android 패키징:** Development·ARM64·ETC2 APK를 만들고 크기·해시·ABI·SDK·앱 식별 정보를 기록한다.
7. **실기기 확인:** 모델·OS·GPU·메모리·ABI를 기록하고 USB 디버깅 연결 승인 후 설치·실행·터치·화면 방향·SafeArea를 확인한다.
8. **결과 반영:** 성공·실패와 로그를 검수 기록에 남긴다. 양 플랫폼 빌드 결과를 근거로 엔진 최종 고정 여부를 판단한다.

현재 PC는 1~4번의 Windows 기본 검증을 마쳤다. 한국어 엔진 시작 검사 문제는 아래 미해결 항목으로 남아 있다.

## 5. 바로 사용하는 Windows 빌드 명령

PowerShell 7.2 이상에서 프로젝트 루트로 이동한 뒤 실행한다. 스크립트는 별도 입력 복사본을 만들어 빌드한다.

```powershell
Set-Location 'C:\Users\iam12\Mobile_defense_clone'
pwsh -NoProfile -File .\tools\Invoke-PrototypeBuild.ps1 -Stage Windows -CompilerVersion 14.50.35717
```

엔진 기본 경로는 `C:\Program Files\Epic Games\UE_5.8`이며 다른 경로는 `-EngineRoot`로 지정한다. 산출물과 로그는 `Saved/BuildRuns/<실행 시각>/`에 생성된다. 패키지를 전달할 때는 실행 파일만 추출하지 않고 패키지 폴더 전체를 전달한다.

문서 참조·형식 검사는 Node.js가 준비된 환경에서 `node tools/validate-planning.mjs`로 실행한다. 문서 검사가 게임 실행 검수를 대신하지 않는다.

## 6. 남은 준비와 알려진 문제

- **Android:** 목표 도구 조합 적용, 프로젝트 모바일 설정, APK 패키징·검사, 실기기 검증이 남아 있다. 기기 미정은 도구·패키징 준비를 막지 않는다.
- **엔진 시작 검사:** 동일 진단 조건에서 한국어 오류 15건·영어 0건을 관찰했다. 언어 의존 엔진 테스트 문제로 기록했으며 엔진 코드를 수정하거나 해결 완료로 처리하지 않았다.
- **기능별 원작 확인:** Android 2.0.11 보통 모드 기준의 보드·조작·경제 확인은 해당 기능 구현의 선행 조건이다.
- **완료 범위:** 기본 맵 실행과 세팅 완료 후에도 PC 2인 상태 일치·중복 소비·승패 경계 및 Android 10웨이브 터치 완주의 P0 검수가 필요하다.

근거: [작업 보드의 TASK-CORE-01](../production/BOARD.md), [환경·빌드 기록](../production/evidence/RUN-20260913-02.txt), [산출물·실행 기록](../production/evidence/RUN-20260913-03.txt), [원작 대조 기록](../product/ORIGINAL_REFERENCE.md).
