package main

import (
	"log"

	"gopkg.in/gomail.v2"
)

var mailDialer *gomail.Dialer

func InitEmail() {
	mailDialer = gomail.NewDialer(
		"smtp.qq.com",
		465,
		EmailUser,
		EmailPass,
	)
	mailDialer.SSL = true

	log.Println("邮件服务初始化成功")
}

func SendMail(to, subject, body string) {
	go func() {
		m := gomail.NewMessage()
		m.SetHeader("From", EmailUser)
		m.SetHeader("To", to)
		m.SetHeader("Subject", subject)
		m.SetBody("text/html", body)

		if err := mailDialer.DialAndSend(m); err != nil {
			log.Printf("发送邮件失败 [%s]: %v", to, err)
		} else {
			log.Printf("邮件发送成功 [%s]", to)
		}
	}()
}
