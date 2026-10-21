# TeamsKeepAlive

Microsoft Teams 상태가 자리비움(노란색)으로 바뀌지 않고 대화 가능(초록색)으로 유지되도록 하는 Windows 트레이 프로그램입니다. 설치 없이 단일 exe(약 56KB)로 실행되며, 시스템 DLL 외 의존성이 없습니다.

## 사용법

1. release/TeamsKeepAlive.exe 를 원하는 위치에 복사 후 더블클릭
2. 작업표시줄 트레이에 초록색 점 아이콘이 생기면 동작 중
3. 트레이 아이콘 우클릭: 마지막 입력 신호 시각 확인, 일시정지/재개, 종료
4. 트레이 아이콘 더블클릭: 일시정지/재개 전환 (일시정지 시 회색 아이콘)
5. 윈도우 시작 시 자동 실행: Win+R → shell:startup 폴더에 exe 바로가기 추가

중복 실행은 막혀 있습니다(이미 실행 중이면 안내 후 종료).

## 동작 원리

- Teams는 OS의 사용자 입력 유휴 시간(GetLastInputInfo)이 약 5분을 넘으면 자리비움으로 바꿉니다.
- SetThreadExecutionState(절전 방지)만으로는 유휴 타이머가 리셋되지 않으므로 Teams가 자리비움으로 바뀝니다. PowerToys Awake로 초록이 유지되지 않는 이유입니다.
- 그래서 30초마다 유휴 시간을 확인하고, 60초 이상 입력이 없으면 SendInput으로 F15 키 입력을 보냅니다. F15는 일반 키보드에 없는 키이고 앱들이 사용하지 않아 화면/작업에 영향이 없습니다 (Caffeine 방식).
- 실제로 사용 중일 때(유휴 60초 미만)는 아무 입력도 보내지 않습니다.
- 추가로 SetThreadExecutionState로 절전/화면 꺼짐을 막아, 화면 잠금으로 인한 자리비움도 방지합니다.

## 검증

- --selftest 옵션: 명령 프롬프트에서 TeamsKeepAlive.exe --selftest 실행 시 3초 대기 후 F15를 보내고 유휴 타이머가 리셋되는지 PASS/FAIL로 출력합니다.
- Linux + Wine 환경에서 확인한 결과
  - selftest: 유휴 3319ms → F15 전송 후 100ms, PASS
  - 앱 미실행 시 8초간 최대 유휴 8104ms, 앱 실행(테스트용 임계값 3초/주기 1초 빌드) 시 15초간 최대 유휴 3907ms로 유휴 타이머가 계속 리셋됨
- 실제 Windows + Teams 환경에서의 초록 유지 여부는 사용 환경에서 확인이 필요합니다.

## 제약 사항

- Teams를 관리자 권한으로 실행하는 등 더 높은 권한의 창이 포커스되어 있으면 UIPI 때문에 입력 주입이 막힐 수 있습니다. 이 경우 TeamsKeepAlive도 관리자 권한으로 실행하세요.
- 회사 그룹 정책(GPO)으로 강제 화면 잠금 시간이 설정된 경우에도 일반적으로 유휴 타이머 리셋으로 잠금이 지연되지만, 정책에 따라 다를 수 있습니다.
- 캘린더 일정(회의 중), 수동으로 설정한 상태(방해 금지 등)는 이 프로그램과 무관하게 Teams가 표시합니다.
- 회사 보안 정책상 이런 도구 사용이 허용되는지는 사용 전에 확인하세요.

## 빌드

Linux(mingw-w64 크로스 컴파일)

- 필요 패키지: cmake, mingw-w64
- cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
- cmake --build build

Windows(Visual Studio 2022)

- cmake -S . -B build -G "Visual Studio 17 2022"
- cmake --build build --config Release
- 정적 CRT로 링크되어 VC++ 재배포 패키지 없이 실행됩니다.

## 참고 자료

- Caffeine (Zhorn Software): F15 키 입력으로 유휴 상태를 방지하는 원조 방식
- Trayffeine: Teams 상태 유지를 위한 F15 기반 keep-awake 모드 (https://github.com/rodrigoantonioli/trayffeine)
- keep-active (Rust): 커서 이동 없이 F15 키를 주기적으로 입력 (https://docs.rs/crate/keep-active/0.2.0)
- vigil: OS 유휴 타이머를 리셋하는 방식으로 Teams 대화 가능 상태 유지 (https://github.com/simtabi/vigil)
- Microsoft Docs: SendInput, GetLastInputInfo, SetThreadExecutionState
