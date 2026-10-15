# VerifyServer (Go gRPC 验证码服务) 镜像

FROM golang:1.26-alpine AS builder
ENV GOPROXY=https://goproxy.cn,direct
WORKDIR /src

COPY go.mod go.sum ./
RUN go mod download

COPY . .
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /verify-server .

FROM alpine:3.20
ARG APK_MIRROR=
RUN if [ -n "${APK_MIRROR}" ]; then       sed -i "s|dl-cdn.alpinelinux.org|${APK_MIRROR}|g" /etc/apk/repositories;     fi     && apk add --no-cache ca-certificates tzdata

WORKDIR /app
COPY --from=builder /verify-server /app/verify-server
COPY docker-entrypoint.sh /app/docker-entrypoint.sh
RUN chmod +x /app/docker-entrypoint.sh

EXPOSE 50051
ENTRYPOINT ["/app/docker-entrypoint.sh"]
