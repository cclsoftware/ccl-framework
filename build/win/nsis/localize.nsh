;-----------------------------------------------------------------------
; Shared strings for all installer scripts
;-----------------------------------------------------------------------

SetFont /LANG=${LANG_ENGLISH} "Segoe UI" 8
LangString Windows10OrLater ${LANG_ENGLISH} "This application requires Windows 10 or 11 22H2 or later."
LangString Windows11OrLater ${LANG_ENGLISH} "This application requires Windows 11 22H2 or later."
LangString Windows64 ${LANG_ENGLISH} "This application requires a 64-bit version of Windows."
LangString WindowsArm64 ${LANG_ENGLISH} "This application requires an Arm64 version of Windows."
LangString FirewallWarning ${LANG_ENGLISH} "The installer is unable to configure the firewall because a third party firewall ($0) is installed on this system. This may limit the network connectivity of this application."

!ifdef LANG_GERMAN
  SetFont /LANG=${LANG_GERMAN} "Segoe UI" 8
  LangString Windows10OrLater ${LANG_GERMAN} "Diese Anwendung erfordert Windows 10 oder 11 22H2 oder neuer."
  LangString Windows11OrLater ${LANG_GERMAN} "Diese Anwendung erfordert Windows 11 22H2 oder neuer."
  LangString Windows64 ${LANG_GERMAN} "Diese Anwendung erfordert eine 64-Bit Version von Windows."
  LangString WindowsArm64 ${LANG_GERMAN} "Diese Anwendung erfordert eine Arm64-Version von Windows."
  LangString FirewallWarning ${LANG_GERMAN} "Die Firewall-Einstellungen konnten nicht angepasst werden, da auf diesem System eine Firewall eines Drittherstellers ($0) installiert ist. Dies kann zu eingeschränkter Netzwerkkonnektivität in dieser Anwendung führen."
!endif

!ifdef LANG_FRENCH
  SetFont /LANG=${LANG_FRENCH} "Segoe UI" 8
  LangString Windows10OrLater ${LANG_FRENCH} "Cette application nécessite Windows 10 ou 11 22H2 ou plus récent."
  LangString Windows11OrLater ${LANG_FRENCH} "Cette application nécessite Windows 11 22H2 ou plus récent."
  LangString Windows64 ${LANG_FRENCH} "Cette application nécessite une version 64 bit de Windows."
  LangString WindowsArm64 ${LANG_FRENCH} "Cette application nécessite une version Arm64 de Windows."
  LangString FirewallWarning ${LANG_FRENCH} "Le programme d'installation ne peut pas configurer le pare-feu car un pare-feu tiers ($0) est installé sur ce système. Cela peut limiter la connectivité réseau de cette application."
!endif

!ifdef LANG_ITALIAN
  SetFont /LANG=${LANG_ITALIAN} "Segoe UI" 8
  LangString Windows10OrLater ${LANG_ITALIAN} "Questa applicazione richiede Windows 10 o 11 22H2 o versioni successive."
  LangString Windows11OrLater ${LANG_ITALIAN} "Questa applicazione richiede Windows 11 22H2 o versioni successive."
  LangString Windows64 ${LANG_ITALIAN} "Questa applicazione richiede una versione a 64 bit di Windows."
  LangString WindowsArm64 ${LANG_ITALIAN} "Questa applicazione richiede una versione Arm64 di Windows."
  LangString FirewallWarning ${LANG_ITALIAN} "Il programma di installazione non è in grado di configurare il firewall perché un firewall di terze parti ($0) è installato su questo sistema. Ciò potrebbe limitare la connettività di rete di questa applicazione."
!endif

!ifdef LANG_SPANISH
  SetFont /LANG=${LANG_SPANISH} "Segoe UI" 8
  LangString Windows10OrLater ${LANG_SPANISH} "Esta aplicación requiere Windows 10 o 11 22H2 o posterior."
  LangString Windows11OrLater ${LANG_SPANISH} "Esta aplicación requiere Windows 11 22H2 o posterior."
  LangString Windows64 ${LANG_SPANISH} "Esta aplicación requiere una versión de Windows a 64-bit."
  LangString WindowsArm64 ${LANG_SPANISH} "Esta aplicación requiere una versión de Windows Arm64."
  LangString FirewallWarning ${LANG_SPANISH} "El instalador no puede configurar el cortafuegos porque hay un cortafuegos de terceros ($0) instalado en este sistema. Esto puede limitar la conectividad de red de esta aplicación."
