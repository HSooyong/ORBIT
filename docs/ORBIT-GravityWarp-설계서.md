# ORBIT — Gravity Warp (중력 왜곡) 설계서 (v0.2, 구현 반영)

작성: 2026-10-25 (KST) · v0.2 개정: 2026-10-26 (KST), M1/M2 구현(`/workspace/ORBIT-GravityWarp`)과 맞춤 · 브랜드: PLUGIN LAB(가칭, 상호 미정) · 한 줄 슬로건: "GUI가 곧 악기"

> 기준 버전: Mac에 있는 **ORBIT v0.3**의 레이아웃(920×640, 좌/우 메탈 노브 패널 + 가운데 오빗 플레이필드, 커스텀 메탈 로터리 LookAndFeel, Sync 7종, 프리셋 5개, 오빗을 드래그해서 Phase/Depth/Shape 연주).
> 박스에는 예전 사본(`/workspace/ORBIT-plugin`)만 있어서 v0.3 코드는 직접 보지 못함. 아래에서 v0.3 내부를 언급하는 부분은 위 설명을 근거로 한 추정이며, Mac 코드를 열 때 다시 확인해야 함.

## v0.2 개정 요약: 구현이 v0.1과 달라진 점 (전부)
아래 내용이 본문보다 우선합니다. 본문은 v0.1 설계 의도를 기록으로 남겨 둡니다.

