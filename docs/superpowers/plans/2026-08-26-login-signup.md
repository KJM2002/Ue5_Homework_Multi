# 로그인 / 회원가입 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** UE5 클라이언트가 Python 웹서버를 통해 MySQL에 로그인/회원가입하고, 로그인에 성공해야만 게임 서버에 접속할 수 있게 한다.

**Architecture:** FastAPI 웹서버가 `/signup`·`/login` 두 엔드포인트를 제공하고 PyMySQL로 `seul.member`를 읽고 쓴다. UE5 쪽은 `UWebApiSubsystem`이 HTTP 호출과 JSON 파싱을 전담하고 결과를 다이나믹 델리게이트로 브로드캐스트한다. `UTitleWidgetBase`는 그 델리게이트를 구독해 화면을 갱신하고, 로그인 전에는 서버 접속 버튼을 잠근다. 인증 성공/실패 여부는 HTTP 상태코드가 아니라 응답 본문의 `result` 플래그로 판정한다.

**Tech Stack:** Python 3.12.10 / FastAPI / Uvicorn / PyMySQL / Pydantic v2 · UE 5.8 C++ (HTTP, Json, JsonUtilities 모듈) · MySQL 8

**Spec:** `docs/superpowers/specs/2026-08-26-login-signup-design.html`

## Global Constraints

- **비밀번호는 평문으로 저장한다.** 실습 프로젝트 전제로 내린 의도적 결정이다. 해시를 임의로 도입하지 말 것.
- **DB 스키마를 변경하지 않는다.** `seul.member`는 `idx`, `user_id`, `passwd`, `nickname`, `level` 컬럼을 가지며 `user_id`에 UNIQUE 인덱스가 이미 존재한다 (2026-08-26 확인).
- **웹서버 포트는 8000 고정.** 클라이언트는 `http://{ServerIP 입력값}:8000` 으로 URL을 조립한다.
  - **[2026-08-26 정정] 8080으로 변경됨.** 언리얼 MCP 플러그인이 `127.0.0.1:8000`을 점유해 서버와 에디터를 동시에 띄울 수 없었다. 이 문서의 나머지 `8000` 표기는 실행 당시 기록이며, 현재 값은 8080이다.
- **인증 실패도 HTTP 200으로 응답한다.** 아이디 중복·비밀번호 불일치는 `200 + {"result": false}` 이다. 401/400을 쓰지 않는다.
- **회원가입 입력은 `user_id`, `passwd` 두 개뿐이다.** 서버가 `nickname = user_id`, `level = 1` 로 채운다. 클라이언트는 `nickname`이나 `level`을 보내지 않는다.
- **가입 후 자동 로그인하지 않는다.** 가입 성공 시 안내 문구만 띄우고, 사용자가 로그인 버튼을 다시 누른다.
- **UE 쪽 로직은 C++로만 작성한다.** 블루프린트에는 위젯 배치만 한다.
- **pytest를 사용하지 않는다.** 서버 검증은 `/docs` Swagger UI + MySQL Workbench 수동 확인이다. 각 서버 태스크의 "검증" 단계가 테스트 사이클을 대신하므로 건너뛰지 말 것.
- **파이썬 패키지는 `Server/.venv` 가상환경에만 설치한다.** 전역 파이썬을 건드리지 않는다.
- **`LobbyWidgetBase.h`를 수정하지 않는다.** `meta = (WidgetBind)` 오타 6개가 남아 있지만 이번 범위 밖이며, 잘못 고치면 `WBP_Lobby` 컴파일이 깨진다.

---

## File Structure

| 파일 | 책임 |
|---|---|
| `Server/db.py` | MySQL 접속 설정과 커넥션 생성. PC마다 달라지는 값이 여기에만 모인다. |
| `Server/main.py` | FastAPI 앱, 요청/응답 모델, `/signup`·`/login` 라우트. |
| `Server/requirements.txt` | 고정 의존성 목록. |
| `Source/.../Web/WebApiSubsystem.h/.cpp` | HTTP 호출, JSON 직렬화/역직렬화, 결과 델리게이트 브로드캐스트. UE에서 서버와 말하는 유일한 지점. |
| `Source/.../DataGameInstanceSubsystem.h` | 로그인 상태와 프로필 값 보관. 통신 로직 없음. |
| `Source/.../Title/TitleWidgetBase.h/.cpp` | 버튼 입력, 입력값 검증, 결과 표시, 서버 접속 게이트. |
| `Source/.../L20260713_Day03.Build.cs` | 모듈 의존성. |

