package main

import (
	"context"
	"crypto/rand"
	"fmt"
	"log"
	"net"
	"time"

	pb "github.com/deadnumb/cppchat/apps/verify-server/proto"

	"google.golang.org/grpc"
)

// 实现服务接口
type server struct {
	pb.UnimplementedVerifyServiceServer
}

func (s *server) GetVerifyCode(ctx context.Context, req *pb.GetVerifyReq) (*pb.GetVerifyRsp, error) {
	email := req.Email
	log.Printf("收到请求，email: %s", email)

	code := generateCode()
	log.Printf("生成验证码: %s", code)

	if RedisCli == nil {
		log.Printf("Redis未初始化")
		return &pb.GetVerifyRsp{Email: email, Error: RedisErr}, nil
	}
	if err := RedisCli.Set(ctx, CodePrefix+email, code, 180*time.Second).Err(); err != nil {
		log.Printf("存储验证码到Redis失败 [%s] : %v", email, err)
		return &pb.GetVerifyRsp{Email: email, Error: RedisErr}, nil
	}

	textStr := fmt.Sprintf("您的验证码为 %s，请三分钟内完成注册", code)
	SendMail(email, "验证码", textStr)

	return &pb.GetVerifyRsp{Email: email, Error: Success}, nil
}

func generateCode() string {
	b := make([]byte, 3)
	_, _ = rand.Read(b)
	code := uint32(b[0])<<16 | uint32(b[1])<<8 | uint32(b[2])
	return fmt.Sprintf("%06d", code%1000000)
}

func main() {
	if err := InitConfig(); err != nil {
		log.Fatalf("加载配置失败: %v", err)
	}

	InitEmail()
	InitRedis()

	addr := GetServerAddr()
	lis, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatalf("监听端口失败: %v", err)
	}

	grpcServer := grpc.NewServer()
	pb.RegisterVerifyServiceServer(grpcServer, &server{})

	log.Printf("gRPC 验证码服务已启动，监听 %s", addr)
	if err := grpcServer.Serve(lis); err != nil {
		log.Fatalf("服务启动失败: %v", err)
	}
}
