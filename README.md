# L20260713_Day03 — 로그인 / 회원가입 / 서버 등록

UE5 클라이언트가 파이썬 웹서버를 통해 MySQL에 로그인·회원가입하고, 리슨 서버로 시작하면 자기 주소를 웹서버에 등록한다. 다른 클라이언트는 로그인하면 등록된 서버 주소를 받아 그곳으로 접속한다. 사용자가 게임서버 IP를 직접 입력하지 않는다.

```
UE5 클라이언트  ──HTTP/JSON──▶  FastAPI (8080)  ──PyMySQL──▶  MySQL (seul)
     │                                                            │
     └────────── 게임 접속 (7777) ──▶ UE5 리슨 서버 ◀─── 등록/heartbeat
```

## 요구 사항

| | 버전 |
|---|---|
| Unreal Engine | 5.8 |
| MySQL | 8.0 이상 |
| Python | 3.11 이상 |
| Visual Studio | 2022 (C++ 게임 개발 워크로드) |

Python을 따로 설치하지 않았다면 엔진에 들어 있는 것을 써도 된다:
`C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe`

## 1. DB 준비

```bash
mysql -u root -p < Server/schema.sql
```

`seul` 데이터베이스와 `member`·`game_server` 두 테이블을 만든다. 이미 있으면 건너뛴다(`IF NOT EXISTS`).

그다음 `Server/db.py`의 접속 정보를 자기 환경에 맞게 고친다.

```python
DB_CONFIG = dict(
    host="127.0.0.1",
    port=3306,
    user="root",
    password="여기를 고친다",
    db="seul",
    charset="utf8mb4",
)
```

## 2. 파이썬 환경

가상환경은 `Server/.venv`에만 만든다. 전역 파이썬은 건드리지 않는다.

```bash
python -m venv Server/.venv
```

```bash
Server\.venv\Scripts\python.exe -m pip install -r Server/requirements.txt
```

## 3. 웹서버 실행

```bash
Server\run.bat
```

`http://127.0.0.1:8080/docs`가 열리면 정상이다. Swagger UI에서 UE 없이 엔드포인트를 먼저 눌러볼 수 있다. 멈출 때는 `Ctrl+C`.

## 4. 언리얼 실행

1. `L20260713_Day03.uproject`를 연다 (C++ 빌드가 필요하면 프롬프트에 따른다)
2. `Content/Maps/Title`을 열고 Play
3. `ServerIP` 칸에 **웹서버** 주소를 넣는다 (로컬이면 `127.0.0.1`)
4. 아이디·비밀번호를 넣고 **회원가입** → **로그인**
5. 로그인 성공 시 등록된 서버를 자동 조회한다
   - 서버가 있으면 `접속 가능: 172.21.1.14:7777 (jaemin)` 이 뜨고 **Connect Server**가 열린다
   - 없으면 `접속 가능한 서버가 없습니다` 가 뜨고 Connect Server는 잠긴 채다
6. **Start Server**를 누르면 리슨 서버로 로비에 들어가고, 그 순간 웹서버에 등록된다

> `ServerIP` 칸은 **웹서버 주소**다. 게임서버 주소가 아니다. 게임서버 주소는 웹서버에서 받아온다.

## 5. 테스트

### 5.1 준비

웹서버와 MySQL을 켜고, 등록 테이블을 비운 상태에서 시작한다.

```sql
DELETE FROM seul.game_server;
```

확인은 이 쿼리로 한다. `age_sec`이 30을 넘으면 그 서버는 조회에서 빠진다.

```sql
SELECT idx, server_ip, port, owner_idx, updated_at,
       TIMESTAMPDIFF(SECOND, updated_at, NOW()) AS age_sec
FROM seul.game_server;
```

### 5.2 웹서버만 먼저 검증 (언리얼 없이)

UE에서 문제가 생겼을 때 서버 탓인지 클라이언트 탓인지 가르려면 여기부터 본다.

`http://127.0.0.1:8080/docs` 를 열고 Swagger UI에서 직접 호출한다.

