---
id: TECH-CONSOLE
version: 0.1.0
status: Draft
owner: unassigned
updated: 2026-09-13
---

# 언리얼 내부 콘솔 주요 명령어

[기획 허브](../README.md) · [개발 환경](DEVELOPMENT_SETUP.md) · [PowerShell 빌드·실행](BUILD_RUN.md)

**이 문서의 명령은 언리얼 에디터 또는 실행 중인 게임의 콘솔에 입력합니다. PowerShell·명령 프롬프트·Python 콘솔용 명령이 아닙니다.** 한 줄씩 입력하고 Enter를 누릅니다.

프로젝트의 UE 5.8 설정, 설치된 UE 5.8.2 소스와 Epic 공식 문서를 기준으로 정리했습니다. 명령을 실제 실행해 검증한 기록은 아니며, MCP 서버의 현재 실행 여부나 Auto Start Server 적용 여부를 뜻하지 않습니다.

## 1. 콘솔을 여는 방법

| 입력 위치 | 여는 방법 | 주로 쓸 때 |
|---|---|---|
| 에디터 하단 콘솔 | 하단의 콘솔 입력란에서 입력 모드를 **Cmd**로 선택 | MCP 서버 시작·중지, 콘솔 변수 조회 |
| Output Log / 출력 로그 | 에디터 하단 **Output Log**를 열고 아래 명령 입력란 사용. 모드가 있으면 **Cmd** 선택 | 실행 결과와 오류를 함께 확인 |
| 플레이 중 게임 콘솔 | Play로 실행한 뒤 게임 화면을 클릭하고 **백틱(`) / 물결(~) 키** 사용. 일반적인 영문 키보드에서 Esc 아래 키 | FPS·화면 디버깅·플레이 상태 확인 |

플레이 중 마우스가 잡혀 있으면 **Shift+F1**로 풀고 Output Log를 열 수 있습니다. 콘솔 키가 작동하지 않으면 영문 입력 상태와 게임 화면 포커스를 확인하고, **Edit → Project Settings → Engine → Input → Console Keys**에서 키를 확인합니다. 이 프로젝트의 `Config/DefaultInput.ini`에는 `ConsoleKeys=Tilde`가 설정되어 있습니다.

게임 화면에 표시하는 명령은 해당 PIE(Play In Editor) 게임 콘솔에서 실행하는 편이 명확합니다. 여러 플레이 창을 띄웠다면 확인할 창을 선택합니다. Shipping 빌드에서는 개발용 콘솔·통계·디버그 명령이 제한될 수 있습니다.

## 2. MCP 서버

### 자동 시작 설정

1. **Edit → Plugins**에서 **Unreal MCP**, **All Toolsets**를 활성화하고, 재시작 안내가 나오면 에디터를 재시작합니다. 이 프로젝트의 `.uproject`에는 두 플러그인이 이미 활성화되어 있습니다.
2. **Edit → Editor Preferences → General → Model Context Protocol**로 이동합니다.
3. **Auto Start Server**를 켭니다. 이후 에디터 시작 시 서버가 자동으로 실행됩니다.
4. 현재 실행 중인 에디터에서 바로 시작하려면 아래 명령을 입력합니다.

```text
ModelContextProtocol.StartServer
```

기본 접속 주소는 `http://127.0.0.1:8000/mcp`입니다. 포트나 URL 경로를 변경했다면 클라이언트에도 같은 값을 사용합니다.

| 명령 | 하는 일 | 사용 시점 |
|---|---|---|
| `ModelContextProtocol.StartServer` | 설정된 포트로 MCP 서버 시작 | 에디터를 연 뒤 수동 연결 |
| `ModelContextProtocol.StartServer 8001` | 포트를 지정해 서버 시작 | 기본 8000 포트가 사용 중일 때의 예시 |
| `ModelContextProtocol.StopServer` | 서버 중지, 연결된 세션 종료 | 서버 재시작·포트 변경 전 |
| `ModelContextProtocol.RefreshTools` | 등록된 도구 제공자를 다시 조회 | 도구 추가·핫 리로드 후 |
| `ModelContextProtocol.GenerateClientConfig Codex` | 프로젝트 루트에 Codex용 연결 설정 생성 | 최초 연결 설정 준비 |
| `ModelContextProtocol.GenerateClientConfig All` | 지원하는 모든 클라이언트의 설정 파일 생성 | 여러 클라이언트를 함께 준비할 때 |

포트 변경은 `StopServer` 후 원하는 포트로 `StartServer`를 실행하고 클라이언트 주소도 맞춥니다. 재시작 후 기존 클라이언트는 다시 연결해야 할 수 있습니다. 시작 성공 여부는 **Output Log**에서 `LogModelContextProtocol` 관련 메시지와 포트 오류를 확인합니다.

`GenerateClientConfig`는 파일을 쓰는 명령입니다. UE 5.8.2 구현에서 `Codex` 대상은 `.codex/config.toml`을 생성하며, 해당 파일이 이미 있으면 덮어쓰지 않고 오류를 기록합니다. `All`은 여러 설정 파일을 만들거나 갱신하므로 필요한 클라이언트만 지정해도 됩니다.

근거: [Epic — Unreal MCP](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor), 설치된 플러그인의 `ModelContextProtocolModule.cpp`, `ModelContextProtocolEngineModule.cpp`, `ModelContextProtocolClientConfig.cpp`.

## 3. 명령 검색·현재 값·도움말

| 입력 예시 | 의미 |
|---|---|
| `ModelContextProtocol.` | 접두사를 입력하고 자동완성 목록에서 MCP 명령 찾기 |
| `DumpConsoleCommands` | 콘솔 명령 목록을 로그에 출력 |
| `DumpConsoleCommands ModelContextProtocol.` | MCP 접두사에 해당하는 명령 목록 출력 |
| `stat help` | 사용 가능한 통계 명령 목록 확인 |
| `t.MaxFPS` | 현재 FPS 제한 값 조회 |
| `t.MaxFPS ?` | 해당 콘솔 변수의 도움말 확인 |
| `t.MaxFPS 60` | 해당 변수 값을 60으로 변경 |

`명령 값`의 값은 공백으로 구분합니다. 문서에서 `변수명`만 입력하면 조회, `변수명 값`이면 변경을 뜻합니다. 이 규칙은 콘솔 변수에 해당하며, `StartServer` 같은 실행 명령은 이름만 입력해도 동작합니다.

등록된 변수·명령은 자동완성과 도움말로 확인할 수 있습니다. 일부 오래된 Exec 명령은 자동완성 목록에 없을 수 있습니다. 근거: [Epic — Console Variables and Commands](https://dev.epicgames.com/documentation/en-us/unreal-engine/console-variables-cplusplus-in-unreal-engine), UE 5.8.2 `ConsoleManager.cpp`.

## 4. FPS·CPU·GPU·메모리

아래 `stat` **표시 명령**은 같은 명령을 다시 입력하면 표시를 끕니다. 기록을 시작하는 `stat startfile`은 별도의 `stat stopfile`로 종료합니다.

| 명령 | 확인할 내용 | 디펜스 프로젝트에서의 용도 |
|---|---|---|
| `stat fps` | 초당 프레임 수 | 전체 성능 빠르게 확인 |
| `stat unit` | Frame·Game·Draw·GPU 등의 시간(ms) | CPU 게임 처리·렌더 처리·GPU 중 병목 후보 구분 |
| `stat unitgraph` | 프레임 시간 그래프 | 웨이브 시작·소환 순간의 튐 확인 |
| `stat game` | 게임 처리 시간 | 유닛·적 Tick 등 처리 비용 확인 |
| `stat gpu` | GPU 작업별 시간 | 이펙트·그림자·후처리 비용 확인 |
| `stat scenerendering` | 장면 렌더링 통계 | 화면 오브젝트가 늘어날 때 비교 |
| `stat rhi` | 렌더링 하드웨어 인터페이스 통계 | 드로우 콜·리소스 관련 지표 확인 |
| `stat memory` | 하위 시스템별 메모리 통계 | 플레이 전후 메모리 비교 |
| `stat streaming` | 텍스처 스트리밍 사용량 | 텍스처 풀 초과·로딩 문제 확인 |
| `stat slate` | Slate UI 처리 통계 | UI 갱신 비용 확인. 에디터 UI 비용도 포함될 수 있음 |
| `stat ai` | AI 관련 처리 통계 | 적 수 증가에 따른 AI 부하 확인 |
| `stat navigation` | 내비게이션 통계 | 경로 탐색 관련 비용 확인 |
| `stat net` | 네트워크 통계 | 협동 플레이 송수신 상태 확인 |

60FPS의 프레임 예산은 약 **16.67ms**, 30FPS는 약 **33.33ms**입니다. `stat unit`에서 큰 항목을 먼저 살피되 스레드가 병렬로 움직이므로 Game·Draw·GPU 시간을 단순 합산하지 않습니다. GPU 측정 지원 여부와 세부 항목은 플랫폼·RHI에 따라 다릅니다. PC 에디터 수치는 Android 실기기 측정을 대신하지 않습니다.

근거: [Epic — Stat Commands](https://dev.epicgames.com/documentation/en-us/unreal-engine/stat-commands-in-unreal-engine).

## 5. 프레임 제한·그래픽 품질 비교

**변경 전 변수명만 입력해 기존 값을 기록하고, 비교 후 그 값으로 복구합니다.** 아래 숫자는 테스트 예시이며 프로젝트의 권장값·확정 기본값이 아닙니다.

| 명령 | 효과 | 복구 방법·조건 |
|---|---|---|
| `t.MaxFPS 30` | 최대 30FPS로 제한 | 기존 `t.MaxFPS` 값 재입력 |
| `t.MaxFPS 60` | 최대 60FPS로 제한 | 기존 값 재입력 |
| `t.MaxFPS 0` | 이 변수에 의한 FPS 제한 해제 | VSync·프레임 스무딩·기타 제한은 남을 수 있음 |
| `r.VSync 0` | 게임의 수직 동기화 끄기 | 기존 값 재입력. 켜기는 `r.VSync 1` |
| `r.ScreenPercentage 75` | 지원되는 경로에서 3D 렌더 해상도 비율 조정 | 기존 값 재입력. 에디터 뷰포트·동적 해상도·업스케일러 정책에 따라 적용이 달라짐 |
| `sg.ShadowQuality 0` | 그림자 품질 그룹을 Low로 변경 | 기존 `sg.ShadowQuality` 값 재입력 |
| `sg.PostProcessQuality 0` | 후처리 품질 그룹을 Low로 변경 | 기존 값 재입력 |
| `sg.EffectsQuality 0` | 이펙트 품질 그룹을 Low로 변경 | 기존 값 재입력 |
| `sg.TextureQuality 0` | 텍스처 품질 그룹을 Low로 변경 | 기존 값 재입력. 스트리밍 반영에 시간이 걸릴 수 있음 |

주요 품질 그룹은 `0=Low`, `1=Medium`, `2=High`, `3=Epic`을 사용하며 지원 그룹에는 `4=Cinematic`도 있습니다. 한 항목씩 바꾸고 같은 장면에서 비교합니다. 모바일 렌더링 경로에서는 일부 효과나 변수가 적용되지 않을 수 있습니다.

콘솔 입력만으로 프로젝트 설정 파일이 자동 저장된다고 가정하지 않습니다. 에디터 전역 변수는 PIE 종료 뒤에도 남을 수 있고, 설정 우선순위에 따라 입력이 무시되거나 다른 설정을 덮어쓸 수 있으므로 로그와 현재 값을 확인합니다.

근거: [Epic — Scalability Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/scalability-reference-for-unreal-engine), [Console Variables Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-console-variables-reference), UE 5.8.2의 콘솔 변수 등록 소스.

## 6. 화면·충돌 디버깅

| 명령 | 효과 | 되돌리기 |
|---|---|---|
| `viewmode lit` | 조명이 적용된 일반 화면 | 일반 화면으로 돌아갈 때 사용 |
| `viewmode unlit` | 조명 영향을 제외한 화면 | `viewmode lit` |
| `viewmode wireframe` | 와이어프레임 표시 | `viewmode lit` |
| `viewmode shadercomplexity` | 셰이더 복잡도 시각화 | `viewmode lit` |
| `show collision` | 충돌 형상 표시 전환 | 같은 명령 다시 입력 |
| `show navigation` | 내비게이션 데이터 표시 전환 | 같은 명령 다시 입력 |
| `DisableAllScreenMessages` | 화면 디버그 메시지 숨김 | `EnableAllScreenMessages` |
| `EnableAllScreenMessages` | 화면 디버그 메시지 표시 허용 | 숨김 해제용 |

View Mode는 개발용 화면에서 사용합니다. 렌더링 경로·빌드에 따라 특정 모드가 지원되지 않을 수 있습니다. `show collision`은 충돌 기능을 끄는 명령이 아니며, `show navigation`은 없는 NavMesh를 생성하지 않습니다. 화면 메시지 숨김은 로그 삭제나 오류 해결을 뜻하지 않으며 UMG 게임 UI를 숨기는 명령도 아닙니다.

근거: [Epic — Viewport Modes](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-modes-in-unreal-engine), UE 5.8.2 `GameViewportClient.cpp`의 Show/ViewMode 처리와 `UnrealEngine.cpp`의 화면 메시지 처리.

## 7. 스크린샷·성능 기록

| 명령 | 생성 결과 | 확인 위치·종료 |
|---|---|---|
| `Shot` | 현재 뷰포트 해상도의 스크린샷 | 프로젝트의 `Saved/Screenshots/` 아래 플랫폼 폴더 |
| `HighResShot 2` | 가로·세로 각각 2배인 고해상도 이미지 | 같은 스크린샷 폴더. 총 픽셀 수는 4배 |
| `HighResShot 1920x1080` | 지정한 해상도의 이미지 | 같은 스크린샷 폴더 |
| `stat startfile` | 통계 파일 기록 시작 | 보통 `Saved/Profiling/UnrealStats/` |
| `stat stopfile` | 통계 기록 종료·파일 닫기 | 기록을 시작했다면 반드시 실행 |

스크린샷의 정확한 저장 경로는 실행 후 로그·알림에서 확인합니다. `HighResShot`은 렌더링 부하가 크므로 성능 측정 중에는 실행하지 않습니다. `stat startfile`은 PIE를 종료해도 기록이 계속될 수 있으므로 `stat stopfile`을 따로 입력합니다. 이 기록은 Unreal Insights 트레이스와 별도 형식입니다.

근거: [Epic — Taking Screenshots](https://dev.epicgames.com/documentation/en-us/unreal-engine/taking-screenshots-in-unreal-engine), [Stat Commands](https://dev.epicgames.com/documentation/en-us/unreal-engine/stat-commands-in-unreal-engine).

## 8. 자주 쓰는 순서

### MCP 연결 준비

에디터의 **Cmd** 입력란에서 실행합니다.

```text
ModelContextProtocol.StartServer
```

Output Log에서 시작 결과를 확인합니다. 클라이언트 설정이 아직 없을 때만 필요한 대상의 `GenerateClientConfig` 명령을 추가로 사용합니다.

### 플레이 성능 확인

게임을 Play로 실행하고 게임 콘솔에서 한 줄씩 입력합니다.

```text
stat fps
stat unit
stat streaming
```

같은 웨이브·유닛 수·카메라 조건에서 관찰합니다. 확인이 끝나면 표시를 켤 때 입력했던 세 명령을 각각 다시 입력해 끕니다.

### 느린 구간 기록

```text
stat startfile
```

느린 상황을 재현한 뒤 아래 명령으로 기록을 마칩니다.

```text
stat stopfile
```

## 9. 명령이 안 될 때

| 증상 | 확인할 것 |
|---|---|
| PowerShell에서 명령을 찾을 수 없다고 나옴 | 언리얼 내부 콘솔의 **Cmd** 입력란으로 이동 |
| Python 오류가 나옴 | 입력 모드가 **Python**인지 확인하고 **Cmd**로 변경 |
| `Command not recognized`가 나옴 | 철자·엔진 버전·필요한 플러그인·게임 실행 여부 확인. 접두사 자동완성 또는 명령 목록 조회 |
| MCP 명령이 없음 | Unreal MCP 플러그인 활성화와 에디터 재시작 여부 확인 |
| MCP 시작 시 포트 오류 | Output Log에서 사용 중인 포트 확인. 다른 포트로 시작했다면 클라이언트 주소도 변경 |
| MCP는 시작했는데 도구가 없거나 갱신되지 않음 | All Toolsets 또는 필요한 개별 도구 플러그인 확인, `ModelContextProtocol.RefreshTools` 후 클라이언트 도구 목록 새로고침 |
| 통계가 표시되지 않음 | 해당 게임 창에서 실행했는지, 다시 입력해 꺼진 상태인지, 빌드·플랫폼이 통계를 지원하는지 확인 |
| 값을 바꿨는데 차이가 없음 | 변수명만 입력해 현재 값과 로그 확인. 설정 우선순위·렌더링 경로·해당 기능 사용 여부 확인 |

전체 명령을 더 찾으려면 [Epic — Console Commands Reference](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-console-commands-reference?lang=en-US)를 참고합니다. 실제 설치 버전에서의 자동완성·도움말·로그를 함께 확인합니다.
