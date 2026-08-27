# 서버 IP 등록 / 조회 구현 계획

**Goal:** 언리얼이 리슨 서버로 시작하면 자기 주소를 웹서버에 등록하고, 클라이언트는 로그인 성공 시 등록된 서버 주소를 받아 그곳으로 접속한다. 사용자가 게임서버 IP를 직접 입력하지 않는다.

**Architecture:** FastAPI가 `/server/register`·`/server/heartbeat`·`/server/unregister`·`/server/latest` 네 엔드포인트를 제공하고 `seul.game_server` 테이블을 읽고 쓴다. 서버 IP는 클라이언트가 보내지 않고 **웹서버가 요청의 소스 주소(`request.client.host`)에서 읽는다.** UE 쪽은 기존 `UWebApiSubsystem`(GameInstance 수명)이 등록·heartbeat·해제·조회를 전담하고, `ALobbyGM::BeginPlay`는 등록 시작 트리거만 담당한다. 클라이언트는 로그인 성공 직후 최신 서버를 조회해 캐시하고, `ConnectServer`는 캐시된 주소로 이동한다.

**Tech Stack:** Python 3.11.8 / FastAPI 0.115.6 / Uvicorn 0.34.0 / PyMySQL 1.1.1 / Pydantic v2 · UE 5.8 C++ (HTTP, Json, JsonUtilities — 이미 Build.cs에 있음) · MySQL 26.7

**Base:** commit `1e0994d` (로그인/회원가입 완료 + uvicorn 0.0.0.0 바인딩)

**선행 계획서:** `docs/superpowers/plans/2026-08-26-login-signup.md`

---

## 왜 등록을 GameMode에 두지 않는가

`ALobbyGM::StartGame()`이 `GetWorld()->ServerTravel(TEXT("Lvl_ThirdPerson"))`을 호출한다. 레벨이 넘어가면 `ALobbyGM`은 파괴되고 새 GameMode가 생성된다. 등록·heartbeat를 GameMode 수명에 묶으면 **살아 있는 서버가 로비→게임 전환 시점에 등록 해제된다.**

그래서 상태와 타이머는 `UWebApiSubsystem`(GameInstanceSubsystem — 레벨 이동에 살아남음)이 들고, 타이머는 `GetGameInstance()->GetTimerManager()`를 쓴다. `ALobbyGM::BeginPlay`는 "서버가 떴다"는 신호만 보낸다.

---

## Global Constraints

- **`member` 테이블 스키마를 변경하지 않는다.** `game_server`는 새 테이블로 추가한다. (선행 계획서의 제약 유지)
- **`LobbyWidgetBase.h`를 수정하지 않는다.** `meta = (WidgetBind)` 오타가 6곳 남아 있다(원래 `BindWidget`). 이번 범위 밖이고, 고치면 `WBP_Lobby`에 같은 이름 위젯이 실제로 있어야 컴파일되므로 깨진다.
- **위젯 이름을 바꾸지 않는다.** `ConnectServerButton`, `UserID`, `Password`, `ServerIP`는 `meta = (BindWidget)`이라 이름을 바꾸면 `WBP_Title`도 함께 고쳐야 한다. `ServerIP` 칸의 **의미만** "웹서버 주소"로 좁힌다.
- **웹서버 포트 8080 고정.** `ModelContextProtocol` 플러그인이 8000을 점유한다.
- **게임서버 포트 기본 7777.** 등록 시 실제 포트를 보내되 0이면 7777로 채운다.
- **서버 IP는 웹서버가 판정한다.** 클라이언트가 `server_ip`를 보내도 무시한다. 위조 방지 + LAN 환경 자동 판정.
- **실패도 HTTP 200으로 응답한다.** 기존 `/login`·`/signup`과 동일하게 `{"result": false, "message": "..."}`. 401/404를 쓰지 않는다.
- **비밀번호는 평문으로 저장한다.** 실습 프로젝트 전제. 해시를 임의로 도입하지 않는다.
- **UE 로직은 C++로만 작성한다.** 블루프린트에는 위젯 배치만.
- **pytest를 사용하지 않는다.** 검증은 `curl` + `/docs` + MySQL 확인. 각 태스크의 "검증"을 건너뛰지 않는다.
- **파이썬 패키지는 `Server/.venv`에만 설치한다.**

---

