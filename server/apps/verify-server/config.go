package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
)

type Config struct {
	Email struct {
		User string `json:"user"`
		Pass string `json:"pass"`
	} `json:"email"`
	MySQL struct {
		Host string `json:"host"`
		Port int    `json:"port"`
	} `json:"mysql"`
	Redis struct {
		Host   string `json:"host"`
		Port   int    `json:"port"`
		Passwd string `json:"passwd"`
	} `json:"redis"`
	Server struct {
		Host string `json:"host"`
		Port int    `json:"port"`
	} `json:"server"`
}

var GlobalConfig Config

var (
	EmailUser   string
	EmailPass   string
	MySQLHost   string
	MySQLPort   int
	RedisHost   string
	RedisPort   int
	RedisPasswd string
	ServerHost  string
	ServerPort  int
)

func InitConfig() error {
	configPath := os.Getenv("VERIFY_CONFIG_FILE")
	if configPath == "" {
		configPath = "config.json"
	}

	data, err := os.ReadFile(configPath)
	if err != nil {
		return fmt.Errorf("读取配置文件失败 %s: %w", configPath, err)
	}

	if err := json.Unmarshal(data, &GlobalConfig); err != nil {
		return fmt.Errorf("解析配置文件失败 %s: %w", configPath, err)
	}

	EmailUser = GlobalConfig.Email.User
	EmailPass = GlobalConfig.Email.Pass
	MySQLHost = GlobalConfig.MySQL.Host
	MySQLPort = GlobalConfig.MySQL.Port
	RedisHost = GlobalConfig.Redis.Host
	RedisPort = GlobalConfig.Redis.Port
	RedisPasswd = GlobalConfig.Redis.Passwd
	ServerHost = GlobalConfig.Server.Host
	ServerPort = GlobalConfig.Server.Port

	log.Printf("配置加载成功: 服务将启动在 %s:%d", ServerHost, ServerPort)
	return nil
}

func GetServerAddr() string {
	return fmt.Sprintf("%s:%d", ServerHost, ServerPort)
}
