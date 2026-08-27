-- L20260713_Day03 인증/서버 등록 스키마
--
-- 적용:
--   mysql -u root -p < schema.sql
--
-- 비밀번호를 평문으로 저장한다. 실습 프로젝트 전제로 내린 의도적 결정이다.

CREATE DATABASE IF NOT EXISTS seul DEFAULT CHARSET utf8mb4;

USE seul;

-- 회원. user_id로 로그인하고 nickname은 가입 시 user_id를 그대로 채운다.
CREATE TABLE IF NOT EXISTS member (
  idx      INT AUTO_INCREMENT PRIMARY KEY,
  user_id  VARCHAR(50)  NOT NULL UNIQUE,
  passwd   VARCHAR(100) NOT NULL,
  nickname VARCHAR(50)  NOT NULL,
  level    INT          NOT NULL DEFAULT 1
) DEFAULT CHARSET utf8mb4;

-- 떠 있는 게임서버. server_ip는 클라이언트가 보내지 않고 웹서버가
-- 요청의 소스 주소(request.client.host)에서 읽어 채운다.
--
-- 활성 판정은 updated_at으로만 한다. 서버가 크래시나 강제 종료로
-- /server/unregister를 못 불러도 30초 뒤 조회에서 자동으로 빠진다.
CREATE TABLE IF NOT EXISTS game_server (
  idx        INT AUTO_INCREMENT PRIMARY KEY,
  server_ip  VARCHAR(45)  NOT NULL,               -- IPv6 표기 대비 45
  port       INT          NOT NULL DEFAULT 7777,
  owner_idx  INT          NOT NULL,               -- member.idx
  created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
                          ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uk_addr (server_ip, port)
) DEFAULT CHARSET utf8mb4;
