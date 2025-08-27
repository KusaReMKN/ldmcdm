# 復号ビット誤り率測定ドライバプログラムのスタブプログラム

## これはなに

一つ上のディレクトリにあるドライバプログラムのスタブプログラムです。
送受信機の対の挙動をエミュレートします。

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
POSIX スレッドを利用しています。

```console
$ make
```

## 使いかた

Unix-like なシステムから次のように実行します。

```console
$ stub
```

疑似端末ファイルが生成され、次の情報が標準エラー出力に書き出されます。

- *receiver*: 受信機のフリをするデバイスファイルです。
- *transmitter*: 送信機のフリをするデバイスファイルです。
