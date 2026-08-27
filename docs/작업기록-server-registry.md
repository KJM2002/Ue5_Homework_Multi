# 작업 기록 — 서버 IP 등록 / 조회

**작업일:** 2026-08-27
**계획서:** `docs/superpowers/plans/2026-08-27-server-registry.md`
**기준선:** `1e0994d` (로그인/회원가입 완료)

## 1. 무엇을 만들었나

| 요구사항 | 구현 |
|---|---|
| 언리얼 서버로 시작하면 웹서버에 서버 IP 등록 | `ALobbyGM::BeginPlay` → `UWebApiSubsystem::StartServerRegistration` → `POST /server/register` |
| 클라이언트로 로그인하면 등록 서버 IP를 받아서 접속 | 로그인 성공 → `RequestLatestServer` → `GET /server/latest` → `ConnectServer`가 받은 주소로 `OpenLevel` |

## 2. 결정과 근거

| 항목 | 결정 | 근거 |
|---|---|---|
| 서버 IP를 누가 정하나 | **웹서버가 `request.client.host`에서 읽는다** | 클라이언트가 IP를 위조할 수 없다. LAN 환경에서 자동으로 맞는다. UE에서 로컬 IP를 찾으려면 `Sockets` 모듈이 추가로 필요한데, 그 의존성도 피할 수 있다 |
| 서버가 1개인가 목록인가 | DB에는 여러 개, 클라이언트에는 **최신 활성 1개** | 과제 문장("등록 서버 아이피를 받아서")은 단수다. 테이블은 여러 개를 담아 확장 여지를 남긴다 |
| 저장 위치 | **MySQL `seul.game_server`** | 웹서버 재시작에도 남는다. 이미 DB가 프로젝트의 축이라 일관된다 |
| 비정상 종료 대응 | **heartbeat 10초 + `updated_at` 30초 창** | 크래시·강제 종료로 `unregister`를 못 불러도 30초 뒤 조회에서 자동으로 빠진다. 활성 판정을 `updated_at` 하나로만 하므로 상태 플래그 관리가 필요없다 |
| 포트 | 저장하되 0이면 **7777** | `UNIQUE (server_ip, port)`로 같은 PC의 다른 포트를 별개 서버로 다룬다 |

## 3. 핵심 설계 — 등록 상태를 GameMode에 두지 않았다

`ALobbyGM::StartGame()`이 `GetWorld()->ServerTravel(TEXT("Lvl_ThirdPerson"))`을 호출한다. 로비 카운트다운이 끝나면 레벨이 넘어가고 **`ALobbyGM`이 파괴된다.**

등록 상태와 heartbeat 타이머를 GameMode에 뒀다면, 로비→게임 전환 순간에 **살아 있는 서버가 스스로 등록 해제**된다. 그 시점부터 새 클라이언트는 이 서버를 찾을 수 없다.

그래서:

- 상태(`RegisteredWebServerIP`, `RegisteredOwnerIdx`, `RegisteredGamePort`)와 `HeartbeatTimerHandle`은 `UWebApiSubsystem`(GameInstanceSubsystem)이 든다 — 레벨 이동에 살아남는다
- 타이머는 `GetGameInstance()->GetTimerManager()`를 쓴다 — `GetWorld()->GetTimerManager()`는 `ServerTravel`에서 끊긴다
- `ALobbyGM::BeginPlay`는 "서버가 떴다"는 신호만 보낸다

이건 계획 단계에서 코드를 읽고 발견한 것이다. 구현하고 나서 알았다면 되돌리는 비용이 컸다.

## 4. 계획에서 벗어난 지점

