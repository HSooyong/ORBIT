# ORBIT

Bearfield 교육용 모듈레이션. 행성 하나가 블랙홀을 돌고, 거리에 따라 피치와 시간이 휜다.

식별자: 회사 Bearfield, 제조사 코드 `Brfd`, 플러그인 코드 `OrGw`, 번들 `com.bearfield.ORBIT`, 제품 이름 ORBIT.

소리 엔진은 그대로다. 창은 1280×660, 비율은 고정이고 가로는 1100–1760이다. 이번 단계는 상단 바, 왼쪽 피치 숫자, 가운데 궤도, 오른쪽 노브 세 개(중력·속도·믹스), 하단 미터 손잡이까지다. 배우기, 미터, 고급, 구역 비교, 토스트는 아직 열리지 않는다. 자리 표시 화면은 `docs/gui_shell.png`이고, 이전 메탈 화면 기록은 `docs/gui_m2.png`이다.

화면 문구는 한국어이고, 파라미터 영문 이름은 작은 보조 글자다. 네트워크, 프롬프트, 자연어 입력은 넣지 않는다.

JUCE 8.0.15는 구성 중에 받는다. C++17, CMake 3.22 이상. 포맷은 macOS에서 AU, VST3, Standalone이고, Linux와 Windows에서는 VST3와 Standalone이다.

## macOS (arm64, macOS 14.7)

Xcode, CMake 3.22 이상, git이 필요하다.

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=14.7
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

맥에서 Audio Unit을 확인할 때는 `auval -v aufx OrGw Brfd`를 쓴다. 이 저장소의 CI는 그 명령을 실행하지 않는다.

## Linux

개발 패키지는 ALSA, JACK, Freetype, Fontconfig, X11이다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Windows

Visual Studio 생성기가 기본이다.

```bash
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## 테스트

`ctest`가 엔진 계약과 라우드니스 계약을 돌린다. 엔진 테스트는 JUCE 없이 빌드된다.

| 이름 | 확인하는 것 |
|---|---|
| `engine` | 무음, 구역별 센트, 100센트 상한, 샘플레이트, 결정론, 피드백 안정, 모노합, 지퍼, CPU, 레벨 매칭, LowSplit(핑크 RMS), 보이스 0 순간 피치 |
| `loudness` | 프리셋 8종 × 소재 3종, dry 대비 ±0.5 LU, 핑크 RMS +0.5 dB 이하 |

한 가지만 돌릴 때는 `ctest --test-dir build -R engine --output-on-failure`를 쓴다. 다중 구성 생성기에서는 `-C Release`를 붙인다.

## 텔레메트리

`Telemetry::cents`는 지금 위치의 흔들림 폭(보이스별 `cWow + cFlutter + cChorus`의 최댓값, 100센트 상한)이다. 큰 숫자는 이 값을 쓴다.

`Telemetry::instantCents`는 보이스 0의 지연 기울기에서 구한 순간 피치(센트)다. `cWow`, `cFlutter`, `cChorus`, `levelMatch`, 입력·출력 파워도 오디오 경로가 다시 읽지 않는 관찰값이다.

화면은 배우기, 미터, 고급, 구역 비교를 열고 세션에 저장한다. 키보드: L 배우기, M 미터, A 고급, Z 구역 비교. Shift로 끌기와 노브를 미세 조정한다. 글꼴은 `fonts/`의 Noto Sans KR, DM Sans, IBM Plex Mono다.

`layout` 테스트는 1100, 1280, 1440, 1600, 1760과 화면 조합에서 글자가 창 안에 있고, 서로 겹치지 않으며, 칸 안에 들어가는지 확인한다.