| # | 항목 | v0.1 | v0.2 (구현, 승인됨) |
|---|---|---|---|
| 1 | 모듈레이션 매핑 | 깊이를 ms로 지정 | **센트 기준**. 진폭 A = cents / (1731.234·2π·f). 그래서 샘플레이트나 레이트가 바뀌어도 피치 편차가 같음 |
| 2 | 매핑 식 | 안 | fWow = 0.25+1.2g Hz · cWow = Wow·120·g^1.5 · cFlutter = Flutter·35·g³, 주파수는 6+8g Hz · cChorus = 20·SpaceChorus·(0.3+0.7(1−g)) · 기본 딜레이 7+8g ms, 보이스마다 +1.1 ms |
| 3 | 중력 | 안 | g = Gravity / (1+(d/rs)^2.5). rs = Horizon (0.05–0.5) |
| 4 | 피치 한도 | ±100 cent | 내부 목표는 88 cent(오버슈트 여유분). 여기에 **딜레이 변화율을 하드 리밋**(|dD/dt| ≤ 0.0594 = 1−2^(−100/1200))해서, 파라미터가 순간적으로 점프해도 100 cent를 넘지 않음 |
| 5 | Chaos | 화이트 노이즈 + 1-pole | 랜덤 점들 사이를 코사인으로 보간하는 노이즈(주기 0.55 s + 0.07 s·i). 화이트 노이즈는 미분값이 커서 424 cent까지 튀었음 |
| 6 | 파라미터 | 안 | **25개**, X/Y 파라미터 없음. 행성을 드래그하면 Phase와 Orbit Size가 바뀜. 25개: rate, sync, phase, shape, stages, ecc, radius, bh_x, bh_y, gravity, horizon, floor(Space Chorus), warp(Wow), flutter, chaos, voices, width, feedback, tone, drive(Tape), kepler, mix, output, hold, bloom |
| 7 | Hold | 안 | 자동화 가능한 bool. 행성을 드래그하는 동안에는 임시로 켜졌다가, 놓으면 원래 상태로 돌아감 |
| 8 | Horizon Bloom | 안 | **존 확대(2026-10-26): d < 0.6·rs에서 시작해 0.3·rs에서 최대**(이전 0.3→0.15·rs). wet −1.5 dB, 피드백 +10%(총합 상한 70%). 프리셋: Event Horizon 60%, **Tidal Pad 20%**, 나머지 0%. 구·신 존 A/B(같은 설정): 블룸 0% 프리셋 5개는 비트 단위 동일, Event Horizon −0.05 dB (차이 신호 −32.8 dB), Tidal Pad −0.02 dB (−40.7 dB, 블룸 최대치 0.07→0.20). `tests/bloom_zone_ab_results.txt` |
| 9 | Sync | 7종 | 11종: Free, 4/1, 2/1, 1/1, 1/2D, 1/2, 1/4D, 1/4, 1/8, 1/8T, 1/16. PPQ에 잠그고 10 ms 슬루로 보정 |
| 10 | 제거 | v0.3 트레몰로/팬, HQ 모드, 테스트 톤 | 셋 다 제거. 220 Hz 무음 입력 톤은 없음(무음 입력이면 무음 출력) |
| 11 | 호환성 | 안 | v0.3 상태와 호환하지 않음. 새 플러그인 코드 OrGw, 제조사 PLab |
| 12 | 블랙홀 | 1개 이상 | 1개 |
| 13 | Mix 기본값 | 안 | 35% |
| 14 | Voices 변경 | 즉시 | 보이스 위상 분산과 팬이 30 ms 동안 글라이드(지퍼 노이즈 방지). 딜레이 오프셋은 보이스 수와 상관없이 1.1 ms·i |
| 15 | Kepler | 안 | 균등한 시간 → 각도 테이블(256개, dt/dθ ∝ ρ²) 사용, 토글 가능 |
| 16 | GUI | 6장 | 920×640. 좌: ORBIT/MOTION(Rate, Phase, Shape, Ecc, Orbit Size, Gravity, Sync, Stages). 우: SOUND CHARACTER(Wow, Flutter, Chaos, Width, Tape, Tone, Mix, Output). 접히는 ADVANCED(Horizon, H.Bloom, Space Chorus, Feedback, Hole X/Y, Voices, Kepler). 헤더: 프리셋 ◀▶, HOLD |
| 17 | 필드 제스처 | 6.3 | 행성 드래그 → Phase/Orbit Size · 블랙홀 드래그 → BH X/Y · 블랙홀 위에서 휠 → Gravity · 빈 곳 더블클릭 → 블랙홀 이동. 모든 제스처가 begin/endChangeGesture를 보냄 |
| 19 | Tidal Pad Width | 100% | **75%**: mono-sum 최악 대역 −2.82 → −1.88 dB |
| 20 | 값 표시 단위 | 안 | % · Hz · dB · ° (Phase) · Wow/Flutter/Space Chorus는 "% (최대 ct)" · Tone은 Dark/Flat/Bright · Ecc는 "e 0.30" · Horizon은 "r 0.20" · Sync가 켜지면 Rate 칸에 "Sync 1/4" 표시. ms 단위 파라미터는 없음 |
| 21 | 출력 레벨 (LevelMatch) | 없음. Mix는 등전력(cos/sin) 크로스페이드 | 등전력 Mix는 dry와 상관된 wet(짧은 지연의 저역, √n 보이스 합, 피드백 누적, Bloom 확산)에서 레벨이 올라 기본값에서도 +0.3~1.6 LU, 핑크 노이즈 RMS +1.3~3 dB가 됐다. Mix 뒤에 자동 레벨 매칭을 넣었다. 입력과 출력의 K-가중(BS.1770) 파워를 400 ms로 추적하고, 게인 sqrt(Pin/Pout)를 1.0 s로 글라이드(v0.2 초안 1.5 s에서 단축)한다(범위 −6~+1.5 dB). 입력이 −60 dB 아래면 학습을 멈춰 꼬리는 마지막 게인을 유지한다. Mix 0%면 게인은 1이다. Output 노브는 그 뒤에 곱한다. 회귀 테스트: 기본값과 7개 프리셋 × 핑크/드럼/패드, 통합 LUFS가 dry 대비 ±0.5 LU 이내 |
| 22 | 저역 분리 (LowSplit) | wet 전 대역 | 180 Hz 아래는 dry만 통과한다. 2차 Butterworth(TPT SVF) LP로 dry를 나누고(LP + (x−LP)·dry게인, 합이 평탄), wet에서는 같은 LP 성분을 뺀다(12 dB/oct 로우컷). 이유: 짧은 지연 탭이 저역에서 dry와 같은 위상으로 쌓여 비가중 RMS가 최대 +2.7 dB 올라갔다. 검토한 안은 셋이다. (a) wet만 로우컷: Mix 35%에서 저역이 dry게인 0.854만큼 줄어 저역이 얇아진다. (b) 저역을 모노·무변조로: 상관 누적이 그대로라 RMS가 줄지 않는다. (c) 크로스오버로 저역은 dry만: 저역이 원음 그대로이고 킥·베이스의 위상과 모노 호환도 좋아진다. (c)를 골랐다. 120/160/180/200 Hz를 비교했다(핑크 RMS 최악 +0.67/+0.47/+0.42/+0.39 dB). 180 Hz는 여유를 두면서도 저역 컷이 가장 낮은 값이다. 회귀 테스트: 핑크 RMS가 dry 대비 +0.5 dB 이하 |
| 18 | 비주얼 | 6.2 | 오디오 텔레메트리(px, py, bh, d, g, cents, wowHz, bloom, theta)로 구동: 별빛 렌즈 효과, 얇고 기울어진 따뜻한 강착원반과 휘어진 윗호, 포톤 링, 빨려 들어가는 먼지(g, bloom에 비례), 중력 쪽으로 늘어나는 행성 꼬리와 조석 변형, 보이스 수 = 위성 수. 네온 없음 |