| 항목 | 계획 | 실제 | 이유 |
|---|---|---|---|
| heartbeat 존재 판정 | `UPDATE`의 영향 행 수로 판정 | **`SELECT` 먼저, 그다음 `UPDATE`** | PyMySQL은 기본적으로 `CLIENT.FOUND_ROWS`를 켜지 않아 `UPDATE`가 **변경된** 행 수를 돌려준다. 같은 초에 두 번 들어온 heartbeat는 `updated_at`이 그대로여서 0행이 되고, 등록된 서버를 "미등록"으로 오판한다 |
| `ParseResponse` | 언급 없음 | 정적 헬퍼로 분리 | 새 엔드포인트 3개가 같은 판정(HTTP 200 + `result` 플래그)을 공유한다. 기존 `HandleAuthResponse`는 건드리지 않았다 — 델리게이트 시그니처가 달라 재사용이 안 되고, 굳이 리팩터링할 이유가 없다 |
| `Build.cs` | 변경 없음 | 변경 없음 (그대로) | 서버 IP를 웹서버가 판정하므로 `Sockets`/`Networking`이 필요없었다 |

## 5. 서버 쪽 검증 결과

`curl` + MySQL 직접 확인. 모두 통과.

| # | 검증 | 결과 |
|---|---|---|
| 1 | 첫 등록 | `{"result":true,"server_ip":"127.0.0.1","port":7777}` |
| 2 | 같은 주소 재등록 | 행 수 1 유지 (UPSERT 동작) |
| 3 | `port: 0` 전송 | 7777로 보정, 행 수 1 유지 |
| 4 | 다른 포트(7778) 등록 | 행 수 2 — 별개 서버로 취급 |
| 5 | `updated_at`을 5초 전으로 되돌린 뒤 heartbeat | `12:02:15` → `12:02:20` 갱신 확인 |
| 6 | 같은 초에 연속 heartbeat 2회 | **둘 다 `result: true`** — 4번 항목의 오판이 실제로 막혔다 |
| 7 | 미등록 포트(9999)로 heartbeat | `{"result":false,"message":"등록되지 않은 서버입니다"}` |
| 8 | 활성 서버 조회 | `{"result":true,"server_ip":"127.0.0.1","port":7777,"nickname":"jaemin"}` — `member` JOIN 동작 |
| 9 | 전부 60초 전으로 강제 만료 후 조회 | `{"result":false,"message":"접속 가능한 서버가 없습니다"}` — **행은 테이블에 2개 남아 있는데 조회에서 빠졌다.** 비정상 종료 대응이 이렇게 동작한다 |
| 10 | 하나만 `updated_at` 갱신 후 조회 | 그 서버가 반환됨 |
| 11 | `unregister` | 행 삭제, 남은 행 1 |
| 12 | 같은 `unregister` 재호출 | `{"result":false,"message":"등록되지 않은 서버입니다"}` |

## 6. 빌드

```
[3/10] Compile [x64] DataGameInstanceSubsystem.cpp
[4/10] Compile [x64] TitleWidgetBase.cpp
[5/10] Compile [x64] LobbyGM.cpp
[6/10] Compile [x64] WebApiSubsystem.cpp
Result: Succeeded (36.58초)
```

## 7. 커밋

| 커밋 | 내용 |
|---|---|
| `d7944bd` | git 저장소 초기화 — 기존 프로젝트 기준선 |
| `1e0994d` | uvicorn을 `0.0.0.0`에 바인딩해 외부 접속 허용 |
| `e61b18f` | 구현 계획서 |
| `5c25c1d` | 서버: 엔드포인트 4개 + `schema.sql` |
| `5336c37` | 클라이언트: 서버 IP 자동 등록 및 조회 + README |

## 8. 시작 전 정리한 환경 문제

과제와 직접 관련은 없지만, 없으면 진행이 막혔던 것들.

| 문제 | 처리 |
|---|---|
| 프로젝트가 다운로드 폴더에 있었다 (`...-main (1)`, 경로에 공백과 괄호) | `D:\Work\L20260713_Day03`로 이동. `Server/.venv`는 절대경로가 박혀 있어 재생성 |
| git 저장소가 아니었다 (`.git` 없음) | `git init` + 기준선 커밋 |
| `.gitattributes`에 `Content/* filter=lfs`가 있었다 | 제거. 패턴이 한 파일도 매칭하지 않았고(모든 에셋이 더 깊은 경로), 클론하는 쪽에 git-lfs가 없으면 `.uasset`이 포인터 파일로 받아져 프로젝트가 열리지 않는다. 50MB 넘는 파일이 없어 일반 git으로 충분하다. 대신 `*.uasset`/`*.umap`을 `binary`로 명시해 줄바꿈 정규화를 막았다 |
| `uvicorn`이 `127.0.0.1`에만 바인딩됐다 | `run.bat`에 `--host 0.0.0.0` 추가. 없으면 2대 테스트가 아예 불가능하다 |
| MySQL root 비밀번호를 알 수 없었다 (3306은 `MySQL267` 서비스) | 공식 `--init-file` 절차로 재설정 |
| `.gitignore`에 AI 도구 설정이 빠져 있었다 | `.codex`, `.cursor`, `.gemini`, `.vscode`, `.mcp.json`, `*.slnx` 추가 |