!endif

!ifdef LANG_PORTUGUESEBR
  SetFont /LANG=${LANG_PORTUGUESEBR} "Segoe UI" 8
  LangString Windows10OrLater ${LANG_PORTUGUESEBR} "Esta aplicação requer Windows 10 ou 11 22H2 ou superior."
  LangString Windows11OrLater ${LANG_PORTUGUESEBR} "Esta aplicação requer Windows 11 22H2 ou superior."
  LangString Windows64 ${LANG_PORTUGUESEBR} "Esta aplicação requer uma versão 64 bits do Windows."
  LangString WindowsArm64 ${LANG_PORTUGUESEBR} "Esta aplicação requer uma versão Arm64 do Windows."
  LangString FirewallWarning ${LANG_PORTUGUESEBR} "O instalador não pode configurar o firewall porque outro firewall de terceiros ($0) está instalado no sistema. Isso pode limitar a conectividade dessa aplicação com a rede."
!endif

!ifdef LANG_SIMPCHINESE
  SetFont /LANG=${LANG_SIMPCHINESE} "Segoe UI" 9
  LangString Windows10OrLater ${LANG_SIMPCHINESE} "此应用程序需要Windows 10或11 22H2或更高版本。"
  LangString Windows11OrLater ${LANG_SIMPCHINESE} "此应用程序需要Windows 11 22H2或更高版本。"
  LangString Windows64 ${LANG_SIMPCHINESE} "该应用需要在64位Windows下运行。"
  LangString WindowsArm64 ${LANG_SIMPCHINESE} "该应用需要在Arm64版本的Windows下运行。"
  LangString FirewallWarning ${LANG_SIMPCHINESE} "安装程序无法配置防火墙，因为在这个系统上安装了第三方防火墙($0)。这可能会限制此应用程序的网络连接。"
!endif

!ifdef LANG_JAPANESE
  SetFont /LANG=${LANG_JAPANESE} "Segoe UI" 9
  LangString Windows10OrLater ${LANG_JAPANESE} "このアプリケーションにはWindows 10または11 22H2以降が必要です。"
  LangString Windows11OrLater ${LANG_JAPANESE} "このアプリケーションにはWindows 11 22H2以降が必要です。"
  LangString Windows64 ${LANG_JAPANESE} "このアプリケーションには64-bitバージョンのWindowsが必要です。"
  LangString WindowsArm64 ${LANG_JAPANESE} "このアプリケーションにはArm64バージョンのWindowsが必要です。"
  LangString FirewallWarning ${LANG_JAPANESE} "このシステムにはサードパーティ製のファイアウォール（$0）がインストールされているため、インストーラーがファイアウォールを設定することはできません。これにより、このアプリケーションのネットワーク接続が制限される場合があります。"
!endif

!ifdef LANG_KOREAN
  SetFont /LANG=${LANG_KOREAN} "Segoe UI" 9
  LangString Windows10OrLater ${LANG_KOREAN} "이 응용 프로그램에는 Windows 10 또는 11 22H2 이상이 필요합니다."
  LangString Windows11OrLater ${LANG_KOREAN} "이 응용 프로그램에는 Windows 11 22H2 이상이 필요합니다."
  LangString Windows64 ${LANG_KOREAN} "이 응용 프로그램에는 64비트 버전의 Windows가 필요합니다."
  LangString WindowsArm64 ${LANG_KOREAN} "이 응용 프로그램에는 Arm64 버전의 Windows가 필요합니다."
  LangString FirewallWarning ${LANG_KOREAN} "이 시스템에 서드파티 방화벽($0)이 설치되어 있어서 인스톨러가 방화벽 구성을 설정할 수 없습니다. 그러므로 이 어플리케이션의 네트워크 접속이 제한될 수 있습니다."
!endif