`WebApiSubsystem`(통신)과 `DataGameInstanceSubsystem`(보관)을 나누는 이유는 책임이 다르고 각각 따로 이해·확인할 수 있기 때문이다. 나중에 소켓 서버가 붙어도 통신 코드는 한 곳에 모인다.

---

## Task 1: 파이썬 서버 환경 준비와 DB 연결 확인

이 태스크의 결과물은 "가상환경에서 파이썬이 `seul.member`를 읽을 수 있다"는 사실 하나다. 여기서 DB 접속을 확실히 뚫어두지 않으면 이후 태스크의 실패 원인이 코드인지 접속 설정인지 구분되지 않는다.

**Files:**
- Create: `Server/requirements.txt`
- Create: `Server/db.py`
- Modify: `.gitignore` (끝에 추가)

**Interfaces:**
- Consumes: 없음 (첫 태스크)
- Produces: `db.get_connection() -> pymysql.connections.Connection` — `DictCursor`가 설정된 커넥션. 호출자가 `with` 로 닫는다.

- [ ] **Step 1: 가상환경 생성**

```bash
cd "C:/Work/L20260713_Day03"
python -m venv Server/.venv
```

- [ ] **Step 2: requirements.txt 작성**

`Server/requirements.txt`:

```
fastapi==0.115.6
uvicorn==0.34.0
pymysql==1.1.1
```

- [ ] **Step 3: 패키지 설치**

```bash
Server/.venv/Scripts/python.exe -m pip install -r Server/requirements.txt
```

Expected: `Successfully installed fastapi ... pymysql ... uvicorn ...`

- [ ] **Step 4: .gitignore에 가상환경 제외 추가**

`.gitignore` 맨 끝에 아래 두 줄을 덧붙인다. 기존 내용은 건드리지 않는다.

```
# Python virtual environment
Server/.venv/
```

- [ ] **Step 5: db.py 작성**

`Server/db.py`:

```python
import pymysql

DB_CONFIG = dict(
    host="127.0.0.1",
    port=3306,
    user="root",
    password="CHANGE_ME",
    db="seul",
    charset="utf8mb4",
)


def get_connection():
    return pymysql.connect(**DB_CONFIG, cursorclass=pymysql.cursors.DictCursor)
```

- [ ] **Step 6: 본인 MySQL 비밀번호로 교체**

`DB_CONFIG`의 `password="CHANGE_ME"` 를 실제 MySQL root 비밀번호로 바꾼다. `user`가 root가 아니면 그것도 함께 고친다.

- [ ] **Step 7: DB 연결 검증**

```bash
Server/.venv/Scripts/python.exe -c "import sys; sys.path.insert(0, 'Server'); from db import get_connection; conn = get_connection(); cur = conn.cursor(); cur.execute('SELECT COUNT(*) AS c FROM member'); print('member rows:', cur.fetchone()['c']); conn.close()"
```

Expected: `member rows: 0` (또는 기존 행 수). 숫자가 출력되면 성공.

실패 시 대처:
- `Access denied` → Step 6의 비밀번호/계정이 틀렸다.
- `Unknown database 'seul'` → 스키마 이름을 확인한다.
- `Can't connect` → MySQL 서비스가 꺼져 있다.

- [ ] **Step 8: UNIQUE 인덱스 재확인**

```bash
Server/.venv/Scripts/python.exe -c "import sys; sys.path.insert(0, 'Server'); from db import get_connection; conn = get_connection(); cur = conn.cursor(); cur.execute('SHOW INDEX FROM member'); [print(r['Key_name'], 'Column:', r['Column_name'], 'NonUnique:', r['Non_unique']) for r in cur.fetchall()]; conn.close()"
```

Expected: `user_id` 컬럼에 `Non_unique: 0` 인 인덱스가 존재. 없다면 진행을 멈추고 보고할 것 — Task 2의 중복 검사가 동작하지 않는다.

- [ ] **Step 9: 커밋**

```bash
git add Server/requirements.txt Server/db.py .gitignore
git commit -m "feat(server): 파이썬 서버 환경 및 MySQL 커넥션 추가"
```

---

## Task 2: /signup 엔드포인트

**Files:**
- Create: `Server/main.py`

**Interfaces:**
- Consumes: `db.get_connection()` (Task 1)
- Produces:
  - `AuthRequest` — `user_id: str`, `passwd: str` (둘 다 `min_length=1`)
  - `AuthResponse` — `result: bool`, `message: str = ""`, `idx: int = 0`, `nickname: str = ""`, `level: int = 0`
  - `POST /signup` — `AuthRequest` 를 받아 `AuthResponse` 반환
  - FastAPI 앱 인스턴스 이름은 `app` (uvicorn이 `main:app` 으로 찾는다)

