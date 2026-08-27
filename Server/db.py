import pymysql

DB_CONFIG = dict(
    host="127.0.0.1",
    port=3306,
    user="root",
    password="qweasd123",
    db="seul",
    charset="utf8mb4",
)


def get_connection():
    return pymysql.connect(**DB_CONFIG, cursorclass=pymysql.cursors.DictCursor)