## 데이터 모델

```sql
CREATE TABLE IF NOT EXISTS seul.game_server (
  idx        INT AUTO_INCREMENT PRIMARY KEY,
  server_ip  VARCHAR(45)  NOT NULL,               -- IPv6 표기 대비 45
  port       INT          NOT NULL DEFAULT 7777,
  owner_idx  INT          NOT NULL,               -- member.idx — 누가 띄운 서버인지
  created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
                          ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uk_addr (server_ip, port)
) DEFAULT CHARSET utf8mb4;
```

- `UNIQUE (server_ip, port)` — 같은 주소 재등록은 UPSERT로 처리한다. 같은 PC에서 포트만 다르면 별개 서버.
- **활성 판정은 `updated_at`으로만 한다.** `updated_at >= NOW() - INTERVAL 30 SECOND`. 비정상 종료(크래시·강제 종료)로 `unregister`가 못 불려도 30초 뒤 자동으로 목록에서 빠진다.
- heartbeat 주기 10초, 활성 창 30초 → 두 번 연속 놓쳐야 죽은 것으로 본다.

---

## API 계약

| 메서드 | 경로 | 요청 | 응답 |
|---|---|---|---|
| POST | `/server/register` | `{owner_idx, port}` | `{result, message, server_ip, port}` |
| POST | `/server/heartbeat` | `{owner_idx, port}` | `{result, message}` |
| POST | `/server/unregister` | `{owner_idx, port}` | `{result, message}` |
| GET | `/server/latest` | — | `{result, message, server_ip, port, nickname}` |

- 세 POST 모두 대상 행을 `(request.client.host, port)`로 찾는다. `owner_idx`는 기록용.
- `/server/latest`는 활성 서버 중 `updated_at DESC LIMIT 1`. 없으면 `{"result": false, "message": "접속 가능한 서버가 없습니다"}`.
- `nickname`은 `member`와 JOIN해서 "누가 띄운 방인지" 표시용으로 반환한다.

---

## File Structure

| 파일 | 책임 |
|---|---|
| `Server/schema.sql` | **신규.** `member` + `game_server` DDL. 채점자 재현용 |
| `Server/main.py` | 엔드포인트 4개 추가 |
| `Source/.../Web/WebApiSubsystem.h/.cpp` | 등록·heartbeat·해제·조회 + 델리게이트 + 타이머 |
| `Source/.../DataGameInstanceSubsystem.h` | 조회 결과 캐시 |
| `Source/.../Lobby/LobbyGM.h/.cpp` | `BeginPlay`에서 등록 시작 (서버 전용) |
| `Source/.../Title/TitleWidgetBase.h/.cpp` | 로그인 성공 시 조회, `ConnectServer`가 받은 주소로 이동 |
| `README.md` | **신규.** 클론부터 실행까지 |
| `docs/작업기록-server-registry.md` | 작업 기록 |

`Build.cs` 변경 없음 — 서버 IP를 웹서버가 판정하므로 `Sockets`/`Networking` 모듈이 필요하지 않다.

---

## Tasks

### Task 1 — DB 스키마

- [ ] `Server/schema.sql` 작성 (`member` + `game_server`, 둘 다 `IF NOT EXISTS`)
- [ ] MySQL에 `game_server` 적용

**검증:** `SHOW COLUMNS FROM seul.game_server;` 로 6개 컬럼과 `uk_addr` 확인. `member`는 건드리지 않았는지 기존 행 유지 확인.

### Task 2 — `POST /server/register`

- [ ] `ServerRegisterRequest`(`owner_idx: int`, `port: int = 7777`) / `ServerResponse` 모델 추가
- [ ] `request: Request`에서 `request.client.host` 읽기
- [ ] `INSERT ... ON DUPLICATE KEY UPDATE owner_idx = VALUES(owner_idx), updated_at = NOW()`
- [ ] `port <= 0`이면 7777로 보정

**검증:** `curl -X POST .../server/register -d '{"owner_idx":1,"port":7777}'` → `{"result":true,"server_ip":"127.0.0.1","port":7777}`. 같은 요청 두 번 → 행이 늘지 않고 `updated_at`만 갱신.

### Task 3 — `POST /server/heartbeat`, `POST /server/unregister`