| 순서 | 호출 | 기대 응답 |
|---|---|---|
| 1 | `POST /signup` — `{"user_id":"tester","passwd":"1234"}` | `{"result":true,"idx":...,"nickname":"tester","level":1}` |
| 2 | 같은 요청 한 번 더 | `{"result":false,"message":"이미 존재하는 아이디입니다"}` |
| 3 | `POST /login` — 비밀번호를 틀리게 | `{"result":false,"message":"아이디 또는 비밀번호가 올바르지 않습니다"}` |
| 4 | `POST /login` — 올바르게 | `{"result":true,...}` |
| 5 | `GET /server/latest` | `{"result":false,"message":"접속 가능한 서버가 없습니다"}` |
| 6 | `POST /server/register` — `{"owner_idx":1,"port":7777}` | `{"result":true,"server_ip":"127.0.0.1","port":7777}` |
| 7 | `GET /server/latest` | 6번에서 등록한 서버가 반환된다 |
| 8 | `POST /server/heartbeat` — 같은 본문 | `{"result":true,...}` |
| 9 | `POST /server/heartbeat` — `{"owner_idx":1,"port":9999}` | `{"result":false,"message":"등록되지 않은 서버입니다"}` |
| 10 | 아래 SQL로 강제 만료 후 `GET /server/latest` | `{"result":false,...}` — **행은 남아 있는데 조회에서 빠진다** |
| 11 | `POST /server/unregister` — 6번과 같은 본문 | `{"result":true,...}`, 행 삭제됨 |

10번의 강제 만료:

```sql
UPDATE seul.game_server SET updated_at = NOW() - INTERVAL 60 SECOND;
```

10번이 서버 크래시·강제 종료 대응의 검증이다. `unregister`가 호출되지 않아도 30초 뒤 접속 대상에서 자동으로 빠진다.

cmd에서 `curl`로 하려면:

```bash
curl -X POST http://127.0.0.1:8080/server/register -H "Content-Type: application/json" -d "{\"owner_idx\":1,\"port\":7777}"
```

### 5.3 한 PC에서 흐름 검증

에디터 Play 버튼 옆 **▼ → Standalone Game** 을 **두 번** 눌러 독립 창 2개를 띄운다. 각 창이 `Title`에서 따로 시작하므로 서버 역할과 클라이언트 역할을 나눌 수 있다.

양쪽 IP가 모두 `127.0.0.1`이라 "받아온 IP로 접속한다"가 실질적으로 증명되지는 않는다. 여기서는 **흐름과 버튼 잠금 상태만** 확인하고, 실제 의미는 5.5에서 확인한다.

### 5.4 체크리스트

두 창을 A(서버 역할), B(클라이언트 역할)로 부른다. 양쪽 모두 `ServerIP` 칸에 웹서버 주소를 넣는다.

| # | 조작 | 기대 결과 |
|---|---|---|
| 1 | A: 로그인 | `Connect Server` **비활성**, `tester (Lv.1) · 접속 가능한 서버가 없습니다` |
| 2 | A: Start Server | 로비 진입. `game_server`에 행 1개, `server_ip`가 A의 주소 |
| 3 | 10초 대기 | `updated_at` 갱신됨 (`age_sec`이 0으로 돌아간다) |
| 4 | B: 로그인 | `Connect Server` **활성**, `접속 가능: <A의 IP>:7777 (tester)` |
| 5 | B: Connect Server | A의 로비에 접속. 접속 인원 2명 |
| 6 | 로비 카운트다운 종료 → `Lvl_ThirdPerson` | **행이 그대로 남고 heartbeat가 계속된다** |
| 7 | A 정상 종료 | 행이 삭제된다 |
| 8 | A 재시작 후 작업 관리자로 **강제 종료** | 행은 남지만 30초 뒤 `GET /server/latest`가 `result: false` |
| 9 | 웹서버를 끄고 로그인 | `서버에 연결할 수 없습니다`. 에디터가 멈추거나 크래시하지 않아야 한다 |

**6번이 가장 중요하다.** 등록 상태와 heartbeat 타이머는 `UWebApiSubsystem`(GameInstance 수명)이 들고 있다. `ALobbyGM::StartGame()`이 `ServerTravel`을 호출해 GameMode가 파괴되어도 등록이 유지돼야 한다. 여기서 행이 사라지면 등록 상태가 잘못된 수명에 묶인 것이다.

**등록이 안 될 때 첫 확인 지점:** `WBP_Title`의 `StartServerButton`. 이 위젯만 `meta = (BindWidget)`이 아니라 `GetWidgetFromName`으로 찾으므로, 이름이 없거나 틀려도 블루프린트 컴파일은 통과하고 런타임에 조용히 아무 일도 하지 않는다.

### 5.5 2대 LAN에서 검증