- [ ] **Step 1: main.py 작성**

`Server/main.py`:

```python
from fastapi import FastAPI
from pydantic import BaseModel, Field
import pymysql

from db import get_connection

app = FastAPI(title="L20260713_Day03 Auth Server")


class AuthRequest(BaseModel):
    user_id: str = Field(min_length=1)
    passwd: str = Field(min_length=1)


class AuthResponse(BaseModel):
    result: bool
    message: str = ""
    idx: int = 0
    nickname: str = ""
    level: int = 0


@app.post("/signup", response_model=AuthResponse)
def signup(req: AuthRequest):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            try:
                cur.execute(
                    "INSERT INTO member (user_id, passwd, nickname, level)"
                    " VALUES (%s, %s, %s, 1)",
                    (req.user_id, req.passwd, req.user_id),
                )
            except pymysql.err.IntegrityError:
                return AuthResponse(result=False, message="이미 존재하는 아이디입니다")

            new_idx = cur.lastrowid

        conn.commit()
    finally:
        conn.close()

    return AuthResponse(
        result=True, idx=new_idx, nickname=req.user_id, level=1
    )
```

주의할 점:
- SQL은 반드시 `%s` 파라미터 바인딩을 쓴다. f-string이나 `%` 포매팅으로 값을 끼워넣지 않는다.
- `nickname` 자리에 `req.user_id` 를 넣는 것이 맞다. 오타가 아니다.
- `IntegrityError`는 `pymysql.err` 아래에 있다. `pymysql.IntegrityError` 로 써도 동작하지만 위 형태로 통일한다.

- [ ] **Step 2: 서버 실행**

```bash
cd "C:/Work/L20260713_Day03/Server" && .venv/Scripts/python.exe -m uvicorn main:app --reload
```

Expected: `Uvicorn running on http://127.0.0.1:8000`

이후 단계 동안 이 서버는 계속 켜둔다. `--reload` 덕분에 파일을 고치면 자동 재시작된다.

- [ ] **Step 3: 정상 가입 검증**

브라우저에서 `http://127.0.0.1:8000/docs` 를 연다. `POST /signup` → `Try it out` → 본문에 아래를 넣고 실행:

```json
{ "user_id": "junios", "passwd": "1234" }
```

Expected 응답:

```json
{ "result": true, "message": "", "idx": 1, "nickname": "junios", "level": 1 }
```

(`idx`는 기존 행 수에 따라 달라질 수 있다.)

- [ ] **Step 4: DB에 실제로 들어갔는지 확인**

MySQL Workbench에서:

```sql
SELECT * FROM seul.member;
```

Expected: `user_id = junios`, `passwd = 1234`, `nickname = junios`, `level = 1` 인 행 1개.

- [ ] **Step 5: 중복 가입 검증**

`/docs`에서 Step 3과 **완전히 동일한 본문**으로 다시 실행.

Expected 응답:

```json
{ "result": false, "message": "이미 존재하는 아이디입니다", "idx": 0, "nickname": "", "level": 0 }
```

이어서 Workbench에서 `SELECT COUNT(*) FROM seul.member;` → **행 수가 늘지 않았어야 한다.** 늘었다면 UNIQUE 인덱스가 없는 것이므로 멈추고 보고할 것.

- [ ] **Step 6: 빈 값 검증**

`/docs`에서 `{ "user_id": "", "passwd": "1234" }` 로 실행.

Expected: HTTP `422 Unprocessable Entity`. Pydantic의 `min_length=1`이 막은 것이며 정상 동작이다.

- [ ] **Step 7: 커밋**

```bash
git add Server/main.py
git commit -m "feat(server): 회원가입 엔드포인트 추가"
```

---

## Task 3: /login 엔드포인트

**Files:**
- Modify: `Server/main.py` (파일 끝에 추가)

**Interfaces:**
- Consumes: `AuthRequest`, `AuthResponse`, `get_connection()` (Task 2)
- Produces: `POST /login` — `AuthRequest` 를 받아 `AuthResponse` 반환

- [ ] **Step 1: /login 라우트 추가**

`Server/main.py` 맨 끝에 아래를 덧붙인다. 기존 `signup` 함수는 건드리지 않는다.

```python
@app.post("/login", response_model=AuthResponse)
def login(req: AuthRequest):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                "SELECT idx, nickname, level FROM member"
                " WHERE user_id = %s AND passwd = %s",
                (req.user_id, req.passwd),
            )
            row = cur.fetchone()
    finally:
        conn.close()

    if row is None:
        return AuthResponse(
            result=False, message="아이디 또는 비밀번호가 올바르지 않습니다"
        )

    return AuthResponse(
        result=True,
        idx=row["idx"],
        nickname=row["nickname"],
        level=row["level"],
    )
```

