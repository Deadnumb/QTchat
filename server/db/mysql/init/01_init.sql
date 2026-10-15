-- ChatServer 数据库初始化脚本
-- mysql 容器首次启动时由 docker-entrypoint-initdb.d 自动执行
--
-- 注意：下表结构与存储过程是基于代码推演的骨架，
--       请与你本地已有的 chatdb 结构核对后按需调整。

-- 统一会话字符序/排序规则：MySQL 8 的 CLI 默认 collation_connection 是
-- utf8mb4_0900_ai_ci，而建表用的是 utf8mb4_unicode_ci，存储过程参数若带上
-- 0900 排序规则，WHERE 比较会报 Illegal mix of collations 并被异常处理器吞掉。
SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE DATABASE IF NOT EXISTS chatdb
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;
USE chatdb;

CREATE TABLE IF NOT EXISTS `user` (
    `uid`   INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`  VARCHAR(64)  NOT NULL,
    `email` VARCHAR(128) NOT NULL,
    `pwd`   VARCHAR(255) NOT NULL,
    PRIMARY KEY (`uid`),
    UNIQUE KEY `uk_name`  (`name`),
    UNIQUE KEY `uk_email` (`email`)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- 与 MySQLDao::RegUser 中的 "CALL reg_user(?,?,?,@result)" 对应：
--   成功 -> 返回新 uid；用户名/邮箱已存在 -> 返回 0
DELIMITER $$
DROP PROCEDURE IF EXISTS reg_user $$
CREATE PROCEDURE reg_user(
    IN  in_name  VARCHAR(64),
    IN  in_email VARCHAR(128),
    IN  in_pwd   VARCHAR(255),
    OUT result   INT
)
BEGIN
    DECLARE cnt INT DEFAULT 0;
    SELECT COUNT(*) INTO cnt FROM `user` WHERE `name` = in_name OR `email` = in_email;
    IF cnt > 0 THEN
        SET result = 0;
    ELSE
        INSERT INTO `user` (`name`, `email`, `pwd`) VALUES (in_name, in_email, in_pwd);
        SET result = LAST_INSERT_ID();
    END IF;
END $$
DELIMITER ;
