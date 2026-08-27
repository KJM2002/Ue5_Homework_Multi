from fastapi import FastAPI, Request
from pydantic import BaseModel, Field
import pymysql

from db import get_connection

app = FastAPI(title="L20260713_Day03 Auth Server")

# 언리얼 리슨 서버 기본 포트.
DEFAULT_GAME_PORT = 7777

# heartbeat가 이 시간 안에 들어온 서버만 접속 대상으로 본다.
# 클라이언트 heartbeat 주기(10초)의 3배로 잡아 두 번 놓쳐도 버틴다.
ACTIVE_WINDOW_SECONDS = 30


class AuthRequest(BaseModel):
    user_id: str = Field(min_length=1)
    passwd: str = Field(min_length=1)


class AuthResponse(BaseModel):
    result: bool
    message: str = ""
    idx: int = 0
    nickname: str = ""
    level: int = 0


class ServerRequest(BaseModel):
    owner_idx: int = 0
    port: int = DEFAULT_GAME_PORT


class ServerResponse(BaseModel):
    result: bool
    message: str = ""
    server_ip: str = ""
    port: int = 0
    nickname: str = ""


def get_client_ip(request: Request) -> str:
    """서버 주소는 클라이언트를 믿지 않고 요청의 소스 주소에서 읽는다."""
    return request.client.host if request.client else ""


def normalize_port(port: int) -> int:
    return port if port > 0 else DEFAULT_GAME_PORT


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


@app.post("/server/register", response_model=ServerResponse)
def register_server(req: ServerRequest, request: Request):
    server_ip = get_client_ip(request)
    if not server_ip:
        return ServerResponse(result=False, message="서버 주소를 확인할 수 없습니다")

    port = normalize_port(req.port)

    conn = get_connection()
    try:
        with conn.cursor() as cur:
            # 같은 (server_ip, port)로 다시 등록하면 행을 늘리지 않고 갱신한다.
            cur.execute(
                "INSERT INTO game_server (server_ip, port, owner_idx)"
                " VALUES (%s, %s, %s)"
                " ON DUPLICATE KEY UPDATE"
                " owner_idx = VALUES(owner_idx), updated_at = NOW()",
                (server_ip, port, req.owner_idx),
            )
        conn.commit()
    finally:
        conn.close()

    return ServerResponse(result=True, server_ip=server_ip, port=port)


@app.post("/server/heartbeat", response_model=ServerResponse)
def heartbeat_server(req: ServerRequest, request: Request):
    server_ip = get_client_ip(request)
    if not server_ip:
        return ServerResponse(result=False, message="서버 주소를 확인할 수 없습니다")

    port = normalize_port(req.port)

    conn = get_connection()
    try:
        with conn.cursor() as cur:
            # UPDATE의 영향 행 수로 존재를 판정하면, 같은 초에 두 번 들어온
            # 요청이 "변경 없음 = 0행"이 되어 미등록으로 오판된다. 먼저 조회한다.
            cur.execute(
                "SELECT idx FROM game_server WHERE server_ip = %s AND port = %s",
                (server_ip, port),
            )
            if cur.fetchone() is None:
                return ServerResponse(
                    result=False,
                    message="등록되지 않은 서버입니다",
                    server_ip=server_ip,
                    port=port,
                )

            cur.execute(
                "UPDATE game_server SET updated_at = NOW()"
                " WHERE server_ip = %s AND port = %s",
                (server_ip, port),
            )
        conn.commit()
    finally:
        conn.close()

    return ServerResponse(result=True, server_ip=server_ip, port=port)


@app.post("/server/unregister", response_model=ServerResponse)
def unregister_server(req: ServerRequest, request: Request):
    server_ip = get_client_ip(request)
    if not server_ip:
        return ServerResponse(result=False, message="서버 주소를 확인할 수 없습니다")

    port = normalize_port(req.port)

    conn = get_connection()
    try:
        with conn.cursor() as cur:
            deleted = cur.execute(
                "DELETE FROM game_server WHERE server_ip = %s AND port = %s",
                (server_ip, port),
            )
        conn.commit()
    finally:
        conn.close()

    if deleted == 0:
        return ServerResponse(
            result=False,
            message="등록되지 않은 서버입니다",
            server_ip=server_ip,
            port=port,
        )

    return ServerResponse(result=True, server_ip=server_ip, port=port)


@app.get("/server/latest", response_model=ServerResponse)
def latest_server():
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                "SELECT g.server_ip, g.port, m.nickname"
                " FROM game_server g"
                " LEFT JOIN member m ON m.idx = g.owner_idx"
                " WHERE g.updated_at >= NOW() - INTERVAL %s SECOND"
                " ORDER BY g.updated_at DESC LIMIT 1",
                (ACTIVE_WINDOW_SECONDS,),
            )
            row = cur.fetchone()
    finally:
        conn.close()

    if row is None:
        return ServerResponse(result=False, message="접속 가능한 서버가 없습니다")

    return ServerResponse(
        result=True,
        server_ip=row["server_ip"],
        port=row["port"],
        nickname=row["nickname"] or "",
    )