- [ ] **Step 2: 정상 로그인 검증**

`/docs` → `POST /login`:

```json
{ "user_id": "junios", "passwd": "1234" }
```

Expected:

```json
{ "result": true, "message": "", "idx": 1, "nickname": "junios", "level": 1 }
```

- [ ] **Step 3: 틀린 비밀번호 검증**

```json
{ "user_id": "junios", "passwd": "wrong" }
```

Expected:

```json
{ "result": false, "message": "아이디 또는 비밀번호가 올바르지 않습니다", "idx": 0, "nickname": "", "level": 0 }
```

HTTP 상태코드가 **200** 인지 반드시 확인한다. 401이 나오면 잘못 구현한 것이다.

- [ ] **Step 4: 없는 아이디 검증**

```json
{ "user_id": "nobody", "passwd": "1234" }
```

Expected: Step 3과 동일한 응답. 아이디가 없는 경우와 비밀번호가 틀린 경우를 구분해서 알려주지 않는 것이 의도된 동작이다.

- [ ] **Step 5: 커밋**

```bash
git add Server/main.py
git commit -m "feat(server): 로그인 엔드포인트 추가"
```

---

## Task 4: UE 모듈 의존성과 데이터 보관 필드

서버가 완전히 동작하는 것을 확인한 뒤에 UE 쪽으로 넘어온다. 이 태스크는 컴파일이 통과하는 것까지가 결과물이다.

**Files:**
- Modify: `Source/L20260713_Day03/L20260713_Day03.Build.cs:11-20`
- Modify: `Source/L20260713_Day03/DataGameInstanceSubsystem.h:22-30`

**Interfaces:**
- Consumes: 없음
- Produces: `UDataGameInstanceSubsystem` 에 `bLoggedIn: bool`, `Idx: int32`, `Nickname: FString`, `Level: int32` 추가. 기존 `UserID`, `Password`, `ServerIP` 는 그대로 둔다.

- [ ] **Step 1: Build.cs에 모듈 추가**

`PublicDependencyModuleNames.AddRange` 블록의 `"UMG"` 뒤에 세 줄을 추가한다. 결과는 아래와 같아야 한다.

```csharp
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
            "EnhancedInput",
            "AnimGraphRuntime",
			"UMG",
			"HTTP",
			"Json",
			"JsonUtilities"

        });
```

`HTTP`는 대문자다. `Http`로 쓰면 링크 에러가 난다.

- [ ] **Step 2: DataGameInstanceSubsystem에 필드 추가**

`ServerIP` 선언 아래, 클래스 닫는 중괄호 앞에 추가한다.

```cpp
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	bool bLoggedIn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	int32 Idx = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	int32 Level = 0;
```

- [ ] **Step 3: 컴파일 검증**

Visual Studio에서 솔루션을 빌드하거나, 언리얼 에디터에서 Live Coding (`Ctrl+Alt+F11`)을 실행한다.

Expected: 에러 0개. Build.cs를 고쳤으므로 에디터를 완전히 껐다 켜야 할 수도 있다.

- [ ] **Step 4: 커밋**

```bash
git add Source/L20260713_Day03/L20260713_Day03.Build.cs Source/L20260713_Day03/DataGameInstanceSubsystem.h
git commit -m "feat(client): HTTP/Json 모듈 의존성 및 로그인 상태 필드 추가"
```

---

## Task 5: WebApiSubsystem

**Files:**
- Create: `Source/L20260713_Day03/Web/WebApiSubsystem.h`
- Create: `Source/L20260713_Day03/Web/WebApiSubsystem.cpp`

**Interfaces:**
- Consumes: `UDataGameInstanceSubsystem` 의 `bLoggedIn`, `Idx`, `Nickname`, `Level` (Task 4)
- Produces:
  - `FWebApiResultSignature` — `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWebApiResultSignature, const bool, bInSuccess, const FString&, InMessage)`
  - `UWebApiSubsystem::OnLoginResult`, `UWebApiSubsystem::OnSignUpResult` — 위 시그니처의 델리게이트
  - `void RequestLogin(const FString& InServerIP, const FString& InUserID, const FString& InPassword)`
  - `void RequestSignUp(const FString& InServerIP, const FString& InUserID, const FString& InPassword)`
  - 로그인 성공 시 `UDataGameInstanceSubsystem` 의 `bLoggedIn = true` 와 프로필 3개를 채운 **뒤에** 브로드캐스트한다. 회원가입 성공 시에는 아무것도 저장하지 않는다.