### 테스트 기준 (v0.2에서 확정, 결과는 `tests/last_results.txt`)
무음 입력이면 무음 출력 · 구역별 센트(far ≤4, mid 5–30, near 50–100) · 상한 ≤100 cent · SR 44.1–192k 편차 ≤0.5 cent · 결정론(블록 크기와 무관하게 비트 단위로 같음) · 피드백 70%에서 안정 · **mono-sum**: 1/3옥타브 대역 중 −2 dB 아래로 떨어지는 대역 없음 (v0.2 초안은 −3 dB) · **zipper**: 파라미터 8개를 21 ms마다 점프시켜도 2차 차분 피크가 정상 상태의 1.5배 이하 · **CPU**: 최악 조건(4보이스)에서 코어 1개 기준 < 2%

---
## 0. 기존 자료 요약
- **박스 사본 ORBIT**: 파라미터는 Rate(0.05–20 Hz) / Depth / Phase(cyc) / Shape / Stages(Smooth,2…16) / Sync / Mix. DSP는 트레몰로 + L/R 역위상 오빗 팬.
  - `evaluateOrbitLfo(phase, eccentricity, shape, stages, phaseOffset)`: 위상 오프셋 → Stages 양자화 → 타원 반경 `r = (1-e)/(1-e·cosθ)` → sine↔triangle 모프 → `wave·(0.55+0.45r)`.
  - `resolveRateHz(freeRate, syncIdx)`: Free/박자 → Hz.
  - 입력이 무음이면 220 Hz 테스트 톤을 **자동으로 섞는 디버그 코드**가 들어 있음 → 제품판에서는 반드시 빼야 함.
  - 빌드와 pluginval(strictness 5) 통과 (2026-10-25 실사용 정리 계획 기준).
- **OrbitVisualizer**: 30–60 Hz 타이머, mouseDown/Drag/Up으로 파라미터를 연주. 이 구조를 그대로 확장함.
- **TILT 설계서**의 형식(신호 흐름 → 모델 → 매핑 → DSP → 파라미터 → GUI → 프리셋 → 구현/테스트 → 로드맵 → 미결)을 따름.

---
## 1. 컨셉과 사운드 목표
**이야기:** 오빗 화면 위에서 행성(= 소리)이 궤도를 돈다. 화면 안에는 블랙홀이 하나 있다. 행성이 블랙홀에 가까워질수록 시간과 피치가 휘어서 테이프 wow/flutter처럼 늘어지고 일렁인다. 멀어지면 다시 맑아진다.

| 구역 | 블랙홀까지 거리 d (정규화) | 들리는 소리 | 용도 |
|---|---|---|---|
| 원거리 (Clear Space) | d > 0.6 | 아주 옅은 2–3보이스 코러스. 피치 흔들림 ±3 cent 안팎 | 데일리 인서트, 보컬/기타/패드 폭 넓히기 |
| 중거리 (Drift) | 0.25–0.6 | 느린 wow(0.3–1.5 Hz), 넓은 코러스, ±10–20 cent | 로파이, 빈티지 테이프 느낌 |
| 근접 (Event Horizon) | d < 0.25 | 깊은 wow + 랜덤 flutter(6–14 Hz), ±50 cent 이상, 시간이 늘어지는 느낌 | 이펙트 연출, 트랜지션, 사운드디자인 |

**원칙**
1. 기본값은 반드시 '쓸 만한' 소리: 기본 프리셋에서 행성은 원거리 궤도를 돌고, 은은한 코러스로 들린다.
2. 거리 하나가 **깊이·속도·랜덤성**을 함께 움직인다. 매크로 하나로 여러 값이 같이 움직이니 쓰는 사람은 거리만 생각하면 된다.
3. 궤도 자체가 LFO다. 타원 궤도라면 원지점 → 근지점으로 갈 때 자연스럽게 휘어짐이 커졌다가 풀린다(기존 eccentricity 은유를 그대로 살림).
4. 볼륨 트레몰로는 기본으로 끄고, 옵션(Legacy Tremolo)으로만 남긴다 (→ 미결 Q3).

---
## 2. 신호 흐름
```
In (mono/stereo) ─┬───────────────────────────────────────────────┐
                  │                                               │ dry (지연 보상 필요 없음: 0 latency)
                  ▼                                               │
          [Input Drive (opt, tanh)] → [Tone pre (opt)]             │
                  ▼                                               │
      ┌── Modulated Delay Lines (N voices, 2–4) ◀── Mod Matrix ◀── Gravity Mapper ◀── Orbit Engine (LFO 재사용)
      │      ▲  frac-delay (cubic Hermite/Lagrange3)                 ▲                   ▲
      │      └── Feedback (LP/HP 필터, tanh 리미트) ◀──┐            d(t), 각속도          Rate/Sync/Shape/Stages/Phase
      ▼                                               │
  Voice Pan/Width → Sum ─────────────────────────────┘
      ▼
  [Tone post: Tilt LP/HP] → [Tape Sat (opt)] → Wet ──▶ Mix (equal-power) ──▶ Output Gain ──▶ Out
```

