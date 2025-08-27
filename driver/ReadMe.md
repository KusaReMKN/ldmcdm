# 復号ビット誤り率測定ドライバプログラム

## これはなに

送信機と受信機とを利用して復号ビット誤り率を測定するドライバプログラムです。

## 動作環境

少なくとも、次の環境で動作しています。

```console
$ uname -a
Linux iyokan.local.kusaremkn.com 6.14.0-28-generic #28~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Fri Jul 25 10:47:01 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
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
$ driver receiver transmitter random chip-rate nSamples
```

ここで、各パラメータには以下を指定します:

- *receiver*: 受信機のデバイスファイルを指定します。
    端末のプログラムの実行後、端末の設定は破壊されます。
- *transmitter*: 送信機のデバイスファイルを指定します。
    端末のプログラムの実行後、端末の設定は破壊されます。
- *random*: 乱数生成器のデバイスファイルを指定します。
    無限の長さを読み出せる必要があります（さもなくば、無限ループし得ます）。
- *chip-rate*: 送信機に指示するチップレートを指定します。
- *nSamples*: 送信機に渡すデータの長さをバイト単位で指定します。