- [ ] **Step 1: 헤더 작성**

`Source/L20260713_Day03/Web/WebApiSubsystem.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "WebApiSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWebApiResultSignature, const bool, bInSuccess, const FString&, InMessage);

/**
 * 웹서버와의 HTTP 통신을 전담한다. 결과는 델리게이트로만 알린다.
 */
UCLASS()
class L20260713_DAY03_API UWebApiSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "WebApi")
	FWebApiResultSignature OnLoginResult;

	UPROPERTY(BlueprintAssignable, Category = "WebApi")
	FWebApiResultSignature OnSignUpResult;

	void RequestLogin(const FString& InServerIP, const FString& InUserID, const FString& InPassword);

	void RequestSignUp(const FString& InServerIP, const FString& InUserID, const FString& InPassword);

private:

	void SendAuthRequest(const FString& InServerIP, const FString& InPath,
		const FString& InUserID, const FString& InPassword,
		FWebApiResultSignature& InDelegate, const bool bInIsLogin);

	void HandleAuthResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
		FWebApiResultSignature& InDelegate, const bool bInIsLogin);
};
```

`Interfaces/IHttpRequest.h` 가 `FHttpResponsePtr`, `FHttpRequestRef` 타입 별칭을 제공한다. `HttpFwd.h` 로 더 가볍게 갈 수도 있지만 엔진 버전에 따라 경로가 다를 수 있어 확실한 쪽을 택한다.

- [ ] **Step 2: 구현부 작성**

`Source/L20260713_Day03/Web/WebApiSubsystem.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "WebApiSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "../DataGameInstanceSubsystem.h"

namespace
{
	constexpr int32 WebServerPort = 8000;
}

void UWebApiSubsystem::RequestLogin(const FString& InServerIP, const FString& InUserID, const FString& InPassword)
{
	SendAuthRequest(InServerIP, TEXT("/login"), InUserID, InPassword, OnLoginResult, true);
}

void UWebApiSubsystem::RequestSignUp(const FString& InServerIP, const FString& InUserID, const FString& InPassword)
{
	SendAuthRequest(InServerIP, TEXT("/signup"), InUserID, InPassword, OnSignUpResult, false);
}

void UWebApiSubsystem::SendAuthRequest(const FString& InServerIP, const FString& InPath,
	const FString& InUserID, const FString& InPassword,
	FWebApiResultSignature& InDelegate, const bool bInIsLogin)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("user_id"), InUserID);
	JsonObject->SetStringField(TEXT("passwd"), InPassword);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject, Writer);

	const FString Url = FString::Printf(TEXT("http://%s:%d%s"), *InServerIP, WebServerPort, *InPath);

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	// 응답이 도착하기 전에 GameInstance가 정리될 수 있으므로 약참조로 잡는다.
	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);
	FWebApiResultSignature* DelegatePtr = &InDelegate;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, DelegatePtr, bInIsLogin](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->HandleAuthResponse(InResponse, bInConnectedSuccessfully, *DelegatePtr, bInIsLogin);
		});

	Request->ProcessRequest();
}

void UWebApiSubsystem::HandleAuthResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
	FWebApiResultSignature& InDelegate, const bool bInIsLogin)
{
	if (!bInConnectedSuccessfully || !InResponse.IsValid())
	{
		InDelegate.Broadcast(false, TEXT("서버에 연결할 수 없습니다"));
		return;
	}

	const int32 ResponseCode = InResponse->GetResponseCode();
	if (ResponseCode != 200)
	{
		InDelegate.Broadcast(false, FString::Printf(TEXT("요청을 처리할 수 없습니다 (코드 %d)"), ResponseCode));
		return;
	}

	const FString ResponseBody = InResponse->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		InDelegate.Broadcast(false, TEXT("응답을 해석할 수 없습니다"));
		return;
	}

	if (!JsonObject->GetBoolField(TEXT("result")))
	{
		InDelegate.Broadcast(false, JsonObject->GetStringField(TEXT("message")));
		return;
	}

	if (bInIsLogin)
	{
		UDataGameInstanceSubsystem* Data = GetGameInstance()->GetSubsystem<UDataGameInstanceSubsystem>();
		if (Data)
		{
			Data->Idx = JsonObject->GetIntegerField(TEXT("idx"));
			Data->Nickname = JsonObject->GetStringField(TEXT("nickname"));
			Data->Level = JsonObject->GetIntegerField(TEXT("level"));
			Data->bLoggedIn = true;
		}
	}

	InDelegate.Broadcast(true, TEXT(""));
}
```