---
## 3. 중력 모델 (Orbit Engine → Gravity Mapper)
### 3.1 좌표
- 플레이필드 좌표는 정규화한 [-1,1]². 궤도 중심은 (0,0)이고, 블랙홀 위치는 `bh_x, bh_y` 파라미터.
- 행성 위치: `P(θ) = R · (a·cosθ', b·sinθ')`, θ' = θ + Phase·2π. `a=1`, `b = 1 − Ecc` (타원).
- θ는 기존 `phaseAccumulator`. Stages가 Smooth가 아니면 θ를 N개 노드로 양자화하고, 노드 사이를 짧게(5 ms) 슬루해서 "궤도 정거장" 느낌을 준다.

### 3.2 거리 → 중력 강도
- `d = |P − BH| / d_max` (d_max = 2.0, 0..1로 클램프)
- 중력 강도 `g = Gravity · (1 / (1 + (d/r_s)^k))` — 로지스틱 형태라 무한대로 발산하지 않음.
  - `r_s` = Horizon 파라미터(사건의 지평선 크기 0.05–0.5), `k = 2.5` 고정.
  - 데일리 바닥값: `g_eff = max(g, Floor)`. Floor는 원거리에서 남는 코러스 양(기본 0.08).
- 블록마다가 아니라 **샘플 단위로 계산**하고, 비용을 줄이려면 16샘플마다 계산한 뒤 선형 보간.

### 3.3 g → 모듈레이션 매핑 (Mod Matrix)
| 대상 | 식 (g∈[0,1]) | 원거리 g≈0.08 | 근접 g≈1 |
|---|---|---|---|
| 코러스 딜레이 기준 시간 | 7 ms + 8 ms·g | ~7.6 ms | 15 ms |
| Wow 깊이 (ms 편차) | Warp · (0.05 + 3.0·g²) ms | ~0.05 ms (≈±2–3 cent) | ~3 ms |
| Wow 속도 | 0.25 Hz + 1.2·g Hz (또는 궤도 각속도에 고정, §3.4) | 0.35 Hz | 1.45 Hz |
| Flutter 깊이 | Flutter · 0.4 ms · g³ | ≈0 | 0.4 ms |
| Flutter 속도 | 6 + 8·g Hz, 랜덤 ±20% | — | 14 Hz |
| 랜덤성 (smoothed noise 비율) | Chaos · g^1.5 | ≈0 | Chaos |
| 보이스 간 디튠 확산 | Spread · (0.3 + 0.7(1−g)) | 넓게 | 좁게 (모노로 빨려 들어감) |
| 고역 감쇠 (선택) | LP cutoff 18 kHz → 6 kHz · g | 열림 | 어두움 |
- 피치 편차 ≈ `dDelay/dt`이므로 깊이(ms)와 속도(Hz)를 곱한 값을 **최대 ±100 cent로 리미트**한다(근접 + Fast Rate일 때 귀가 아프지 않게).
- 블랙홀에 "흡수"되는 연출: d < r_s·0.3이면 wet 레벨을 −1.5 dB 낮추고 피드백을 +10% 올린다(Horizon Bloom). 이 연출을 켤지 말지는 → 미결 Q5.

### 3.4 속도의 근거 (케플러 느낌)
- 타원 궤도에서는 근지점에서 빨라지는 게 자연스럽다. Kepler 옵션을 켜면 θ 진행 속도에 `(1/r)²` 가중을 준다(평균 주기는 Rate/Sync를 유지하도록 정규화). 기본은 ON. 박자 싱크일 때도 한 바퀴 주기는 정확히 맞춤.

---
## 4. DSP 상세
### 4.1 딜레이 라인
- 채널당 원형 버퍼 하나를 보이스가 공유하는 멀티탭 구조. 최대 지연 = 15 ms + 3 ms + 0.4 ms + 여유 → **50 ms** 버퍼(SR에 비례해 prepareToPlay에서 할당).
- 분수 지연 보간: 기본 **4점 Hermite(cubic)**. HQ 옵션에서는 Lagrange 5차, 또는 2× 오버샘플링(→ 미결 Q7).
- 딜레이 시간 변화는 샘플 단위로 연속이라 지퍼가 생기지 않음. 단, 파라미터 타깃은 아래 스무딩을 거침.

### 4.2 보이스와 스테레오
- Voices: 1 / 2 / 3 / 4 (기본 2). 보이스 i의 LFO 위상은 `i/N` 만큼 어긋나게 하고, 랜덤 시드를 따로 둔다.
- 스테레오: 보이스를 L↔R로 `Width`만큼 고르게 배치. 기존 "오빗 팬"은 보이스 팬이 궤도 x좌표를 아주 조금(±15%) 따라가게 해서 계승한다.
- 모노 입력 → 스테레오 출력 지원 (mono→stereo 레이아웃). 모노 출력이면 보이스를 합산하되, Width는 디튠에만 적용.
- 모노 호환: 보이스 간 위상이 서로 상쇄되지 않도록 기본 Width는 70%, 합산 테스트 필수(§9).

