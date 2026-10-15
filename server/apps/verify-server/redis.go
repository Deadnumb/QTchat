package main

import (
	"context"
	"fmt"
	"log"

	"github.com/redis/go-redis/v9"
)

var RedisCli *redis.Client

func InitRedis() {
	RedisCli = redis.NewClient(&redis.Options{
		Addr:     fmt.Sprintf("%s:%d", RedisHost, RedisPort),
		Password: RedisPasswd,
		DB:       0,
	})

	ctx := context.Background()
	if err := RedisCli.Ping(ctx).Err(); err != nil {
		log.Printf("Redis连接失败: %v", err)
		RedisCli = nil
	} else {
		log.Printf("Redis连接成功: %s:%d", RedisHost, RedisPort)
	}
}

func CloseRedis() {
	if RedisCli != nil {
		RedisCli.Close()
	}
}