- [ ] heartbeat: `UPDATE game_server SET updated_at = NOW() WHERE server_ip = %s AND port = %s`
- [ ] 갱신 행이 0이면 `{"result": false, "message": "등록되지 않은 서버입니다"}` — 클라이언트가 재등록할 근거
- [ ] unregister: `DELETE FROM game_server WHERE server_ip = %s AND port = %s`

**검증:** heartbeat 호출 전후 `updated_at` 변화 확인. 없는 주소로 heartbeat → `result: false`. unregister 후 행 삭제 확인.

### Task 4 — `GET /server/latest`

- [ ] `SELECT g.server_ip, g.port, m.nickname FROM game_server g LEFT JOIN member m ON m.idx = g.owner_idx WHERE g.updated_at >= NOW() - INTERVAL 30 SECOND ORDER BY g.updated_at DESC LIMIT 1`
- [ ] 결과 없으면 `{"result": false, "message": "접속 가능한 서버가 없습니다"}`

**검증:** 등록 직후 조회 → 해당 서버 반환. `UPDATE game_server SET updated_at = NOW() - INTERVAL 60 SECOND` 로 강제 만료 후 조회 → `result: false`. **이것이 비정상 종료 대응의 검증이다.**

### Task 5 — `UWebApiSubsystem` 확장

- [ ] 델리게이트 추가
  - `OnRegisterServerResult` — 기존 `FWebApiResultSignature` 재사용
  - `OnLatestServerResult(bool, FString Message, FString ServerIP, int32 Port, FString OwnerNickname)` — 새 시그니처
- [ ] `StartServerRegistration(WebServerIP, OwnerIdx, Port)` — 즉시 등록 + `GetGameInstance()->GetTimerManager()`에 10초 반복 heartbeat 등록
- [ ] `StopServerRegistration()` — 타이머 해제 + `/server/unregister` 호출
- [ ] heartbeat 응답이 `result: false`면 자동 재등록 (웹서버·DB가 재시작된 경우 복구)
- [ ] `RequestLatestServer(WebServerIP)` — `GET /server/latest`
- [ ] `Deinitialize()`에서 `StopServerRegistration()` 호출 — 정상 종료 시 즉시 해제
- [ ] 응답 파싱은 기존 `HandleAuthResponse`와 같은 방식(HTTP 200 + `result` 플래그 판정), `TWeakObjectPtr` 가드 유지

**주의:** 기존 `SendAuthRequest`는 `FWebApiResultSignature&`를 포인터로 캡처한다. 새 델리게이트는 시그니처가 다르므로 이 함수를 재사용하지 말고 별도 경로로 짠다.

**검증:** 컴파일 통과. PIE 로그에서 URL과 응답 확인.

### Task 6 — `UDataGameInstanceSubsystem` 필드 추가

- [ ] `FString GameServerIP`, `int32 GameServerPort = 0`, `FString GameServerOwner`, `bool bServerFound = false`

**검증:** 컴파일 통과.

### Task 7 — `ALobbyGM`에서 등록 시작

- [ ] `BeginPlay()`에 등록 호출 추가 (`Super::BeginPlay()` 호출 유지 — BP 이벤트가 여기 걸려 있다)
- [ ] GameMode는 서버에만 존재하므로 넷모드 분기는 불필요. 다만 `GetNetMode() == NM_Standalone`이면 등록하지 않는다 (혼자 플레이는 서버가 아니다)
- [ ] 웹서버 주소와 `owner_idx`는 `UDataGameInstanceSubsystem`의 `ServerIP`, `Idx`에서 읽는다 — 기존 `SaveData()`가 이미 저장한다
- [ ] 포트는 `GetWorld()->URL.Port`, 0이면 7777

**검증:** 리슨 서버로 로비 진입 후 `SELECT * FROM seul.game_server;` 에 행 1개. 10초 뒤 `updated_at` 갱신. 종료 후 행 삭제. **`StartGame()`의 `ServerTravel`로 `Lvl_ThirdPerson`에 넘어간 뒤에도 행이 남아 있고 heartbeat가 계속되는지 확인 — 이게 이번 설계의 핵심 검증이다.**

### Task 8 — `UTitleWidgetBase` 클라이언트 흐름