과제의 실제 취지를 증명하는 구성이다. `server_ip`에 `127.0.0.1`이 아닌 값이 들어가는 걸 확인한다.

- 웹서버 PC 방화벽에서 **8080/TCP 인바운드** 허용
- 게임서버 PC 방화벽에서 **7777/TCP·UDP 인바운드** 허용
- 두 클라이언트 모두 `ServerIP` 칸에 **웹서버 PC의 LAN IP**를 넣는다 (예: `172.21.1.14`)
- 5.4의 2번에서 `game_server.server_ip`가 게임서버 PC의 LAN IP로 들어가야 한다
- 5.4의 4번에서 B가 그 LAN IP를 받아야 한다

LAN IP는 `ipconfig`의 IPv4 주소로 확인한다.

## API

| 메서드 | 경로 | 용도 |
|---|---|---|
| POST | `/signup` | 회원가입 |
| POST | `/login` | 로그인 |
| POST | `/server/register` | 게임서버 등록 (재등록은 UPSERT) |
| POST | `/server/heartbeat` | 생존 갱신 (10초 주기) |
| POST | `/server/unregister` | 등록 해제 |
| GET | `/server/latest` | 접속 가능한 최신 서버 1개 |

**인증 실패도 HTTP 200으로 응답한다.** 성공/실패는 상태코드가 아니라 본문의 `result` 플래그로 판정한다.

**서버 IP는 클라이언트가 보내지 않는다.** 웹서버가 요청의 소스 주소(`request.client.host`)에서 읽는다. 클라이언트가 IP를 위조할 수 없고 LAN에서 자동으로 맞는다.

**활성 판정은 `updated_at`으로만 한다.** heartbeat가 30초 안에 들어온 서버만 `/server/latest`에 나온다. 서버가 크래시나 강제 종료로 `unregister`를 못 불러도 30초 뒤 자동으로 빠진다.

## 세팅에서 걸리는 것들

- **포트 8000을 쓰지 않는다.** `ModelContextProtocol` 플러그인이 `127.0.0.1:8000`을 점유해서 에디터와 웹서버를 동시에 띄울 수 없다. 그래서 8080이다.
- **`uvicorn`을 `--host` 없이 띄우면 `127.0.0.1`에만 바인딩된다.** 다른 PC에서 접속이 안 된다. `run.bat`에 `--host 0.0.0.0`이 들어 있다.
- **`cryptography` 패키지가 필요하다.** MySQL 8 이상의 `caching_sha2_password` 인증에 PyMySQL이 요구한다.
- **PowerShell `Set-Content -Encoding utf8`은 BOM을 붙인다.** SQL 파일을 그렇게 만들면 첫 줄에서 문법 오류가 난다. `mysql < file` 로 넣을 파일은 BOM 없이 저장한다.
- **Git LFS를 쓰지 않는다.** 50MB를 넘는 에셋이 없어 일반 git으로 충분하다. LFS로 관리하면 클론하는 쪽에 git-lfs가 없을 때 `.uasset`이 포인터 파일로 받아져 프로젝트가 열리지 않는다.

## 알려진 한계

이 프로젝트는 실습용이며, 아래는 의도적으로 단순화한 부분이다.

- **비밀번호를 평문으로 저장한다.** 해시를 쓰지 않는다.
- **인증 토큰이 없다.** `/server/register`의 `owner_idx`는 위조할 수 있다. 서버 IP만 웹서버가 판정한다.
- **NAT 뒤의 서버는 등록되지 않는다.** 공유기 내부에서 등록하면 사설 IP가 저장되어 외부에서 접속할 수 없다. 같은 LAN 안에서만 동작한다.
- **서버 목록이 아니라 최신 1개만 노출한다.** DB에는 여러 서버가 쌓이지만 클라이언트는 가장 최근에 갱신된 하나만 받는다.
- **HTTP다.** TLS를 쓰지 않으므로 아이디·비밀번호가 평문으로 흐른다.

## 문서

| 문서 | 내용 |
|---|---|
| `docs/superpowers/plans/2026-08-26-login-signup.md` | 로그인/회원가입 구현 계획 |
| `docs/superpowers/plans/2026-08-27-server-registry.md` | 서버 등록/조회 구현 계획 |
| `docs/작업기록.md` | 로그인/회원가입 작업 기록 |
| `docs/작업기록-server-registry.md` | 서버 등록/조회 작업 기록 |