- [ ] **Step 3: 컴파일 검증**

Live Coding 또는 Visual Studio 빌드.

Expected: 에러 0개.

`unresolved external symbol` 이 뜨면 Task 4 Step 1의 `"HTTP"` 모듈이 빠졌거나 에디터가 옛 Build.cs를 쓰고 있는 것이다. 에디터를 껐다 켜고 다시 빌드한다.

- [ ] **Step 4: 커밋**

```bash
git add Source/L20260713_Day03/Web/
git commit -m "feat(client): 웹 API 호출 서브시스템 추가"
```

---

## Task 6: TitleWidgetBase 로그인 게이트

**Files:**
- Modify: `Source/L20260713_Day03/Title/TitleWidgetBase.h`
- Modify: `Source/L20260713_Day03/Title/TitleWidgetBase.cpp`

**Interfaces:**
- Consumes: `UWebApiSubsystem::RequestLogin/RequestSignUp`, `OnLoginResult`, `OnSignUpResult` (Task 5) · `UDataGameInstanceSubsystem::bLoggedIn/Nickname/Level` (Task 4)
- Produces: 새 위젯 프로퍼티 `LoginButton`, `SignUpButton`, `InfoText` — Task 7에서 블루프린트에 같은 이름으로 배치해야 한다.

- [ ] **Step 1: 헤더 수정**

`TitleWidgetBase.h` 의 전방 선언에 한 줄 추가:

```cpp
class UButton;
class UEditableTextBox;
class UTextBlock;
class UWebApiSubsystem;
```

`ServerIP` 선언 아래에 새 위젯 3개를 추가:

```cpp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UButton> SignUpButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UTextBlock> InfoText;
```

`void SaveData();` 아래에 함수들을 추가:

```cpp
	UFUNCTION()
	void Login();

	UFUNCTION()
	void SignUp();

	UFUNCTION()
	void ProcessLoginResult(const bool bInSuccess, const FString& InMessage);

	UFUNCTION()
	void ProcessSignUpResult(const bool bInSuccess, const FString& InMessage);

private:

	UWebApiSubsystem* GetWebApi() const;

	bool IsLoggedIn() const;

	bool ValidateInput();

	void SetInfoText(const FString& InMessage);
```

`ProcessLoginResult`/`ProcessSignUpResult` 는 다이나믹 델리게이트에 바인딩되므로 `UFUNCTION()`이 반드시 필요하다. 빠뜨리면 런타임에 조용히 바인딩되지 않는다.

- [ ] **Step 2: cpp에 include 추가**

`TitleWidgetBase.cpp` 상단 include 목록에 추가:

```cpp
#include "Components/TextBlock.h"
#include "../Web/WebApiSubsystem.h"
```

- [ ] **Step 3: 헬퍼 3개 구현**

`TitleWidgetBase.cpp` 맨 끝에 추가:

```cpp
UWebApiSubsystem* UTitleWidgetBase::GetWebApi() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWebApiSubsystem>() : nullptr;
}

bool UTitleWidgetBase::IsLoggedIn() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	UDataGameInstanceSubsystem* Data = GI->GetSubsystem<UDataGameInstanceSubsystem>();
	return Data && Data->bLoggedIn;
}

void UTitleWidgetBase::SetInfoText(const FString& InMessage)
{
	if (InfoText)
	{
		InfoText->SetText(FText::FromString(InMessage));
	}
}

bool UTitleWidgetBase::ValidateInput()
{
	if (!UserID || !Password || !ServerIP)
	{
		return false;
	}

	if (UserID->GetText().IsEmptyOrWhitespace() || Password->GetText().IsEmptyOrWhitespace())
	{
		SetInfoText(TEXT("아이디와 비밀번호를 입력해 주세요"));
		return false;
	}

	if (ServerIP->GetText().IsEmptyOrWhitespace())
	{
		SetInfoText(TEXT("서버 주소를 입력해 주세요"));
		return false;
	}

	return true;
}
```

- [ ] **Step 4: NativeConstruct 수정**

기존 `NativeConstruct` 를 아래로 교체한다. `StartServerButton` 의 `GetWidgetFromName` 줄은 그대로 유지한다 (이 프로퍼티에는 `BindWidget` 이 없다).

