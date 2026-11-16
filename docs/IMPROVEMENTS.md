# ORBIT · Gravity Warp 개선 목록 (우선순위순)

기준: 커밋 a32dcd4 이후 레벨 매칭을 반영한 상태 (2026-10-26 KST). P1은 릴리스를 막는 항목, P2는 릴리스 직후, P3는 나중.

## 1. 릴리스 전 필수 (Must-have)
1. ~~**P1 저역 레벨 누적.**~~ 해결: 180 Hz LowSplit(설계서 22행), 핑크 RMS 최악 +0.42 dB. 원래 내용: LUFS는 ±0.5 LU 안으로 맞췄지만 비가중 RMS는 핑크 노이즈에서 여전히 +0.6~2.7 dB(Tidal Pad 최대)다. 짧은 지연 탭이 저역에서 dry와 같은 위상으로 합쳐져서다. 상관 저역(~150 Hz 이하)에 보이스 합 1/n 정규화를 쓰거나 wet에 저역 크로스오버를 넣는다.
2. **P1 LevelMatch 동작 검증.** 실제 DAW에서 오토메이션(Mix 0→100% 스윕), 긴 무음 뒤 재생, 바이패스 토글을 확인한다. 1.5 s 글라이드가 "늦게 따라오는" 것으로 들리는지 청취해 보고, 필요하면 On/Off 옵션(비자동화 설정)을 검토한다. 파라미터 수 25개는 유지한다.
3. **P1 상태 저장/복원과 버전 태그.** 프리셋/세션 호환을 위해 state에 버전 번호를 넣는다.
4. **P1 레이턴시/테일 보고.** getTailLengthSeconds를 피드백과 Space에 맞게 정확히 보고하고, 오프라인 바운스에서 꼬리가 잘리지 않는지 확인한다.
5. **P1 DAW 호환 매트릭스.** Logic, Ableton, Reaper, Cubase, FL에서 AU와 VST3를 확인한다. 샘플레이트 44.1~192k, 버퍼 32~2048, 모노와 스테레오 버스를 포함한다.

## 2. UX
1. **P2** Gravity Field에서 블랙홀을 더블클릭하면 리셋하고, Shift+드래그로 미세 조정한다.
2. **P2** 지금 LevelMatch가 적용 중인 게인(dB)을 작게 보여준다. 신뢰를 주고 디버깅에도 쓸모 있다.
3. **P2** A/B 비교 버튼, Undo/Redo, 노브 툴팁 설명을 넣는다.
4. **P3** 창 크기 조절(100/125/150%)과 HiDPI 확인.
5. **P3** 키보드 접근성과 포커스 순서.

## 3. 프리셋
1. **P1** 7개 프리셋을 실제 소스(보컬, 기타, 신스, 드럼 버스)로 다시 청취해 튜닝한다. 레벨은 이제 자동으로 맞춰진다.
2. **P2** 사용자 프리셋을 저장/불러오기하는 브라우저를 만든다(파일 기반, 카테고리 태그).
3. **P2** 장르/용도별 프리셋을 15~20개로 확장한다(Lo-fi, Ambient, Vocal, Drum Bus).
4. **P3** 프리셋 모핑 또는 랜덤화(범위 제한).

## 4. 안정성
1. **P1** 퍼즈 테스트: 극단 파라미터, NaN/Inf 입력, 샘플레이트 변경 중 처리, 블록 크기 1을 넣는다. Denormal 방지(FTZ)도 확인한다.
2. **P1** pluginval strictness 10과 AddressSanitizer/UBSan 빌드를 CI에 넣는다.
3. **P2** 스레드 안전성: GUI 텔레메트리 읽기가 lock-free인지 감사한다.
4. **P2** CPU 회귀 테스트를 CI에 넣는다(지금은 <1%, 경보선 2%).

## 5. 패키징 / 코드사인 / 공증
1. **P1** Apple Developer ID Application 인증서로 VST3와 AU에 서명한다(지금은 ad-hoc). hardened runtime과 timestamp를 켠다.
2. **P1** notarytool로 공증하고 staple한다. Developer ID Installer로 서명한 .pkg 설치기(VST3/AU 선택 설치)를 만든다.
3. **P1** 식별자를 확정했다. 회사 Bearfield, 제조사 코드 Brfd, 플러그인 코드 OrGw, 번들 com.bearfield.ORBIT, 제품 이름 ORBIT. 출시 뒤에는 바꾸지 않는다.
4. **P2** Windows VST3 빌드와 Authenticode 서명, 설치기(Inno Setup).
5. **P2** GitHub Actions로 macOS universal과 Windows 빌드를 자동화하고 아티팩트를 보관한다.
6. **P3** AAX(PACE 서명 필요) 검토.

## 6. 문서
1. **P1** 사용자 매뉴얼: 파라미터 25개 설명, 중력 개념, 프리셋 가이드, LevelMatch 설명.
2. **P1** 설치/제거 안내(macOS 경로, AU 캐시 리셋 방법).
3. **P2** 설계서 v0.3: 저역 누적 처리와 결정 사항을 반영하고 부록 A의 미결 사항을 정리한다.
4. **P2** 변경 이력(CHANGELOG)과 알려진 이슈.
5. **P3** 데모 오디오와 영상, 웹페이지.