### 4.3 Wow/Flutter 소스
- **Wow** = 궤도 LFO 자체(`evaluateOrbitLfo` 출력) + Chaos 비율만큼 저역 노이즈(1-pole LP 0.5 Hz로 거른 white noise).
- **Flutter** = 내부 sine 2개(비정수 비율 1 : 1.618) + 밴드 제한 노이즈. 템포 싱크를 하지 않음(테이프 기계의 물리적 성격).
- 모든 랜덤은 시드를 고정한 xorshift를 써서 **결정론적**으로 만든다 → 오프라인 렌더 = 실시간 결과. TILT의 결정론 테스트 방식을 그대로 이어받음.

### 4.4 피드백
- 0–70% (기본 0). 피드백 경로에 HP 120 Hz + LP(Tone 연동) + `tanh` 소프트 리미트. 근접할 때 플랜저/공명 쪽으로 넘어가는 소리를 내는 용도.
- 극성 반전 옵션은 넣지 않음(단순하게).

### 4.5 Tone / Saturation (선택)
- **Tone**: −1..+1 틸트(−는 어둡게: LP 4k까지, +는 밝게: 저역 셸프 −). wet에만 적용.
- **Tape (Drive)**: 0–100%. 비대칭 tanh + 약한 고역 감쇠. 0%면 바이패스해서 CPU 0. 넣을지 말지는 → 미결 Q4.

### 4.6 스무딩
| 대상 | 방식 | 시간 |
|---|---|---|
| Gravity, Warp, Flutter, Chaos, Floor, Horizon | SmoothedValue linear | 30 ms |
| 블랙홀 위치 (bh_x, bh_y) | 1-pole | 40 ms (드래그를 해도 피치가 튀지 않게) |
| Phase | 래핑을 고려한 각도 슬루 | 20 ms |
| Mix | equal-power, 20 ms |
| Output | multiplicative 20 ms |
| Voices 변경 | 새 보이스는 10 ms 페이드인, 빠지는 보이스는 페이드아웃 | — |

### 4.7 레이턴시
- **0 샘플 보고**. 코러스 특성상 wet은 원래 늦게 들리는 소리이고 dry는 그대로 지나감. HQ 오버샘플링을 켤 때만 필터 지연을 `setLatencySamples`로 보고.

### 4.8 템포 싱크
- 기존 Sync 목록을 유지: Free, 1/1, 1/2, 1/4, 1/8, 1/8T, 1/16 (확장 후보 1/2D, 2/1, 4/1 → 미결 Q6. Wow용으로는 긴 주기가 쓸모 있음).
- `getPlayHead()->getPosition()`의 PPQ로 위상을 맞춘다: 재생 중에는 `θ = frac(ppq / beatsPerCycle) + Phase`, 정지 상태에서는 프리런. 루프 점프에 위상이 튀면 10 ms 슬루.
- 싱크 대상은 궤도(= Wow 주기)뿐. Flutter는 프리런.

### 4.9 CPU 예산 (48 kHz, 512 블록, 스테레오)
| 항목 | 예산 |
|---|---|
| 궤도/중력 계산 (16샘플마다) | < 0.2% |
| 보이스 4 × 2ch Hermite 탭 | ~1.0% |
| 피드백 필터 + Tone + Sat | ~0.5% |
| 합계 목표 | 한 인스턴스당 **코어 1개의 2% 미만** (M1 기준). HQ 모드는 5% 미만 |
- 할당/락 없음. 버퍼는 prepareToPlay에서만 할당. GUI와는 atomic으로 값 교환(행성 x,y, g, 보이스별 순간 피치 편차).

### 4.10 샘플레이트 독립
- 모든 시간 상수는 ms/Hz로 정의하고 prepareToPlay에서 샘플로 환산. 버퍼 길이 = ceil(0.05·SR)+8.
- 노이즈 필터 계수도 SR로 다시 계산. 테스트: 44.1/48/88.2/96/192 kHz에서 cent 편차가 ±0.5 cent 이내로 같아야 함.

---
## 5. 파라미터 (APVTS)
버전 ID를 2로 올림(`ParameterID{ id, 2 }`). 기존 ID는 가능하면 그대로 두어서 v0.3 세션/프리셋과 최대한 호환되게 함(→ 미결 Q8).