```cpp
void UTitleWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();


	StartServerButton = Cast<UButton>(GetWidgetFromName(TEXT("StartServerButton")));
	if (StartServerButton)
	{
		StartServerButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::StartServer);
		StartServerButton->SetIsEnabled(false);
	}

	if (ConnectServerButton)
	{
		ConnectServerButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::ConnectServer);
		ConnectServerButton->SetIsEnabled(false);
	}

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::Login);
	}

	if (SignUpButton)
	{
		SignUpButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::SignUp);
	}

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		WebApi->OnLoginResult.AddUniqueDynamic(this, &UTitleWidgetBase::ProcessLoginResult);
		WebApi->OnSignUpResult.AddUniqueDynamic(this, &UTitleWidgetBase::ProcessSignUpResult);
	}
}
```

서브시스템 델리게이트에는 `AddDynamic` 이 아니라 `AddUniqueDynamic` 을 쓴다. 서브시스템은 위젯보다 오래 살기 때문에, 위젯이 다시 만들어질 때 `AddDynamic` 이면 중복 바인딩된다.

- [ ] **Step 5: Login / SignUp 구현**

```cpp
void UTitleWidgetBase::Login()
{
	if (!ValidateInput())
	{
		return;
	}

	SaveData();

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		SetInfoText(TEXT("로그인 중..."));
		WebApi->RequestLogin(ServerIP->GetText().ToString(),
			UserID->GetText().ToString(),
			Password->GetText().ToString());
	}
}

void UTitleWidgetBase::SignUp()
{
	if (!ValidateInput())
	{
		return;
	}

	SaveData();

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		SetInfoText(TEXT("가입 중..."));
		WebApi->RequestSignUp(ServerIP->GetText().ToString(),
			UserID->GetText().ToString(),
			Password->GetText().ToString());
	}
}
```

- [ ] **Step 6: 결과 처리 구현**

```cpp
void UTitleWidgetBase::ProcessLoginResult(const bool bInSuccess, const FString& InMessage)
{
	if (!bInSuccess)
	{
		SetInfoText(InMessage);
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UDataGameInstanceSubsystem* Data = GI ? GI->GetSubsystem<UDataGameInstanceSubsystem>() : nullptr;
	if (Data)
	{
		SetInfoText(FString::Printf(TEXT("%s (Lv.%d)"), *Data->Nickname, Data->Level));
	}

	if (StartServerButton)
	{
		StartServerButton->SetIsEnabled(true);
	}

	if (ConnectServerButton)
	{
		ConnectServerButton->SetIsEnabled(true);
	}
}

void UTitleWidgetBase::ProcessSignUpResult(const bool bInSuccess, const FString& InMessage)
{
	SetInfoText(bInSuccess ? TEXT("가입이 완료되었습니다. 로그인해 주세요.") : InMessage);
}
```

- [ ] **Step 7: StartServer / ConnectServer에 게이트 추가**

두 함수의 맨 앞에 로그인 검사를 넣는다. 나머지 본문은 그대로 둔다.

```cpp
void UTitleWidgetBase::StartServer()
{
	if (!IsLoggedIn())
	{
		SetInfoText(TEXT("먼저 로그인해 주세요"));
		return;
	}

	SaveData();

	UGameplayStatics::OpenLevel(GetWorld(),
		TEXT("Lobby"),
		true,
		TEXT("Listen")
	);
}

void UTitleWidgetBase::ConnectServer()
{
	if (!IsLoggedIn())
	{
		SetInfoText(TEXT("먼저 로그인해 주세요"));
		return;
	}

	SaveData();

	UGameplayStatics::OpenLevel(GetWorld(),
		FName(ServerIP->GetText().ToString()),
		true,
		TEXT("Key=100")
	);
}
```

버튼 비활성화가 1차 방어이고 이 검사가 2차 방어다. 둘 다 둔다.

- [ ] **Step 8: 컴파일 검증**

Live Coding 또는 Visual Studio 빌드.

Expected: 에러 0개.

- [ ] **Step 9: 커밋**

```bash
git add Source/L20260713_Day03/Title/TitleWidgetBase.h Source/L20260713_Day03/Title/TitleWidgetBase.cpp
git commit -m "feat(client): 타이틀 화면 로그인 게이트 추가"
```

---

## Task 7: 블루프린트 배치와 통합 검증

**Files:**
- Modify: `Content/Blueprints/Title/UI/WBP_Title.uasset` (에디터 작업)

**Interfaces:**
- Consumes: Task 6의 위젯 프로퍼티 이름들
- Produces: 동작하는 로그인 화면

- [ ] **Step 1: WBP_Title에 위젯 배치**

언리얼 에디터에서 `Content/Blueprints/Title/UI/WBP_Title` 을 연다. 디자이너에 아래를 추가하고 **이름을 정확히** 맞춘다. 대소문자까지 일치해야 한다.