- [ ] `OnLatestServerResult` 구독, `ProcessLatestServerResult` 추가
- [ ] `ProcessLoginResult` 성공 시 `RequestLatestServer()` 호출
- [ ] `StartServerButton`은 로그인만 되면 활성 (서버가 없어도 방을 열 수 있어야 한다)
- [ ] `ConnectServerButton`은 **활성 서버를 찾았을 때만** 활성
- [ ] `InfoText`에 `jaemin (Lv.1) · 접속 가능: 172.21.1.14:7777` 형태로 표시, 없으면 `접속 가능한 서버가 없습니다`
- [ ] `ConnectServer()`는 `ServerIP` 입력값이 아니라 캐시된 `GameServerIP:GameServerPort`로 `OpenLevel` 한다
- [ ] `ClearLoginState()`에서 캐시도 비운다

**주의:** `StartServerButton`만 `meta = (BindWidget)`이 아니라 `GetWidgetFromName`으로 찾는다. 이름이 틀리면 컴파일은 통과하고 런타임에 조용히 무동작한다. 서버 등록이 안 될 때 첫 번째 의심 지점.

### Task 9 — 통합 검증

**사전 준비:** `Server/run.bat` 실행, MySQL 기동, `game_server` 비어 있는 상태.

| # | 조작 | 기대 결과 |
|---|---|---|
| 1 | 클라이언트 A: PIE → 로그인 | `ConnectServer` **비활성**, `접속 가능한 서버가 없습니다` |
| 2 | 클라이언트 A: StartServer | 로비 진입. `game_server`에 행 1개 (`server_ip` = A의 IP) |
| 3 | 10초 대기 | `updated_at` 갱신됨 |
| 4 | 클라이언트 B: 로그인 | `ConnectServer` **활성**, `접속 가능: <A의 IP>:7777` 표시 |
| 5 | 클라이언트 B: ConnectServer | A의 로비에 접속, 접속 인원 2명 |
| 6 | 로비 카운트다운 종료 → `Lvl_ThirdPerson` | **행이 그대로 남고 heartbeat 계속** (ServerTravel 회귀 검증) |
| 7 | 서버 A 정상 종료 | 행 삭제됨 |
| 8 | 서버 A 재시작 후 **강제 종료**(작업 관리자) | 행이 남지만 30초 뒤 `/server/latest`가 `result: false` |
| 9 | 웹서버를 끄고 클라이언트 로그인 | `서버에 연결할 수 없습니다`. 에디터가 멈추거나 크래시하지 않아야 한다 |

1인 PC 검증은 Standalone 창 2개로 흐름만 확인하고(둘 다 `127.0.0.1`), 4~5번의 실제 의미는 **2대 LAN 환경**에서 확인한다. 이때 방화벽 인바운드 8080(웹서버) / 7777(게임서버) 허용이 필요하다.

### Task 10 — 문서

- [ ] `README.md` — 클론 → MySQL 계정·`schema.sql` → venv → `run.bat` → 에디터 실행. 세팅 함정(포트 8000 점유, uvicorn 기본 바인딩이 127.0.0.1, PowerShell `Set-Content -Encoding utf8`의 BOM) 포함
- [ ] `docs/작업기록-server-registry.md` — 결정과 근거, 계획에서 벗어난 지점, 알려진 한계

**알려진 한계로 명시할 것:** 비밀번호 평문 저장 · `owner_idx` 위조 가능(인증 토큰 없음) · NAT 뒤 서버는 사설 IP가 등록되어 외부에서 접속 불가 · 서버 목록이 아니라 최신 1개만 노출.

---

## 리스크

| 리스크 | 대응 |
|---|---|
| `ServerTravel` 시 등록이 끊긴다 | 상태·타이머를 GameInstance 수명에 둔다. Task 9-6이 이걸 검증한다 |
| 같은 PC 테스트에서는 IP가 전부 `127.0.0.1`이라 기능이 증명되지 않는다 | 흐름은 1PC, 의미는 2PC LAN에서 검증. Task 9에 분리 명시 |
| `LobbyWidgetBase.h` 오타를 건드려 `WBP_Lobby`가 깨진다 | 제약으로 명시. 이 파일은 열지 않는다 |
| `StartServerButton`이 조용히 무동작 | 등록이 안 될 때 첫 확인 지점으로 문서화 |
| GameMode가 클라이언트에도 있는 것처럼 착각 | GameMode는 서버에만 존재. `NM_Standalone` 분기만 추가 |