| ID | 이름 | 범위 | 기본 | 단위 | 하는 일 | 오토메이션 |
|---|---|---|---|---|---|---|
| `rate` | Rate | 0.02–10 (skew 0.4) | 0.35 | Hz | Sync=Free일 때 궤도 1바퀴 속도(= wow 기본 주기) | O |
| `sync` | Sync | Free,1/1,1/2,1/4,1/8,1/8T,1/16 (+후보) | Free | — | 궤도 주기를 템포에 고정 | O (choice) |
| `phase` | Phase | 0–1 | 0 | cyc | 궤도 시작 각도 / 싱크 오프셋 | O |
| `shape` | Shape | 0–1 | 0.2 | — | sine↔triangle 모프. 높을수록 근지점 통과가 급해짐 | O |
| `stages` | Stages | Smooth,2,3,4,5,6,8,12,16 | Smooth | — | 궤도 정거장 양자화(계단식 피치 이동) | O (choice) |
| `ecc` | Eccentricity | 0–0.9 | 0.3 | — | 궤도의 타원 정도. 블랙홀에 가까워졌다 멀어지는 폭 | O |
| `radius` | Orbit Size | 0.2–1 | 0.7 | — | 궤도 크기 | O |
| `bh_x` | Black Hole X | −1–1 | 0.85 | — | 블랙홀 위치 (기본은 궤도 밖 → 데일리 코러스) | O |
| `bh_y` | Black Hole Y | −1–1 | 0 | — | 블랙홀 위치 | O |
| `gravity` | Gravity | 0–100 | 50 | % | 블랙홀 질량 = 거리 대비 휘어짐의 세기 (매크로) | O |
| `horizon` | Horizon | 0.05–0.5 | 0.2 | — | 영향권 반경 r_s | O |
| `floor` | Space Chorus | 0–100 | 15 | % | 멀리 있을 때 남는 코러스 양 | O |
| `warp` | Wow | 0–100 | 60 | % | 최대 wow 깊이 스케일 | O |
| `flutter` | Flutter | 0–100 | 30 | % | 근접 시 flutter 양 | O |
| `chaos` | Chaos | 0–100 | 25 | % | 랜덤성(테이프 불안정성) | O |
| `voices` | Voices | 1,2,3,4 | 2 | — | 모듈레이션 딜레이 보이스 수 | O (choice, 페이드) |
| `width` | Width | 0–100 | 70 | % | 보이스 스테레오 확산 | O |
| `feedback` | Feedback | 0–70 | 0 | % | 딜레이 피드백 | O |
| `tone` | Tone | −1–+1 | 0 | — | wet 틸트 EQ | O |
| `drive` | Tape | 0–100 | 0 | % | 테이프 포화 (옵션, 미결 Q4) | O |
| `kepler` | Kepler | Off/On | On | — | 근지점 가속 | O |
| `mix` | Mix | 0–100 | 50 | % | dry/wet (equal-power) | O |
| `output` | Output | −12–+12 | 0 | dB | 출력 게인 | O |
| `hq` | Quality | Eco/HQ | Eco | — | 보간 품질 / 오버샘플링 | X (비오토메이션 권장) |
| `legacy_trem` | Tremolo | 0–100 | 0 | % | 구 ORBIT 트레몰로 (미결 Q3) | O |

- 기존 `depth`는 **Gravity**로 의미를 옮긴다(이름은 다르지만 '얼마나 세게'라는 역할이 같음). 호환 매핑은 미결 Q8.
- 노브는 23개 이상이라 많다 → 패널에는 **주 노브 12개**만 두고, 나머지는 "ADVANCED" 접이식 줄로 보낸다(§6.1).

---
## 6. GUI (v0.3 레이아웃 기준, 920×640)
### 6.1 배치
```
┌──────────────────────────── 920 ────────────────────────────┐
│ ORBIT · GRAVITY WARP        [Preset ◀ ▶]   [Sync] [HQ]  ⏻  │ 상단 바 48px
├────────┬──────────────────────────────────────────┬─────────┤
│ LEFT   │                                          │ RIGHT   │
│ (ORBIT)│         ORBIT PLAYFIELD (≈560×520)       │ (SPACE) │
│ Rate   │    · 얇은 궤도 링 (cyan/white 1px)        │ Wow     │
│ Shape  │    · 행성 (흰 점 + 옅은 꼬리)             │ Flutter │
│ Ecc    │    · 블랙홀 (void 원반 + 렌즈 링)         │ Chaos   │
│ Stages │    · 중력 등고선 (점선 동심원 2–3개)       │ Width   │
│ Phase  │                                          │ Mix     │
│ Gravity│                                          │ Output  │
├────────┴──────────────────────────────────────────┴─────────┤
│ ADVANCED ▸ Horizon · Space Chorus · Voices · Feedback · Tone · Tape · Kepler │ 접이식
└──────────────────────────────────────────────────────────────┘
```
- 좌 패널 = "궤도"(시간/움직임), 우 패널 = "공간"(소리 성격). 메탈 로터리 LookAndFeel과 베젤은 v0.3 것을 그대로 씀.