| 위젯 타입 | 이름 |
|---|---|
| Button | `LoginButton` |
| Button | `SignUpButton` |
| Text | `InfoText` |

버튼 안에 라벨용 Text를 넣는 경우, 그 Text의 이름은 아무거나 상관없다. 이름을 맞춰야 하는 것은 Button 자체다.

- [ ] **Step 2: 기존 위젯 이름 확인**

Task 4에서 `BindWidget` 지정자를 고쳤기 때문에, 아래 4개가 `WBP_Title` 에 **정확히 같은 이름으로 존재해야** 블루프린트가 컴파일된다.

`ConnectServerButton`, `UserID`, `Password`, `ServerIP`

블루프린트를 컴파일했을 때 `A required widget binding "X" of type Y was not found` 에러가 나면, 해당 위젯의 이름이 다른 것이다. 디자이너에서 이름을 바꿔 맞춘다.

- [ ] **Step 3: 블루프린트 컴파일**

`WBP_Title` 에서 Compile 버튼.

Expected: 에러 0개.

- [ ] **Step 4: 로그인 전 게이트 검증**

서버를 켜둔 상태로 PIE 실행.

Expected: StartServer / ConnectServer 버튼이 **회색으로 비활성**. 클릭해도 레벨이 바뀌지 않는다.

- [ ] **Step 5: 틀린 비밀번호 검증**

`ServerIP`에 `127.0.0.1`, `UserID`에 `junios`, `Password`에 `wrong` 을 넣고 로그인 클릭.

Expected: `InfoText`에 `아이디 또는 비밀번호가 올바르지 않습니다`. 두 버튼은 여전히 비활성.

- [ ] **Step 6: 정상 로그인 검증**

`Password`를 `1234` 로 고치고 로그인 클릭.

Expected: `InfoText`에 `junios (Lv.1)`. StartServer / ConnectServer 버튼이 활성화됨.

- [ ] **Step 7: 회원가입 검증**

PIE를 재시작하고, `UserID`에 새 아이디(예: `tester2`), `Password`에 `1234` 를 넣고 회원가입 클릭.

Expected: `InfoText`에 `가입이 완료되었습니다. 로그인해 주세요.` 버튼은 **여전히 비활성** (자동 로그인하지 않는다). Workbench에서 `SELECT * FROM seul.member;` 로 행이 추가됐는지 확인.

이어서 같은 아이디로 회원가입을 한 번 더 클릭 → `이미 존재하는 아이디입니다`.

- [ ] **Step 8: 서버 접속 검증**

Step 6 상태(로그인 성공)에서 StartServer 클릭.

Expected: Lobby 레벨로 이동한다. 기존 동작이 그대로 유지되는지 확인하는 단계다.

- [ ] **Step 9: 서버 장애 검증**

uvicorn을 `Ctrl+C` 로 끈다. PIE를 재시작하고 로그인 클릭.

Expected: `InfoText`에 `서버에 연결할 수 없습니다`. 에디터가 멈추거나 크래시하지 않아야 한다.

- [ ] **Step 10: 커밋**

```bash
git add Content/Blueprints/Title/UI/WBP_Title.uasset
git commit -m "feat(client): 타이틀 화면에 로그인/회원가입 위젯 배치"
```

---

## 완료 기준

아래가 전부 충족되면 이 계획은 완료다.

1. `Server/.venv` 환경에서 uvicorn이 뜨고 `/docs` 가 열린다.
2. `/signup` 이 새 계정을 만들고, 중복 아이디는 `result: false` 로 거부하며 행이 늘지 않는다.
3. `/login` 이 정상 계정에 `result: true` + 프로필을, 틀린 자격증명에 `result: false` 를 **HTTP 200으로** 반환한다.
4. UE 프로젝트가 에러 없이 컴파일되고 `WBP_Title` 이 컴파일된다.
5. 로그인 전에는 StartServer / ConnectServer 가 비활성이다.
6. 정상 로그인 시 `닉네임 (Lv.N)` 이 표시되고 두 버튼이 활성화되며, StartServer가 기존대로 Lobby로 이동한다.
7. 서버가 꺼져 있을 때 로그인하면 안내 문구가 뜨고 크래시하지 않는다.

## 이번 범위 밖

- 소켓 서버
- 세션/토큰 인증, 로그아웃
- 비밀번호 해시, HTTPS
- pytest 자동 테스트
- UE 리슨 서버가 접속자의 로그인 여부를 재검증하는 것
- `level` 값을 게임 로직에서 사용하는 것
- `LobbyWidgetBase.h` 의 `meta = (WidgetBind)` 오타 6개