## 9. 사람이 직접 확인할 일

에디터 PIE 실행은 자동화할 수 없다. 아래를 순서대로 확인한다.

**사전 준비:** `Server/run.bat` 실행, `DELETE FROM seul.game_server;`로 비운 상태.

| # | 조작 | 기대 결과 |
|---|---|---|
| 1 | 클라이언트 A: `Title` 레벨 PIE → 로그인 | `ConnectServer` **비활성**, `jaemin (Lv.1) · 접속 가능한 서버가 없습니다` |
| 2 | 클라이언트 A: StartServer | 로비 진입. `SELECT * FROM seul.game_server;` 에 행 1개 |
| 3 | 10초 대기 | `updated_at` 갱신됨 |
| 4 | 클라이언트 B: 로그인 | `ConnectServer` **활성**, `접속 가능: <A의 IP>:7777 (jaemin)` |
| 5 | 클라이언트 B: ConnectServer | A의 로비 접속, 접속 인원 2명 |
| 6 | 로비 카운트다운 종료 → `Lvl_ThirdPerson` | **행이 그대로 남고 heartbeat 계속** ← 3장 설계의 검증 |
| 7 | 서버 A 정상 종료 | 행 삭제됨 |
| 8 | 서버 A 재시작 후 작업 관리자로 **강제 종료** | 행은 남지만 30초 뒤 `/server/latest`가 `result: false` |
| 9 | 웹서버를 끄고 클라이언트 로그인 | `서버에 연결할 수 없습니다`. 에디터가 멈추거나 크래시하지 않아야 한다 |

1인 PC에서는 Standalone 창 2개로 흐름만 확인한다(양쪽 IP가 `127.0.0.1`이라 "받아온 IP로 접속"이 실질적으로 증명되지 않는다). 4~5번의 실제 의미는 **2대 LAN 환경**에서 확인한다. 방화벽 인바운드 8080(웹서버)·7777(게임서버) 허용 필요.

## 10. 미결 사항

- **`StartServerButton`만 `meta = (BindWidget)`이 아니다.** `GetWidgetFromName`으로 찾으므로 `WBP_Title`에 이름이 없거나 틀려도 블루프린트 컴파일은 통과하고 런타임에 조용히 무동작한다. 서버 등록이 안 될 때 첫 확인 지점.
- **`LobbyWidgetBase.h`에 `meta = (WidgetBind)` 오타가 6곳 남아 있다** (`StartButton`, `ChatButton`, `ChatInput`, `LeftTimeText`, `ConnectCountText`, `ChatScrollBox`). 원래 `BindWidget`이다. 이번 범위 밖이라 손대지 않았다. 고치면 `WBP_Lobby`에 같은 이름 위젯이 실제로 있어야 컴파일되므로 별도 작업으로 다룰 것.
- **명령줄로 로비에 바로 진입하면 등록되지 않는다.** 웹서버 주소를 타이틀 화면에서 받기 때문이다. 데디케이티드 서버로 띄우려면 웹서버 주소를 커맨드라인 인자나 config에서 읽는 경로가 추가로 필요하다. 현재 과제 범위(타이틀 → 리슨 서버)에는 해당하지 않는다.
- **`Deinitialize`의 `unregister`는 응답을 기다리지 않는다.** 종료 경로라 HTTP가 완료되기 전에 프로세스가 내려갈 수 있다. 놓쳐도 `updated_at` 30초 창이 대신 정리하므로 기능상 문제는 없다.