### 6.2 비주얼 언어
- 배경 void black(#05060A), 링 #9FE8FF 25–60% 알파, 행성 #F2F7FF. 네온·글로우 블룸은 쓰지 않음. 인터스텔라처럼 조용한 톤.
- **블랙홀**: 검은 원반 + 아주 얇은 광자 링(1px) + 주변 별 배경을 약하게 굴절시키는 렌즈 효과(사전 계산한 변위맵, 30fps).
- **중력 표시**: 행성의 꼬리 길이와 흔들림이 현재 g에 비례(가까울수록 꼬리가 늘어지고 미세하게 떨림 = flutter를 시각화). 궤도 링도 블랙홀 근처에서 살짝 휘게 그림.
- **보이스**: 행성 옆에 작은 위성 점 N개. 스테레오 위치를 반영.
- **수치 리드아웃**(하단 구석, 작은 모노 폰트): `d 0.72 · ±3.1 ct · 0.35 Hz`.

### 6.3 제스처 → 파라미터
| 제스처 | 대상 | 동작 |
|---|---|---|
| 블랙홀 드래그 | `bh_x`,`bh_y` | 블랙홀을 옮겨 영향권 배치. 가장 중요한 "악기" 제스처 |
| 블랙홀 위에서 휠 / Alt+드래그 | `gravity` | 질량 조절 (원반 크기가 바뀜) |
| Shift+블랙홀 드래그(반경 방향) | `horizon` | 영향권 링 크기 |
| 행성 드래그 (v0.3 계승) | `phase`(각도), `radius`/`ecc`(반경), `shape`(드래그 속도 또는 수직 성분) | v0.3의 Phase/Depth/Shape 연주를 유지하되, Depth 자리는 Orbit Size로 바꿈. 블랙홀 쪽으로 끌어당기면 즉시 휘어짐 |
| 행성 잡고 멈춤 (Hold) | 내부 상태 | 드래그하는 동안 궤도 진행을 멈추고 손 위치가 행성 위치가 됨 → "손으로 블랙홀에 밀어 넣기" 연주. 놓으면 그 각도에서 궤도가 다시 진행 (미결 Q2) |
| 링 더블클릭 | — | 블랙홀 기본 위치로 리셋 |
| 빈 공간 더블클릭 | — | 블랙홀을 그 위치로 순간 이동 (40 ms 스무딩) |

**오토메이션 제스처 규칙**
- mouseDown에서 영향받는 모든 파라미터에 `beginChangeGesture()`를 호출하고, mouseDrag에서 `setValueNotifyingHost()`, mouseUp에서 `endChangeGesture()`. 2축 드래그는 두 파라미터를 같은 제스처로 묶어 시작하고 끝냄.
- Hold 연주는 내부 "manual position" 상태라 오토메이션하지 않음. 기록이 필요하면 phase/radius로 기록되게 할지는 미결 Q2.
- DAW 오토메이션을 재생할 때는 GUI가 파라미터를 따라가서 블랙홀/행성이 스스로 움직임(= 오토메이션이 그대로 퍼포먼스 영상이 됨).

---
## 7. 재사용하는 LFO 코드
| 기존 | 재사용 방식 |
|---|---|
| `evaluateOrbitLfo()` | 그대로 남겨서 **Wow 파형**으로 씀. 다만 1D 출력 대신 2D 위치가 필요하므로 `OrbitEngine::position(θ) → {x,y}`를 새로 만들고, 기존 함수는 `wave` 성분(shape 모프, stages 양자화)을 계산하는 헬퍼로 쪼갬. 타원식 `r(θ)`도 거리 계산에 그대로 씀 |
| `resolveRateHz()` | 그대로 씀 + PPQ 위상 동기를 추가 |
| `phaseAccumulator` 및 1e6 래핑 | 그대로 씀. Kepler 가중만 추가 |
| Stages 테이블 `{0,2,3,4,5,6,8,12,16}` | 그대로 씀 + 노드 사이 슬루 추가 |
| `smoothL/R` (SmoothedValue) 패턴 | 모든 매크로에 확장 |
| 트레몰로/팬 게인 코드 | `legacy_trem`으로만 남기거나 삭제 (Q3) |
| 무음일 때 테스트 톤 주입 | **삭제** (Standalone 데모 모드에서만 메뉴로 켜는 방식은 가능) |
| OrbitVisualizer 타이머/마우스 구조 | 그대로 쓰고, 블랙홀 히트테스트·렌즈 렌더를 추가 |
- 새 클래스: `OrbitEngine`(궤도/위치), `GravityMapper`(d→g→mod 타깃), `WarpDelay`(멀티보이스 모듈레이션 딜레이), `TapeColor`(Tone/Sat). Processor는 이들을 엮기만 함. 엔진은 JUCE 의존 없이 헤더로 만들어 결정론 테스트를 가능하게 함(TILT 규약).

---
## 8. 프리셋 후보 (7개)
| 이름 | 의도 | 핵심 설정 |
|---|---|---|
| **Low Orbit** (기본) | 데일리 은은한 코러스. 꽂아만 둬도 넓어짐 | BH 궤도 밖(0.9,0), Floor 15, Mix 40, Voices 2 |
| **Vocal Halo** | 보컬 더블링/폭 | Voices 3, Width 85, Wow 25, Flutter 0, Mix 30 |
| **Worn Cassette** | 상시 테이프 wow/flutter, 로파이 | BH가 궤도 안쪽 가까이, Chaos 50, Flutter 45, Tone −0.3, Tape 30 |
| **Periapsis** | 한 바퀴에 한 번 블랙홀을 스침 → 주기적으로 휘었다 풀림 | Ecc 0.8, Sync 1/1, Kepler On |
| **Event Horizon** | 극단적인 피치 붕괴, 트랜지션/FX | BH 궤도 위, Gravity 100, Horizon 0.35, Feedback 35 |
| **Stepping Stones** | Stages 8로 계단식 피치 이동, 리듬감 있는 wow | Stages 8, Sync 1/8, Shape 0.6 |
| **Tidal Pad** | 아주 느리고 깊은 wow, 패드/앰비언트 | Rate 0.05, Wow 90, Voices 4, Width 100, Mix 60 |
- 기존 v0.3 프리셋 5개는 이름이 중력 컨셉과 맞으면 이어받고, 맞지 않으면 교체(Mac에서 확인 필요).

---
## 9. 구현과 테스트 계획
- 구조는 기존 ORBIT 규약(JUCE 8, C++17, CMake)을 유지. `Source/engine/` 폴더에 엔진 헤더를 둠.
- 테스트:
  1. **결정론**: 블록 크기 1–4096, SR 44.1–192 kHz에서 같은 출력(오프라인 렌더 비교).
  2. **피치 편차 측정**: 1 kHz 사인을 입력하고 순간 주파수를 추적해서 거리별 cent 곡선이 §3.3 표와 맞는지 확인.
  3. **모노 합산**: L+R 합산 시 −3 dB 넘는 콤필터 노치가 없는지.
  4. **지퍼 테스트**: 블랙홀을 빠르게 왕복 드래그하는 매크로 → 클릭 없음.
  5. pluginval strictness 5 (가능하면 GUI 테스트 포함), Logic/Ableton/Reaper에서 오토메이션 기록·재생.
  6. CPU 측정 (§4.9 기준).

---
## 10. 마일스톤 (프로토타입 우선)
| # | 내용 | 완료 기준 |
|---|---|---|
| M0 | Mac v0.3 코드를 박스로 가져와서 비교, 설계서 v0.2 확정 (미결 질문 답 반영) | 질문 답변 완료 |
| M1 | **사운드 프로토타입**: 노브만 있는 GUI. OrbitEngine + GravityMapper + WarpDelay 2보이스. bh_x/bh_y/gravity/warp/mix | 거리를 바꾸면 코러스 → wow로 이어지는 소리를 들을 수 있음. 렌더 wav로 청취 |
| M2 | Flutter/Chaos/Voices/Width/Feedback, 스무딩, 템포 싱크(PPQ), 결정론 테스트 | 테스트 1–4 PASS |
| M3 | GUI: 블랙홀 드래그/렌즈/꼬리, 제스처 오토메이션, ADVANCED 줄 | DAW에서 드래그 기록 → 재생 |
| M4 | Tone/Tape(확정 시), HQ, 프리셋 7개, 상태 저장 | 프리셋 청취 리뷰 |
| M5 | pluginval, CPU 최적화, Mac AU/VST3 빌드, 데모 영상("GUI가 곧 악기") | 릴리스 후보 |

---
## 부록 A. 미결 사항 (확인 필요)
1. 플러그인 이름은 "ORBIT"을 유지하고 부제로 "Gravity Warp"를 붙일까요, 아니면 이름 자체를 바꿀까요?
2. 행성을 손으로 잡고 있을 때 궤도를 멈추고 손 위치를 따라가게 할까요(Hold 연주)? 그 움직임도 오토메이션으로 기록할까요?
3. 예전 트레몰로/오빗 팬 기능을 옵션으로 남길까요, 완전히 뺄까요?
4. Tape(포화) 노브를 넣을까요, 순수 모듈레이션만 둘까요?
5. 블랙홀에 아주 가까워지면 소리가 살짝 빨려 들어가는 연출(Horizon Bloom: 볼륨↓, 피드백↑)을 넣을까요?
6. 싱크 목록에 긴 주기(2/1, 4/1, 점음표)를 추가할까요?
7. 고품질(HQ) 모드를 1차 버전에 넣을까요, 나중으로 미룰까요?
8. v0.3 세션/프리셋과의 호환을 지켜야 할까요(예: Depth → Gravity 자동 변환), 아니면 새 제품으로 끊을까요?
9. 블랙홀은 1개로 할까요, 2개까지 허용할까요?
10. 기본 Mix를 50%로 할까요, 데일리용으로 더 낮게(30–40%) 할까요?
