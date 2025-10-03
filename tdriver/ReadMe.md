# 送信機用ドライバプログラム

## これはなに

事前に設定した速度で通信するためのドライバプログラムです。
送信機と適当に通信することで、必要に応じて通信速度を再送します。

## 動作環境

少なくとも、次の環境で動作しています。

```console
$ uname -a
Linux iyokan.local.kusaremkn.com 6.14.0-32-generic #32~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue Sep  2 14:21:04 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/os-release 
PRETTY_NAME="Ubuntu 24.04.3 LTS"
NAME="Ubuntu"
VERSION_ID="24.04"
VERSION="24.04.3 LTS (Noble Numbat)"
VERSION_CODENAME=noble
ID=ubuntu
ID_LIKE=debian
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
UBUNTU_CODENAME=noble
LOGO=ubuntu-logo
```

## コンパイル

Unix-like なシステム上で動作する C のコンパイラが必要です。

```console
$ make
```

## 使いかた

Unix-like なシステムから次のように実行します。

```console
$ tdriver [-b buflen] [-d transmitter] [-s speed] [file ...]
```

ここで、各パラメータには以下を指定します。

- *buflen*: 送信機と通信するためのバッファのサイズを指定します。
    デフォルトでは **1024** が指定されています。
    大き過ぎる値を指定すると送信機のバッファが溢れます。
- *transmitter*: 送信機のデバイスファイルを指定します。
    デフォルトでは **/dev/modem** が指定されています。
    端末のプログラムの実行後、端末の設定は破壊されます。
- *speed*: 送信機に指示するチップレートを指定します。
    デフォルトでは **300** が指定されています。
- *file*: 送信するファイルを指定します。
    **-** が指定されるか、何も指定されなかった場合、標準入力を用います。
